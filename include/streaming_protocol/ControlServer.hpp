/*
 * Copyright 2022-2023 Blueberry d.o.o.
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

#include <memory>

#include <boost/asio/io_context.hpp>

#include "streaming_protocol/Logging.hpp"
#include "streaming_protocol/Types.h"

namespace daq::streaming_protocol {

class listener;

class ControlServer
{
public:
    /// \addtogroup producer
    /// This class is used by producer to run the control service for subscribing/unsubscribing signals

    /// Executes subscribe/unsubscribe command
    /// \param streamId A unique ID identifying the stream instance
    /// \param command Command received from client, might be 'subscribe' or 'unsubscribe'.
    /// \param signalIds List of signals to be subscribed/unsubscribed.
    /// \param errorMessage Filled with error message to be responded to client if command failed
    /// \return negative status code if command failed or '0' if command succeeded
    using CommandCb = std::function < int (
        const std::string& streamId,
        const std::string& command,
        const SignalIds& signalIds,
        std::string& errorMessage
    ) >;

    ControlServer(boost::asio::io_context& ioc, uint16_t port, CommandCb commandCb, LogCallback logCb);
    ~ControlServer();

    /// Starts control HTTP server on port
    void start();
    void stop();

    uint16_t getPort();

private:
    boost::asio::io_context& m_ioc;
    std::shared_ptr<listener> m_listener;
    uint16_t m_port;
    CommandCb m_commandCb;
    LogCallback logCallback;
};

}
