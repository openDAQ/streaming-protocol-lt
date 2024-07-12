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

#pragma once

#include <string>
#include <stdexcept>

#include <nlohmann/json.hpp>

#include "streaming_protocol/Types.h"
#include "HttpPost.hpp"
#include "streaming_protocol/Logging.hpp"

namespace Json {
    class Value;
}

namespace daq::streaming_protocol {
    /// Used to send commands (subscribe/unsubscribe) to the streaming control http port.
    class Controller
    {
    public:
        using ResultCb = std::function <void (const boost::system::error_code& ec) >;
        /// \param httpVersion 10 for http version 1.0, 11 for http version 1.1...
        /// \throws std::runtime_error
        Controller(boost::asio::io_context& ioc, const std::string& streamId, const std::string& address, const std::string& port, const std::string &target, unsigned int httpVersion,
                   daq::streaming_protocol::LogCallback logCb);
        Controller(const Controller&) = delete;
        Controller& operator= (const Controller&) = delete;

        /// \param signalIds several signals might be subscribed with one request to the control port.
        /// \throws std::exception
        void asyncSubscribe(const SignalIds& signalIds, ResultCb resultCb);

        /// \param signalIds several signals might be unsubscribed with one request to the control port.
        /// \throws std::exception
        void asyncUnsubscribe(const SignalIds& signalIds, ResultCb resultCb);

    private:
        nlohmann::json createRequest(const SignalIds& signalIds, const char* method);
        /// \throws std::exception
        void execute(const nlohmann::json &request, HttpPost::ResultCb resultCb);

        boost::asio::io_context& m_ioc;
        std::string m_streamId;
        std::string m_address;
        std::string m_port;
        std::string m_target;
        unsigned int m_httpVersion;
        daq::streaming_protocol::LogCallback logCallback;

        static unsigned int s_id;
    };
}
