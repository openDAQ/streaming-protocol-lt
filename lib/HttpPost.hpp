/*
 * Copyright 2022-2025 openDAQ d.o.o.
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
#include <boost/beast/version.hpp>
#include <string>
#include "streaming_protocol/Logging.hpp"

namespace beast = boost::beast;         // from <boost/beast.hpp>
namespace http = beast::http;           // from <boost/beast/http.hpp>
namespace net = boost::asio;            // from <boost/asio.hpp>
using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>


// Performs an HTTP GET and prints the response
class HttpPost : public std::enable_shared_from_this<HttpPost>
{
public:
    using ResultCb = std::function < void(const boost::system::error_code& ec) >;
    /// \throw std::runtime_error on error
    /// \param host If empty, we assume localhost
    explicit HttpPost(boost::asio::io_context& ioc, const std::string& host, const std::string& port, const std::string& target, unsigned int protocolVersion,
                      daq::streaming_protocol::LogCallback logCb);
    ~HttpPost() = default;

    /// Start the asynchronous operation
    /// @param protocolVersion 10 for 1.0, 11 for 1.1
    void run(const std::string& request, ResultCb resultCb);
    void on_resolve(beast::error_code ec, tcp::resolver::results_type results);
    void on_connect(beast::error_code ec, tcp::resolver::results_type::endpoint_type);
    void on_write(beast::error_code ec, std::size_t bytes_transferred);
    void on_read(beast::error_code ec, std::size_t bytes_transferred);
private:
    std::string m_host;
    std::string m_port;
    std::string m_target;
    unsigned int m_protocolVersion;
    ResultCb m_resultCb;
    
    tcp::resolver m_resolver;
    beast::tcp_stream m_stream;
    beast::flat_buffer m_buffer; // (Must persist between reads)
    http::request<http::string_body> m_request;
    http::response<http::string_body> m_response;
    daq::streaming_protocol::LogCallback logCallback;

    void report_failure(beast::error_code ec, char const* what);
};
