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

#include <map>
#include <memory>
#include <mutex>

#include "boost/asio/io_context.hpp"

#include "stream/Stream.hpp"
#include "stream/WebsocketServer.hpp"

#include "streaming_protocol/ProducerSession.hpp"
#include "streaming_protocol/Logging.hpp"

namespace daq::streaming_protocol {
    class Server {
    public:
        Server(boost::asio::io_context& readerIoContext, uint16_t wsDataPort, LogCallback logCb);
        Server(const Server&) = delete;
        Server& operator= (const Server&) = delete;
        virtual ~Server();
        int start();
        void stop();

        size_t sessionCount() const;
    private:
        /// stream id is the key
        using Sessions = std::map < std::string, std::weak_ptr < ProducerSession > >;
        
        void startWebsocketAccept();
        
        void handleWebsocketTcpAccept(const boost::system::error_code& ec, boost::asio::ip::tcp::socket&& streamSocket);
        
        void createSession(std::shared_ptr<stream::Stream> newStream);
        void updateAvailableSignals(const SignalIds& removedSignals, const SignalIds& addedSignals);

        /// Executed upon detection of disconnect from client
        void removeSessionCb(const std::string& sessionId);
                
        boost::asio::io_context& m_readerIoContext;

        stream::WebsocketServer m_server;
        
        Sessions m_sessions;
        std::mutex m_sessionsMtx;
        SignalIds m_availableSignals;
        LogCallback logCallback;
    };
}
