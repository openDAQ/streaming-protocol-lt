#include <boost/beast/core.hpp>

#if defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
// gcc version 13.2.1 reports maybe-unitialized warning in boost/beast/http.hpp. Suppress it
#endif

#include <boost/beast/http.hpp>

#if defined(__GNUC__)
#pragma GCC diagnostic pop
#endif

#include <boost/beast/version.hpp>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <memory>
#include <string>

#include "HttpPost.hpp"

#include "stream/utils/boost_compatibility_utils.hpp"

namespace beast = boost::beast;         // from <boost/beast.hpp>
namespace http = beast::http;           // from <boost/beast/http.hpp>
namespace net = boost::asio;            // from <boost/asio.hpp>
using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>
namespace bsp = daq::streaming_protocol;

// Report a failure
void HttpPost::report_failure(beast::error_code ec, char const* what)
{
    STREAMING_PROTOCOL_LOG_E("{}: {}", what, ec.message());
}

HttpPost::HttpPost(boost::asio::io_context& ioc, const std::string& host, const std::string& port, const std::string& target, unsigned int protocolVersion,
                   daq::streaming_protocol::LogCallback logCb)
    : m_host(host)
    , m_port(port)
    , m_target(target)
    , m_protocolVersion(protocolVersion)
    , m_resolver(ioc)
    , m_stream(ioc)
    , logCallback(logCb)
{
    if(m_host.empty()) {
        m_host = "localhost";
    }

    if(m_port.empty()) {
        throw std::runtime_error("port not provided");
    }

    if(m_target.empty()) {
        throw std::runtime_error("target not provided");
    }
}

// Start the asynchronous operation
void HttpPost::run(const std::string& request, ResultCb resultCb)
{
    STREAMING_PROTOCOL_LOG_D("{} target: {} request: {}", __FUNCTION__, request, m_target);
    m_resultCb = resultCb;
    // Set up an HTTP POST request message
    m_request.version(m_protocolVersion);
    m_request.method(http::verb::post);
    m_request.target(m_target);
    m_request.set(http::field::host, m_host);
    m_request.set(http::field::content_type, "application/json; charset=utf-8");
    m_request.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);
    m_request.body() = request;
    m_request.prepare_payload();
    
    // Look up the domain name
    m_resolver.async_resolve(
                m_host,
                m_port,
                beast::bind_front_handler(
                    &HttpPost::on_resolve,
                    shared_from_this()));
}

void HttpPost::on_resolve(beast::error_code ec, tcp::resolver::results_type results)
{
    if(ec) {
        report_failure(ec, "resolve");
        m_resultCb(ec);
        return;
    }
    
    // Set a timeout on the operation
    //boost::system::error_code localEc;
    //m_stream.expires_after(std::chrono::seconds(30), localEc);
    m_stream.expires_after(std::chrono::seconds(30));

    // Make the connection on the IP address we get from a lookup
    m_stream.async_connect(
                results,
                beast::bind_front_handler(
                    &HttpPost::on_connect,
                    shared_from_this()));
}

void HttpPost::on_connect(beast::error_code ec, tcp::resolver::results_type::endpoint_type)
{
    if(ec) {
        report_failure(ec, "connect");
        m_resultCb(ec);
        return;
    }
    
    // Set a timeout on the operation
    m_stream.expires_after(std::chrono::seconds(30));
    
    // Send the HTTP request to the remote host
    auto self = shared_from_this();
    auto writeCallback = [self](beast::error_code ec, std::size_t bytes_transferred)
    {
        self->on_write(ec, bytes_transferred);
    };
    daq::stream::boost_compatibility_utils::async_write(m_stream, m_request, writeCallback);
}

void HttpPost::on_write(beast::error_code ec, std::size_t)
{    
    if(ec) {
        report_failure(ec, "write");
        m_resultCb(ec);
        return;
    }
    
    // Receive the HTTP response
    http::async_read(m_stream, m_buffer, m_response,
                     beast::bind_front_handler(
                         &HttpPost::on_read,
                         shared_from_this()));
}

void HttpPost::on_read(beast::error_code ec, std::size_t)
{    
    if(ec) {
        report_failure(ec, "read");
        m_resultCb(ec);
        return;
    }
    
    // Write the message to standard out
    if (static_cast<http::status>(m_response.result_int()) != http::status::ok)
    {
        STREAMING_PROTOCOL_LOG_E("Request failed with code {} : {}",
                                 m_response.result_int(), m_response.body());
    }
    else
    {
        STREAMING_PROTOCOL_LOG_D("Request succeeded, response: {}", m_response.body());
    }
    
    // Gracefully close the socket. Ignore error from that.
    beast::error_code dummyEc;
    m_stream.socket().shutdown(tcp::socket::shutdown_both, dummyEc);
    
    m_resultCb(ec);
    // not_connected happens sometimes so don't bother reporting it.
    if(dummyEc && dummyEc != beast::errc::not_connected) {
        report_failure(dummyEc, "shutdown");
    }
    // If we get here then the connection is closed gracefully
}
