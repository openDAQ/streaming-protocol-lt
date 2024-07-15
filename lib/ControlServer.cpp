/*
 * Copyright 2022-2024 openDAQ d.o.o.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/http/string_body.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio/strand.hpp>
#include <boost/config.hpp>
#include <algorithm>
#include <functional>
#include <memory>
#include <string>
#include <vector>

#include <utility>
#include <vector>

#include "nlohmann/json.hpp"

#include "streaming_protocol/common.hpp"
#include "streaming_protocol/jsonrpc_defines.hpp"
#include <spdlog/spdlog.h>
#include "streaming_protocol/ControlServer.hpp"

namespace beast = boost::beast;         // from <boost/beast.hpp>
namespace http = beast::http;           // from <boost/beast/http.hpp>
namespace net = boost::asio;            // from <boost/asio.hpp>
using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>



BEGIN_NAMESPACE_STREAMING_PROTOCOL


/// This function produces an HTTP response for the given
/// request. The type of the response object depends on the
/// contents of the request, so the interface requires the
/// caller to pass a generic lambda for receiving the response.
template<class Body, class Allocator, class Send>
void handle_request(http::request<Body, http::basic_fields<Allocator>>&& req,
    Send&& send, ControlServer::CommandCb commandCb, LogCallback logCallback)
{
    // Returns a bad request response
    auto const bad_request =
    [&req, &logCallback](beast::string_view why)
    {
        std::string whyAsString(why);
        http::response<http::string_body> res{http::status::bad_request, req.version()};
        res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
        res.set(http::field::content_type, "text/html");
        res.keep_alive(req.keep_alive());
        res.body() = whyAsString;
        res.prepare_payload();
        STREAMING_PROTOCOL_LOG_E("Bad request: {}", whyAsString);
        return res;
    };

    // Make sure we can handle the method
    if( req.method() != http::verb::post) {
        return send(bad_request("Unknown HTTP-method"));
    }

    /* This is where the magic happens */
    std::string body = req.body();


    auto request = nlohmann::json::parse(body.c_str(), body.c_str() + body.size());


    const auto& id = request[daq::jsonrpc::ID];
    if (id.is_null()) {
        return send(bad_request("json rpc request without id"));
    }
    const auto& method = request[daq::jsonrpc::METHOD];
    if (method.is_null()) {
        return send(bad_request("json rpc request without method"));
    }

    std::string methodString = method;
    static const char delimiter = '.';
    auto pos = methodString.find(delimiter);
    if (pos==std::string::npos) {
        std::string message("json rpc request with invalid method '" + methodString + "'. Expecting <stream id>.<command>");
        return send(bad_request(message));
    }
    std::string streamId = methodString.substr(0, methodString.find(delimiter));
    std::string command = methodString.substr(pos + sizeof(delimiter));

    const auto& params = request[daq::jsonrpc::PARAMS];
    if (params.is_null()) {
        return send(bad_request("json rpc request without parameters"));
    }
    
    std::string s_response;

    STREAMING_PROTOCOL_LOG_I("Got request '{}' from '{}'", command, streamId);
    // params holds an array of signal ids
    SignalIds signalIds;
    if (!params.is_array()) {
        return send(bad_request("Expecting an array of signal ids as parameters"));
    }

    for (const auto &iter :params) {
        signalIds.push_back(iter);
    }

    int result = commandCb(streamId, command, signalIds, s_response);
    if (result < 0)
    {
        std::string message("json rpc execution failed: " + s_response);
        return send(bad_request(message));
    }

    std::string res_body = "Succeeded";
    http::response<http::string_body> res {
        std::piecewise_construct,
        std::make_tuple(std::move(res_body)),
        std::make_tuple(http::status::ok, req.version())
    };
    res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
    res.set(http::field::content_type, "application/json");
    res.keep_alive(req.keep_alive());
    return send(std::move(res));
}

static void fail(beast::error_code ec, char const* what, LogCallback logCallback)
{
    STREAMING_PROTOCOL_LOG_E("{}: {}", what, ec.message());
}

/// Handles an HTTP server connection
class session : public std::enable_shared_from_this<session>
{
    // This is the C++11 equivalent of a generic lambda.
    // The function object is used to send an HTTP message.
    struct send_lambda
    {
        session& m_self;

        explicit send_lambda(session& self)
            : m_self(self)
        {
        }

        template<bool isRequest, class Body, class Fields>
        void
        operator()(http::message<isRequest, Body, Fields>&& msg) const
        {
            // The lifetime of the message has to extend
            // for the duration of the async operation so
            // we use a shared_ptr to manage it.
            auto sp = std::make_shared<http::message<isRequest, Body, Fields>>(std::move(msg));

            // Store a type-erased version of the shared
            // pointer in the class to keep it alive.
            m_self.m_res = sp;

            // Write the response
            http::async_write(
                m_self.m_stream,
                *sp,
                beast::bind_front_handler(
                    &session::on_write,
                    m_self.shared_from_this(),
                    sp->need_eof()));
        }
    };

