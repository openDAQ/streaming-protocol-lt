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

#include <fstream>
#include <thread>
#include <gtest/gtest.h>

#include "boost/asio/io_context.hpp"
#include "nlohmann/json.hpp"

#include "stream/FileStream.hpp"

#include "streaming_protocol/Defines.h"
#include "streaming_protocol/jsonrpc_defines.hpp"
#include "streaming_protocol/ProtocolHandler.hpp"
#include "streaming_protocol/SignalContainer.hpp"
#include "streaming_protocol/StreamWriter.h"

#include "streaming_protocol/Logging.hpp"


namespace daq::streaming_protocol {
    static LogCallback logCallback = daq::streaming_protocol::Logging::logCallback();

    class TestStream : public stream::Stream
    {
    public:
        TestStream()
        {
        }

        /// Start the asynchronous operation
        /// @param CompletionCb Executed after completion to start receiving/sending data
        void asyncInit(CompletionCb readCb) override
        {
            readCb(boost::system::error_code());
        }

        boost::system::error_code init() override
        {
            return boost::system::error_code();
        }

        std::string endPointUrl() const override
        {
            return "";
        }
        std::string remoteHost() const override
        {
            return "";
        }

        void asyncReadAtLeast(std::size_t bytesToRead, ReadCompletionCb readAtLeastCb) override
        {
        }

        size_t readAtLeast(std::size_t bytesToRead, boost::system::error_code &ec) override
        {
            return 0;
        }

        void asyncWrite(const boost::asio::const_buffer& data, Stream::WriteCompletionCb writeCompletionCb) override
        {
            std::ostream os(&m_buffer);
            os.write(reinterpret_cast< const char*>(data.data()), data.size());
            writeCompletionCb(boost::system::error_code(), data.size());
        }

        void asyncWrite(const stream::ConstBufferVector& data, WriteCompletionCb writeCompletionCb) override
        {
            size_t sizeSum = 0;
            std::ostream os(&m_buffer);
            for( const auto& iter : data) {
                os.write(reinterpret_cast< const char*>(iter.data()), iter.size());
                sizeSum += iter.size();
            }
            writeCompletionCb(boost::system::error_code(), sizeSum);
        }

        size_t write(const boost::asio::const_buffer& data, boost::system::error_code& ec) override
        {
            std::ostream os(&m_buffer);
            os.write(reinterpret_cast< const char*>(data.data()), data.size());
            return data.size();
        }

        size_t write(const stream::ConstBufferVector& data, boost::system::error_code& ec) override
        {
            size_t sizeSum = 0;
            std::ostream os(&m_buffer);

            for (const auto& dataIter : data) {
                os.write(reinterpret_cast< const char*>(dataIter.data()), dataIter.size());
                sizeSum += dataIter.size();
            }
            return sizeSum;
        }

        void asyncClose(CompletionCb closeCb) override
        {
            closeCb(boost::system::error_code());
        }

        boost::system::error_code close() override
        {
            return boost::system::error_code();
        }
    };


    TEST(ProtocolHandlerTest, start_stop)
    {
        boost::asio::io_context ioContext;

        auto streamMetaCb = [](ProtocolHandler& prtotocolHandler, const std::string& method, const nlohmann::json& params)
        {
        };

        SignalContainer signalContainer(logCallback);
        auto protocolHandler = std::make_shared<ProtocolHandler>(ioContext, signalContainer, streamMetaCb, logCallback);
        auto fileStream = std::make_unique < TestStream > ();

        auto completionCb = [](const boost::system::error_code& ec) {
            ASSERT_EQ(ec, boost::system::error_code());
        };

        protocolHandler->start(std::move(fileStream), completionCb);
        protocolHandler->stop();
        ioContext.run();
    }

    TEST(ProtocolHandlerTest, start_wait_one_second_stop)
    {
        boost::asio::io_context ioContext;

        auto streamMetaCb = [](ProtocolHandler& prtotocolHandler, const std::string& method, const nlohmann::json& params)
        {
        };

        SignalContainer signalContainer(logCallback);
        auto protocolHandler = std::make_shared<ProtocolHandler>(ioContext, signalContainer, streamMetaCb, logCallback);
        auto fileStream = std::make_unique < TestStream > ();

        auto completionCb = [](const boost::system::error_code& ec) {
            ASSERT_EQ(ec, boost::system::error_code());
        };

        protocolHandler->start(std::move(fileStream), completionCb);

        auto ThreadFunction = [&]()
        {
            ioContext.run();
        };

        auto workerThread = std::thread(ThreadFunction);

        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        protocolHandler->stop();
        workerThread.join();
    }

