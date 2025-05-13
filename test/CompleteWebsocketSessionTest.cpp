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

#include <thread>
#include <gtest/gtest.h>

#include "boost/asio/io_context.hpp"

#include "stream/WebsocketClientStream.hpp"
#include "stream/WebsocketServer.hpp"

#include "streaming_protocol/Defines.h"
#include "streaming_protocol/ProducerSession.hpp"
#include "streaming_protocol/ProtocolHandler.hpp"
#include "streaming_protocol/SignalContainer.hpp"

#include "streaming_protocol/Logging.hpp"

namespace daq::streaming_protocol {

    class CompleteSession : public ::testing::Test {

    protected:

        CompleteSession()
            : m_server(m_ioContext, std::bind(&CompleteSession::acceptCb, this, std::placeholders::_1), ServerPort)
            , logCallback(Logging::logCallback())
        {
        }

        virtual ~CompleteSession()
        {
        }

        virtual void SetUp()
        {
            auto threadFunction =[this]()
            {
                m_ioContext.run();
            };
            m_server.start();
            m_workerThread = std::thread(threadFunction);

        }

        virtual void TearDown()
        {
            m_ioContext.stop();
            m_workerThread.join();
        }

        void stopServer()
        {
            m_server.stop();
            for(auto &iter : m_sesions) {
                iter.second->stop();
            }
        }

        void errCb (const std::string& sessionId, const boost::system::error_code& ec) {
            // remove the session
            std::cerr << sessionId << ": Protocol handler stopped: " << ec.message() << std::endl;
            m_sesions.erase(sessionId);
        };

        void acceptCb(std::shared_ptr < daq::stream::Stream > newStream)
        {
            std::cout << "accepted client (" << newStream->endPointUrl() << ")..." << std::endl;
            auto producerSession = std::make_shared<ProducerSession>(newStream, nlohmann::json(), logCallback);
            std::string sessionId = newStream->endPointUrl();
            // add sessions and start it
            m_sesions[sessionId] = producerSession;
            producerSession->start(std::bind(&CompleteSession::errCb, this, sessionId, std::placeholders::_1));
        }

        static const uint16_t ServerPort = 8000;
        boost::asio::io_context m_ioContext;
        std::thread m_workerThread;
        daq::stream::WebsocketServer m_server;
        std::map <std::string, std::shared_ptr<ProducerSession>> m_sesions;
        LogCallback logCallback;
    };


    TEST_F(CompleteSession, test_connect_wait_for_init_disconnect)
    {
        boost::asio::io_context clientIoContext;
        SignalContainer signalContainer(logCallback);

        std::vector <std::string> streamMetaMethods;

        auto StreamMetaCb = [&](ProtocolHandler& prtotocolHandler, const std::string& method, const nlohmann::json& params)
        {
            streamMetaMethods.push_back(method);
            if (method==META_METHOD_INIT) {
                prtotocolHandler.stop();
            }
        };

        auto CompletionCb = [](const boost::system::error_code& ec)
        {
            ASSERT_EQ(boost::system::error_code(), ec);
        };

        auto clientStream = std::make_unique<stream::WebsocketClientStream>(clientIoContext, "localhost", std::to_string(ServerPort));
        auto protocolHandler = std::make_shared<ProtocolHandler>(clientIoContext, signalContainer, StreamMetaCb, logCallback);
        protocolHandler->start(std::move(clientStream), CompletionCb);

        clientIoContext.run();
        ASSERT_EQ(streamMetaMethods.size(), 2);
        ASSERT_EQ(streamMetaMethods[0], META_METHOD_APIVERSION);
        ASSERT_EQ(streamMetaMethods[1], META_METHOD_INIT);
        protocolHandler->stop();
    }

    TEST_F(CompleteSession, test_connect_disconnect_from_server)
    {
        boost::asio::io_context clientIoContext;
        SignalContainer signalContainer(logCallback);

        auto StreamMetaCb = [&](ProtocolHandler& protocolHandler, const std::string& method, const nlohmann::json& params)
        {
            if (method==META_METHOD_INIT) {
                stopServer();
            }
        };

        auto CompletionCb = [](const boost::system::error_code& ec)
        {
            ASSERT_EQ(boost::beast::websocket::error::closed, ec) << ec.message();
        };

        auto clientStream = std::make_unique<stream::WebsocketClientStream>(clientIoContext, "localhost", std::to_string(ServerPort));
        auto protocolHandler = std::make_shared<ProtocolHandler>(clientIoContext, signalContainer, StreamMetaCb, logCallback);
        protocolHandler->start(std::move(clientStream), CompletionCb);

        clientIoContext.run();
    }

    TEST_F(CompleteSession, test_connect_disconnect)
    {
        boost::asio::io_context clientIoContext;

        auto logCallback = [](spdlog::source_loc location, spdlog::level::level_enum level, const char* msg)
        {
        };

        SignalContainer signalContainer(logCallback);

        auto StreamMetaCb = [&](ProtocolHandler& protocolHandler, const std::string& method, const nlohmann::json& params)
        {
        };

        auto CompletionCb = [](const boost::system::error_code& ec)
        {
        };

        auto clientStream = std::make_unique<stream::WebsocketClientStream>(clientIoContext, "localhost", std::to_string(ServerPort));
        auto protocolHandler = std::make_shared<ProtocolHandler>(clientIoContext, signalContainer, StreamMetaCb, logCallback);
        // using this causes leak!
        //protocolHandler->start(std::move(clientStream), CompletionCb);
        protocolHandler->startWithSyncInit(std::move(clientStream), CompletionCb);
        protocolHandler->stop();

        clientIoContext.run();
    }
}
