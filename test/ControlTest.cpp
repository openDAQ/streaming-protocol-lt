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

#include <gtest/gtest.h>

#include "boost/asio/io_context.hpp"

#include "streaming_protocol/Types.h"

#include "streaming_protocol/Logging.hpp"

#include "../lib/Controller.hpp"
#include "streaming_protocol/ControlServer.hpp"

#include <thread>
#include <future>

namespace daq::streaming_protocol {
    static const std::string streamId = "theId";;
    static const std::string signalId = "a signal id";
    static const std::string address = "localhost";
    static const uint16_t port = 7438;
    static const std::string target = "/";
    static const unsigned int httpVersion = 11;
    static LogCallback logCallback = daq::streaming_protocol::Logging::logCallback();
    static const std::chrono::seconds timeout = std::chrono::seconds(2);

    TEST(ControlClientTest, empty_stream_id)
    {
        boost::asio::io_context ioc;
        // excepction because of empty stream id
        EXPECT_THROW(Controller controller(ioc, "", address, std::to_string(port), target, httpVersion, logCallback), std::runtime_error);
    }

    TEST(ControlClientTest, signal_id_list_subscribe_unsubscribe)
    {
        boost::asio::io_context ioc;
        Controller controller(ioc, streamId, address, std::to_string(port), target, httpVersion, logCallback);
        SignalIds signalIds;

        boost::system::error_code ec;
        unsigned int count = 0;
        auto resultCb = [&](const boost::system::error_code& cbEc) {
            ec = cbEc;
            ++count;
        };

        signalIds.push_back(signalId);
        controller.asyncSubscribe(signalIds, resultCb);
        controller.asyncUnsubscribe(signalIds, resultCb);

        ioc.run();
        ASSERT_EQ(count, 2);
    }

    TEST(ControlServerClientTest, subscribe_unsubscribe_success)
    {
        boost::asio::io_context ioc;
        Controller controller(ioc, streamId, address, std::to_string(port), target, httpVersion, logCallback);
        SignalIds signalIds;

        std::string recvStreamId;
        std::string recvCommand;
        SignalIds recvSignalIds;
        std::promise<void> serverPromise;
        ControlServer::CommandCb commandCb = [&](
            const std::string& _streamId,
            const std::string& _command,
            const SignalIds& _signalIds,
            std::string& /*_errorMessage*/
        )
        {
            recvStreamId = _streamId;
            recvCommand = _command;
            recvSignalIds = _signalIds;
            serverPromise.set_value();
            return 0;
        };
        ControlServer server(ioc, port, commandCb, logCallback);

        boost::system::error_code ec;
        unsigned int count = 0;
        std::promise<void> clientPromise;
        auto resultCb = [&](const boost::system::error_code& cbEc) {
            ec = cbEc;
            ++count;
            clientPromise.set_value();
        };

        signalIds.push_back(signalId);
        server.start();
        auto workerThread = std::thread([&]() { ioc.run(); });

        std::future<void> serverFuture = serverPromise.get_future();
        std::future<void> clientFuture = clientPromise.get_future();
        controller.asyncSubscribe(signalIds, resultCb);
        EXPECT_EQ(serverFuture.wait_for(timeout), std::future_status::ready);
        EXPECT_EQ(recvStreamId, streamId);
        EXPECT_EQ(recvCommand, "subscribe");
        EXPECT_EQ(recvSignalIds, signalIds);
        EXPECT_EQ(clientFuture.wait_for(timeout), std::future_status::ready);
        EXPECT_EQ(count, 1);
        EXPECT_FALSE(ec.failed());

        serverPromise = std::promise<void>();
        clientPromise = std::promise<void>();
        serverFuture = serverPromise.get_future();
        clientFuture = clientPromise.get_future();
        controller.asyncUnsubscribe(signalIds, resultCb);
        EXPECT_EQ(serverFuture.wait_for(timeout), std::future_status::ready);
        EXPECT_EQ(recvStreamId, streamId);
        EXPECT_EQ(recvCommand, "unsubscribe");
        EXPECT_EQ(recvSignalIds, signalIds);
        EXPECT_EQ(clientFuture.wait_for(timeout), std::future_status::ready);
        EXPECT_EQ(count, 2);
        EXPECT_FALSE(ec.failed());

        ioc.stop();
        workerThread.join();
    }

    TEST(ControlServerClientTest, server_error)
    {
        boost::asio::io_context ioc;
        Controller controller(ioc, streamId, address, std::to_string(port), target, httpVersion, logCallback);
        SignalIds signalIds;

        std::promise<void> serverPromise;
        ControlServer::CommandCb commandCb = [&](
            const std::string& /*_streamId*/,
            const std::string& /*_command*/,
            const SignalIds& /*_signalIds*/,
            std::string& _errorMessage
        )
        {
            _errorMessage = "server error";
            serverPromise.set_value();
            return -1;
        };
        ControlServer server(ioc, port, commandCb, logCallback);

        boost::system::error_code ec;
        unsigned int count = 0;
        std::promise<void> clientPromise;
        auto resultCb = [&](const boost::system::error_code& cbEc) {
            ec = cbEc;
            ++count;
            clientPromise.set_value();
        };

        signalIds.push_back(signalId);
        server.start();
        auto workerThread = std::thread([&]() { ioc.run(); });

        std::future<void> serverFuture = serverPromise.get_future();
        std::future<void> clientFuture = clientPromise.get_future();
        controller.asyncSubscribe(signalIds, resultCb);
        EXPECT_EQ(serverFuture.wait_for(timeout), std::future_status::ready);
        EXPECT_EQ(clientFuture.wait_for(timeout), std::future_status::ready);
        EXPECT_EQ(count, 1);
        EXPECT_FALSE(ec.failed());

        ioc.stop();
        workerThread.join();
    }

}