    beast::tcp_stream m_stream;
    beast::flat_buffer m_buffer;
    http::request<http::string_body> m_req;
    std::shared_ptr<void> m_res;
    send_lambda m_lambda;
    ControlServer::CommandCb m_commandCb;
    LogCallback logCallback;

public:
    session(tcp::socket&& socket, ControlServer::CommandCb commandCb, LogCallback logCb)
        : m_stream(std::move(socket))
        , m_lambda(*this)
        , m_commandCb(commandCb)
        , logCallback(logCb)
    {
    }

    // Start the asynchronous operation
    void run()
    {
        do_read();
    }

    void do_read()
    {
        // Make the request empty before reading,
        // otherwise the operation behavior is undefined.
        m_req = {};

        // Set the timeout.
        m_stream.expires_after(std::chrono::seconds(30));

        // Read a request
        http::async_read(m_stream, m_buffer, m_req,
            beast::bind_front_handler(
                &session::on_read,
                shared_from_this()));
    }

    void on_read(beast::error_code ec, std::size_t bytes_transferred)
    {
        boost::ignore_unused(bytes_transferred);

        // This means they closed the connection
        if(ec == http::error::end_of_stream) {
            do_close();
            return;
        }

        if(ec) {
            fail(ec, "read", logCallback);
            return;
        }

        // Handle request and send the response
        handle_request(std::move(m_req), m_lambda, m_commandCb, logCallback);
    }

    void on_write( bool close, beast::error_code ec, std::size_t bytes_transferred)
    {
        boost::ignore_unused(bytes_transferred);

        if(ec) {
            fail(ec, "write", logCallback);
            return;
        }

        if(close) {
            // This means we should close the connection, usually because
            // the response indicated the "Connection: close" semantic.
            do_close();
            return;
        }

        // We're done with the response so delete it
        m_res = nullptr;

        // Read another request
        do_read();
    }

    void do_close()
    {
        // Send a TCP shutdown
        beast::error_code ec;
        m_stream.socket().shutdown(tcp::socket::shutdown_send, ec);

        // At this point the connection is closed gracefully
    }
};

//------------------------------------------------------------------------------

// Accepts incoming connections and launches the sessions
class listener : public std::enable_shared_from_this<listener>
{
    net::io_context& m_ioc;
    tcp::acceptor m_acceptor;
    ControlServer::CommandCb m_commandCb;
    LogCallback logCallback;

public:
    listener(net::io_context& ioc, const tcp::endpoint& endpoint, ControlServer::CommandCb commandCb, LogCallback logCb)
        : m_ioc(ioc)
        , m_acceptor(ioc)
        , m_commandCb(commandCb)
        , logCallback(logCb)
    {
        beast::error_code ec;

        // Open the acceptor
        m_acceptor.open(endpoint.protocol(), ec);
        if(ec)
        {
            fail(ec, "open", logCallback);
            return;
        }

        // Allow address reuse
        m_acceptor.set_option(net::socket_base::reuse_address(true), ec);
        if(ec)
        {
            fail(ec, "set_option", logCallback);
            return;
        }

        // Bind to the server address
        m_acceptor.bind(endpoint, ec);
        if(ec)
        {
            fail(ec, "bind", logCallback);
            return;
        }

        // Start listening for connections
        m_acceptor.listen(
            net::socket_base::max_listen_connections, ec);
        if(ec)
        {
            fail(ec, "listen", logCallback);
            return;
        }
    }

    // Start accepting incoming connections
    void run()
    {
        do_accept();
    }

    // Stop accepting connections
    void stop()
    {
        beast::error_code ec;

        // Close the acceptor
        m_acceptor.close(ec);
        if(ec)
        {
            fail(ec, "close", logCallback);
        }
    }

private:
    void do_accept()
    {
        // The new connection gets its own strand
        m_acceptor.async_accept(
            m_ioc,
            beast::bind_front_handler(
                &listener::on_accept,
                shared_from_this()));
    }

    void on_accept(beast::error_code ec, tcp::socket socket)
    {
        if(ec) {
            fail(ec, "accept", logCallback);
        } else {
            // Create the session and run it
            std::make_shared<session>(std::move(socket), m_commandCb, logCallback)->run();
        }

        // Accept another connection
        do_accept();
    }
};

//------------------------------------------------------------------------------

ControlServer::ControlServer(boost::asio::io_context& ioc, uint16_t port, CommandCb commandCb, LogCallback logCb)
    : m_ioc(ioc)
    , m_port(port)
    , m_commandCb(commandCb)
    , logCallback(logCb)
{

}

ControlServer::~ControlServer()
{
    stop();
}

void ControlServer::start()
{
    // listen to any interface using ipv4 or ipv6
    auto const address = net::ip::make_address("::");

    // Create and launch a listening port
    m_listener = std::make_shared<listener>(m_ioc, tcp::endpoint{address, m_port}, m_commandCb, logCallback);
    m_listener->run();
}

void ControlServer::stop()
{
    if (m_listener)
        m_listener->stop();
}

uint16_t ControlServer::getPort()
{
    return m_port;
}

END_NAMESPACE_STREAMING_PROTOCOL
