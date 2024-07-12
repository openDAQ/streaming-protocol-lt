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

#include <thread>
#include <gtest/gtest.h>

#include "boost/asio/io_context.hpp"

#include "stream/WebsocketClientStream.hpp"

#include "streaming_protocol/Server.hpp"

#include "streaming_protocol/Logging.hpp"


namespace daq::streaming_protocol {
    static LogCallback logCallback = daq::streaming_protocol::Logging::logCallback();

    TEST(ServerTest, start_stop)
    {
        unsigned int listeningPort = 5000;
        boost::asio::io_context ioc;
        Server server(ioc, listeningPort, logCallback);
        ASSERT_EQ(server.start(), 0);
        server.stop();
        ioc.run();
    }


    TEST(ServerTest, stayingClient)
    {
        unsigned int listeningPort = 5000;
        boost::asio::io_context ioc;
        Server server(ioc, listeningPort, logCallback);
        ASSERT_EQ(server.start(), 0);

        stream::WebsocketClientStream client(ioc, "localhost", std::to_string(listeningPort), "/");

        boost::system::error_code result;
        std::thread clientThread;

        // if not reading on client side, close from the server side would time out!
        auto readFromClient = [&]() {
            boost::system::error_code ec;
            std::size_t bytesRead;
            do {
                bytesRead = client.readSome(ec);
                if (ec) {
                    ASSERT_EQ(ec, boost::beast::websocket::error::closed);
                    return;
                }
                client.consume(bytesRead);
            } while(bytesRead);
        };

        auto initCb =[&](const boost::system::error_code& ec) {
            if (ec) {
                result = ec;
            }
            ASSERT_EQ(server.sessionCount(), 1);
            // after succesfull init we start a thread that reads until being closed!
            clientThread = std::thread(readFromClient);
            server.stop();
        };
        client.asyncInit(initCb);

        ioc.run();
        ASSERT_EQ(result, boost::system::error_code());
        clientThread.join();
    }

    TEST(ServerTest, temporaryClient)
    {
        unsigned int listeningPort = 5000;
        boost::asio::io_context ioc;
        Server server(ioc, listeningPort, logCallback);
        ASSERT_EQ(server.start(), 0);

        stream::WebsocketClientStream client(ioc, "localhost", std::to_string(listeningPort), "/");

        boost::system::error_code result;

        auto closeCb = [&](const boost::system::error_code& ec) {
            if (ec) {
                result = ec;
            }
            server.stop();
            ASSERT_EQ(server.sessionCount(), 0);
        };

        auto initCb =[&](const boost::system::error_code& ec) {
            if (ec) {
                result = ec;
            }
            ASSERT_EQ(server.sessionCount(), 1);
            client.asyncClose(closeCb);
        };
        ASSERT_EQ(server.sessionCount(), 0);
        client.asyncInit(initCb);

        ioc.run();
        ASSERT_EQ(result, boost::system::error_code());
    }
}
