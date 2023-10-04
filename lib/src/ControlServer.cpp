//
// Copyright (c) 2016-2019 Vinnie Falco (vinnie dot falco at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/boostorg/beast
//

//------------------------------------------------------------------------------
//
// Example: HTTP server, asynchronous
//
//------------------------------------------------------------------------------

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
#include "streaming_protocol/ProducerSession.hpp"

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
    Send&& send)
{
    // Returns a bad request response
    auto const bad_request =
    [&req](beast::string_view why)
    {
        std::string whyAsString(why);
        http::response<http::string_body> res{http::status::bad_request, req.version()};
        res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
        res.set(http::field::content_type, "text/html");
        res.keep_alive(req.keep_alive());
        res.body() = whyAsString;
        res.prepare_payload();
        spdlog::error("Bad request: {}", whyAsString);
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
        std::string message("json rpc request with invalid method '%s'. Expecting <stream id>.<command>", methodString.c_str());
        return send(bad_request(message));
    }
    std::string streamId = methodString.substr(0, methodString.find(delimiter));
    std::string command = methodString.substr(pos + sizeof(delimiter));

    const auto& params = request[daq::jsonrpc::PARAMS];
    if (params.is_null()) {
        return send(bad_request("json rpc request without parameters"));
    }
    
    nlohmann::json s_response;

    spdlog::info("Got request '{}' from '{}'", command, streamId);
    // params holds an array of signal ids
    SignalIds signalIds;
    if (!params.is_array()) {
        return send(bad_request("Expecting an array of signal ids as parameters"));
    }

    for (const auto &iter :params) {
        signalIds.push_back(iter);
    }

    /// \todo
    /// - Find the producer session using the stream id
    /// - Depending on command, call subscribe or unsubscribe with the signal ids
    /// - create response for response body

    std::string res_body = s_response.dump();
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

static void fail(beast::error_code ec, char const* what)
{
    spdlog::error("{}: {}", what, ec.message());
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

public:
    session(tcp::socket&& socket)
        : m_stream(std::move(socket))
        , m_lambda(*this)
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
            fail(ec, "read");
            return;
        }

        // Send the response
        handle_request(std::move(m_req), m_lambda);
    }

    void on_write( bool close, beast::error_code ec, std::size_t bytes_transferred)
    {
        boost::ignore_unused(bytes_transferred);

        if(ec) {
            fail(ec, "write");
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

public:
    listener(net::io_context& ioc, const tcp::endpoint& endpoint)
        : m_ioc(ioc)
        , m_acceptor(ioc)
    {
        beast::error_code ec;

        // Open the acceptor
        m_acceptor.open(endpoint.protocol(), ec);
        if(ec)
        {
            fail(ec, "open");
            return;
        }

        // Allow address reuse
        m_acceptor.set_option(net::socket_base::reuse_address(true), ec);
        if(ec)
        {
            fail(ec, "set_option");
            return;
        }

        // Bind to the server address
        m_acceptor.bind(endpoint, ec);
        if(ec)
        {
            fail(ec, "bind");
            return;
        }

        // Start listening for connections
        m_acceptor.listen(
            net::socket_base::max_listen_connections, ec);
        if(ec)
        {
            fail(ec, "listen");
            return;
        }
    }

    // Start accepting incoming connections
    void run()
    {
        do_accept();
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
            fail(ec, "accept");
        } else {
            // Create the session and run it
            std::make_shared<session>(std::move(socket))->run();
        }

        // Accept another connection
        do_accept();
    }
};

//------------------------------------------------------------------------------

//int main(int argc, char* argv[])
//{
//    // Check command line arguments.
//    if (argc != 2) {
//        std::cerr <<
//            "Usage: http-server-async <port>\n" <<
//            "Example:\n" <<
//            "    http-server-async 8080\n";
//        return EXIT_FAILURE;
//    }
//    // listen to any interface using ipv4 or ipv6
//    auto const address = net::ip::make_address("::");
//    auto const port = static_cast<unsigned short>(std::atoi(argv[1]));

//    // The io_context is required for all I/O
//    net::io_context ioc;

//    // Create and launch a listening port
//    std::make_shared<listener>(ioc, tcp::endpoint{address, port})->run();
//    ioc.run();

//    return EXIT_SUCCESS;
//}

END_NAMESPACE_STREAMING_PROTOCOL