    TEST(ProtocolHandlerTest, stop_without_start)
    {
        boost::asio::io_context ioContext;

        auto streamMetaCb = [](ProtocolHandler& prtotocolHandler, const std::string& method, const nlohmann::json& params)
        {
        };

        SignalContainer signalContainer(logCallback);
        auto protocolHandler = std::make_shared<ProtocolHandler>(ioContext, signalContainer, streamMetaCb, logCallback);
        auto fileStream = std::make_unique < TestStream > ();

        protocolHandler->stop();
        ioContext.run();
    }


    TEST(ProtocolHandlerTest, start_stop_by_peer)
    {
        static const std::string streamFileName = "dumpFile";
        boost::asio::io_context ioContext;

        {
            // create an empty file
            std::ofstream dumper;
            dumper.open(streamFileName, std::ios_base::trunc);
            dumper.close();
        }

        auto streamMetaCb = [](ProtocolHandler& prtotocolHandler, const std::string& method, const nlohmann::json& params)
        {
        };

        SignalContainer signalContainer(logCallback);
        auto protocolHandler = std::make_shared<ProtocolHandler>(ioContext, signalContainer, streamMetaCb, logCallback);
        auto fileStream = std::make_unique < daq::stream::FileStream > (ioContext, streamFileName);
        std::string url = fileStream->endPointUrl();
        ASSERT_FALSE(url.empty());

        auto completionCb = [&](const boost::system::error_code& ec) {
            ASSERT_EQ(ec, boost::asio::error::eof);
        };

        protocolHandler->start(std::move(fileStream), completionCb);
        ioContext.run();
    }

    TEST(ProtocolHandlerTest, streamMetainformation)
    {
        static const std::string streamFileName = "dumpFile";
        boost::asio::io_context ioContext;

        std::string methodSend = META_METHOD_APIVERSION;
        std::string methodReceived;
        nlohmann::json paramsSend;
        nlohmann::json paramsReceived;
        paramsSend[VERSION] = OPENDAQ_LT_STREAM_VERSION;

        {
            nlohmann::json data;
            data[daq::jsonrpc::METHOD] = methodSend;
            data[daq::jsonrpc::PARAMS] = paramsSend;
            auto fileStream = std::make_shared < daq::stream::FileStream > (ioContext, streamFileName, true);
            fileStream->init();
            StreamWriter writer(fileStream);
            writer.writeMetaInformation(0, data);
        }

        auto signalMetaCb = [&](SubscribedSignal& subscribedSignal, const std::string& method, const nlohmann::json& params)
        {
            FAIL();
        };
        auto streamMetaCb = [&](ProtocolHandler& prtotocolHandler, const std::string& method, const nlohmann::json& params)
        {
            methodReceived = method;
            paramsReceived = params;
        };

        SignalContainer signalContainer(logCallback);
        signalContainer.setSignalMetaCb(signalMetaCb);
        auto protocolHandler = std::make_shared<ProtocolHandler>(ioContext, signalContainer, streamMetaCb, logCallback);
        auto fileStream = std::make_unique < daq::stream::FileStream > (ioContext, streamFileName);
        protocolHandler->start(std::move(fileStream));
        ioContext.run();

        ASSERT_EQ(methodSend, methodReceived);
        ASSERT_EQ(paramsSend, paramsReceived);
    }

    TEST(ProtocolHandlerTest, signalMetainformation)
    {
        static const std::string streamFileName = "dumpFile";
        static const unsigned int signalNumber = 4;
        static const std::string signalId = "the signal id";
        boost::asio::io_context ioContext;

        std::string methodSend = META_METHOD_SUBSCRIBE;
        std::string methodReceived;
        nlohmann::json paramsSend;
        nlohmann::json paramsReceived;
        paramsSend[META_SIGNALID] = signalId;

        {
            nlohmann::json data;
            data[daq::jsonrpc::METHOD] = methodSend;
            data[daq::jsonrpc::PARAMS] = paramsSend;

            auto fileStream = std::make_shared < daq::stream::FileStream > (ioContext, streamFileName, true);
            fileStream->init();
            StreamWriter writer(fileStream);
            writer.writeMetaInformation(signalNumber, data);
        }

        auto signalMetaCb = [&](SubscribedSignal& subscribedSignal, const std::string& method, const nlohmann::json& params)
        {
            methodReceived = method;
            paramsReceived = params;
        };

        auto streamMetaCb = [&](ProtocolHandler& prtotocolHandler, const std::string& method, const nlohmann::json& params)
        {
            FAIL();
        };

        SignalContainer signalContainer(logCallback);
        signalContainer.setSignalMetaCb(signalMetaCb);
        auto protocolHandler = std::make_shared<ProtocolHandler>(ioContext, signalContainer, streamMetaCb, logCallback);
        auto fileStream = std::make_unique < daq::stream::FileStream > (ioContext, streamFileName);
        protocolHandler->start(std::move(fileStream));
        ioContext.run();

        ASSERT_EQ(methodSend, methodReceived);
        ASSERT_EQ(paramsSend, paramsReceived);
    }
}
