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
#include <gtest/gtest.h>

#include <fstream>

#include "boost/asio/error.hpp"
#include "boost/asio/io_context.hpp"
#include "boost/system/error_code.hpp"

#ifdef _WIN32
#error The current FileStream implementation for windows is not working correctly. When reading from file, second read returns the same data as the first read
#else
#include "stream/FileStream.hpp"
#endif

#include "streaming_protocol/BaseDomainSignal.hpp"
#include "streaming_protocol/Defines.h"
#include "streaming_protocol/LinearTimeSignal.hpp"
#include "streaming_protocol/ProducerSession.hpp"
#include "streaming_protocol/ProtocolHandler.hpp"
#include "streaming_protocol/StreamWriter.h"
#include "streaming_protocol/SignalContainer.hpp"
#include "streaming_protocol/SynchronousSignal.hpp"

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
        return 0;
    }

    size_t write(const stream::ConstBufferVector& data, boost::system::error_code& ec) override
    {
        return 0;
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

TEST(ProducerSessionTest, start_without_stop)
{
    static const std::string fileName = "theFile";

    boost::asio::io_context ioc;
    // file is created for writing, read will fail which causes error callback to be called.
    auto testStream = std::make_shared<TestStream>();
    boost::system::error_code resultEc;

    resultEc = testStream->init();
    ASSERT_EQ(resultEc, boost::system::error_code());
    auto producerSession = std::make_shared<ProducerSession>(testStream, nlohmann::json(), logCallback);
    auto errorCb = [&](const boost::system::error_code& ec) {
        // should not be called since stopped normally.
        FAIL();
        resultEc = ec;
    };
    producerSession->start(errorCb);
    ioc.run();
    ASSERT_EQ(boost::system::error_code(), resultEc);
}

/// Provide a stream to wacht for. Here we use a file which causes end of file when finished reading.
/// This equals disconnect of a network stream and should cause the session to stop.
TEST(ProducerSessionTest, start_read_eof)
{
    static const std::string fileName = "theFile";

    {
        std::ofstream file;
        file.open(fileName);
        file << "something";
        file.close();
    }

    boost::asio::io_context ioc;
    auto fileStream = std::make_shared<stream::FileStream>(ioc, fileName);
    boost::system::error_code resultEc;

    resultEc = fileStream->init();
    ASSERT_EQ(resultEc, boost::system::error_code());
    auto producerSession = std::make_shared<ProducerSession>(fileStream, nlohmann::json(), logCallback);
    auto errorCb = [&](const boost::system::error_code& ec) {
        resultEc = ec;
    };
    producerSession->start(errorCb);
    ioc.run();
    ASSERT_EQ(boost::asio::error::eof, resultEc);
}

TEST(ProducerSessionTest, start_stop)
{
    static const std::string fileName = "theFile";

    boost::asio::io_context ioc;
    // file is created for writing, read will fail which causes error callback to be called.
    auto testStream = std::make_shared<TestStream>();
    boost::system::error_code resultEc;

    resultEc = testStream->init();
    ASSERT_EQ(resultEc, boost::system::error_code());
    auto producerSession = std::make_shared<ProducerSession>(testStream, nlohmann::json(), logCallback);
    auto errorCb = [&](const boost::system::error_code& ec) {
        // should not be called since stopped normally.
        FAIL();
        resultEc = ec;
    };
    producerSession->start(errorCb);
    producerSession->stop();
    ioc.run();
    ASSERT_EQ(boost::system::error_code(), resultEc);
}

TEST(ProducerSessionTest, complete_session)
{
    struct PackageInformation
    {
        TransportType transportType;
        unsigned int signalNumber;
        std::string method;
        nlohmann::json params;
        std::vector<uint8_t> data;
        RelatedSignals relatedSignals;
    };

    static const std::string fileName = "theFile";
    static const std::string signalId = "the 1st Id";
    static const std::string timeSignalId = "the time signal Id";
    static const std::string tableId = "the table Id";
    uint64_t timeTicksPerSecond = 1000000000;
    std::chrono::nanoseconds originalOutputRate = std::chrono::milliseconds(1); // 1kHz
    uint64_t originalOutputRateInTicks = BaseDomainSignal::timeTicksFromNanoseconds(originalOutputRate, timeTicksPerSecond);
    std::chrono::nanoseconds newOutputRate = std::chrono::seconds(1); // 1Hz
    uint64_t newOutputRateInTicks = BaseDomainSignal::timeTicksFromNanoseconds(newOutputRate, timeTicksPerSecond);
    std::vector <double> testData1Small = { 1.0, 67.4365};
    // big data package adds addtional length information to protocol
    std::vector <double> testData2Big;

    Unit originalUnit;
    originalUnit.id = 1;
    originalUnit.displayName = "original unit";

    Unit newUnit;
    newUnit.id = 1111;
    newUnit.displayName = "new unit";

    Range range;
    range.low = -34.9;
    range.high = 1000.1;

    PostScaling postScaling;
    postScaling.offset =-5;
    postScaling.scale = 2;

    for (size_t index = 0; index<1024; ++index) {
        testData2Big.push_back(index * 1.1);
    }

    {
        // Creation of produder session writes "apiVersion" and "init" meta information into file
        boost::asio::io_context ioc;
        auto fileStream = std::make_shared<stream::FileStream>(ioc, fileName, true);
        fileStream->init();
        auto producerSession = std::make_shared<ProducerSession>(fileStream, nlohmann::json(), logCallback);
        StreamWriter writer(fileStream);

        // prepare the signal with initial configuration
        auto timeSignal = std::make_shared<LinearTimeSignal>(timeSignalId, tableId, timeTicksPerSecond, originalOutputRate, writer, logCallback);
        auto syncSignal = std::make_shared<SynchronousSignal<double>>(signalId, tableId, writer, logCallback);
        syncSignal->setUnit(originalUnit.id, originalUnit.displayName);
        RelatedSignals relatedSignals;
        relatedSignals[META_TIME] = timeSignal->getId();
        syncSignal->setRelatedSignals(relatedSignals);

        producerSession->addSignal(timeSignal);
        // since this is a data signal, this causes "availble" meta information
        producerSession->addSignal(syncSignal);

        SignalIds signalIds;
        signalIds.push_back(timeSignalId);
        signalIds.push_back(signalId);
        // this causes "subscribe" and "signal" meta information for the data and time signal
        producerSession->subscribeSignals(signalIds);
        // adds real data
        syncSignal->addData(testData1Small.data(), testData1Small.size());
        syncSignal->addData(testData2Big.data(), testData2Big.size());

        // change unit, range and postScaling of data signal
        syncSignal->setUnit(newUnit.id, newUnit.displayName);
        syncSignal->setRange(range);
        syncSignal->setPostScaling(postScaling);
        syncSignal->writeSignalMetaInformation();

        // change output rate of time signal
        timeSignal->setOutputRate(newOutputRate);
        timeSignal->writeSignalMetaInformation();

        // tear down of signal
        producerSession->unsubscribeSignals(signalIds);
        producerSession->removeSignals(signalIds);
        ioc.run();
    }

    {
        // Consume the file and check for expected meta information in the correct sequence
        boost::asio::io_context ioc;
        SignalContainer signalContainer(logCallback);

        auto fileStream = std::make_unique < daq::stream::FileStream > (ioc, fileName);
        std::string url = fileStream->endPointUrl();
        ASSERT_FALSE(url.empty());

        std::vector<PackageInformation> receivedMetaInformation;
        auto signalMetaCb = [&](SubscribedSignal& subscribedSignal, const std::string& method, const nlohmann::json& params)
        {
            PackageInformation packageInformation;
            packageInformation.transportType = TYPE_METAINFORMATION;
            packageInformation.signalNumber = subscribedSignal.signalNumber();
            packageInformation.method = method;
            packageInformation.params = params;
            packageInformation.relatedSignals = subscribedSignal.relatedSignals();
            receivedMetaInformation.push_back(packageInformation);
        };

        auto dataAsRawCb = [&](const SubscribedSignal& subscribedSignal, uint64_t timeStamp, const uint8_t* data, size_t byteCount)
        {
            PackageInformation packageInformation;
            packageInformation.transportType = TYPE_SIGNALDATA;
            packageInformation.signalNumber = subscribedSignal.signalNumber();
            packageInformation.data.resize(byteCount);
            memcpy(packageInformation.data.data(), data, byteCount);
            receivedMetaInformation.push_back(packageInformation);
        };

        auto streamMetaCb = [&](ProtocolHandler& prtotocolHandler, const std::string& method, const nlohmann::json& params)
        {
            PackageInformation packageInformation;
            packageInformation.transportType = TYPE_METAINFORMATION;
            packageInformation.signalNumber = 0;
            packageInformation.method = method;
            packageInformation.params = params;
            receivedMetaInformation.push_back(packageInformation);
        };

        signalContainer.setSignalMetaCb(signalMetaCb);
        signalContainer.setDataAsRawCb(dataAsRawCb);
        auto protocolHandler = std::make_shared<ProtocolHandler>(ioc, signalContainer, streamMetaCb, logCallback);
        protocolHandler->start(std::move(fileStream));
        ioc.run();

        // expected package sequence:
        // 0: "apiVerison"
        // 1: "init"
        // 2: "available"
        // 3: "subscribe" (time, since data signal relates to time, time has to come first!)
        // 4: "signal" (time) initial signal description
        // 5: "subscribe" (data)
        // 6: "signal" (data) initial signal description
        // 7: measured data (small)
        // 8: measured data (big)
        // 9: "signal" (data) because of updated unit
        // 10: "signal" (time) because of updated output rate
        // 11: "unsubscribe" (data)
        // 12: "unsubscribe" (time)
        // 13: "unavailable"
        ASSERT_EQ(receivedMetaInformation[0].method, META_METHOD_APIVERSION);
        ASSERT_EQ(receivedMetaInformation[1].method, META_METHOD_INIT);
        ASSERT_EQ(receivedMetaInformation[2].method, META_METHOD_AVAILABLE); // data + time


        std::string signalIdTime;
        std::string signalIdData;
        std::string tableIdTime;
        std::string tableIdData;
        RelatedSignals relatedSignals;

        PackageInformation& package = receivedMetaInformation[3];
        ASSERT_EQ(package.method, META_METHOD_SUBSCRIBE); // time signal
        signalIdTime = package.params[META_SIGNALID];

        package = receivedMetaInformation[4];
        ASSERT_EQ(package.method, META_METHOD_SIGNAL); // time
        tableIdTime = package.params[META_TABLEID];

        uint64_t outputRateInTicks = package.params[META_DEFINITION][META_RULETYPE_LINEAR][META_DELTA];
        relatedSignals = package.relatedSignals;
        ASSERT_TRUE(relatedSignals.empty());
        ASSERT_EQ(outputRateInTicks, originalOutputRateInTicks);


        package = receivedMetaInformation[5];
        ASSERT_EQ(package.method, META_METHOD_SUBSCRIBE); // data signal
        signalIdData = package.params[META_SIGNALID];
        ASSERT_FALSE(signalIdData.empty());

        package = receivedMetaInformation[6];
        ASSERT_EQ(package.method, META_METHOD_SIGNAL);
        tableIdData = package.params[META_TABLEID];
        ASSERT_EQ(tableIdTime, tableIdData);

        Unit unit;
        unit.parse(package.params[META_DEFINITION]);
        ASSERT_EQ(unit.id, originalUnit.id);
        ASSERT_EQ(unit.displayName, originalUnit.displayName);
        Range resultingRange;
        resultingRange.parse(package.params[META_DEFINITION]);
        // Range is not set => default
        ASSERT_EQ(Range(), resultingRange);
        PostScaling resultingPostScaling;
        resultingPostScaling.parse(package.params[META_DEFINITION]);
        // Post scaling is not set => default
        ASSERT_EQ(PostScaling(), resultingPostScaling);

        // there should be one relation to the time signal
        relatedSignals = package.relatedSignals;
        ASSERT_EQ(relatedSignals.size(), 1);
        ASSERT_EQ(relatedSignals[META_TIME], signalIdTime);

        package = receivedMetaInformation[7];
        ASSERT_EQ(package.transportType, TYPE_SIGNALDATA);
        int result = memcmp(package.data.data(), testData1Small.data(), package.data.size());
        ASSERT_EQ(result, 0);

        package = receivedMetaInformation[8];
        ASSERT_EQ(package.transportType, TYPE_SIGNALDATA);
        result = memcmp(package.data.data(), testData2Big.data(), package.data.size());
        ASSERT_EQ(result, 0);

        package = receivedMetaInformation[9];
        std::cout << package.params << std::endl;
        ASSERT_EQ(package.method, META_METHOD_SIGNAL); // data
        unit.parse(package.params[META_DEFINITION]);
        ASSERT_EQ(unit, newUnit);
        resultingRange.parse(package.params[META_DEFINITION]);
        ASSERT_EQ(range, resultingRange);
        resultingPostScaling.parse(package.params[META_DEFINITION]);
        ASSERT_EQ(postScaling, resultingPostScaling);

        package = receivedMetaInformation[10];
        ASSERT_EQ(package.method, META_METHOD_SIGNAL); // time
        outputRateInTicks = package.params[META_DEFINITION][META_RULETYPE_LINEAR][META_DELTA];
        ASSERT_EQ(outputRateInTicks, newOutputRateInTicks);

        ASSERT_EQ(receivedMetaInformation[11].method, META_METHOD_UNSUBSCRIBE); // data
        ASSERT_EQ(receivedMetaInformation[12].method, META_METHOD_UNSUBSCRIBE); // time
        ASSERT_EQ(receivedMetaInformation[13].method, META_METHOD_UNAVAILABLE); // data + time
        ASSERT_EQ(receivedMetaInformation.size(), 14);
    }
}

TEST(ProducerSessionTest, session_signals)
{
    static const std::string fileName = "theFile";
    static const std::string tableId = "the table Id";
    static const std::string timeSignalId = "the time";
    static const std::string signalId1 = "the 1st Id";
    static const std::string signalId2 = "the 2nd Id";
    static const std::string signalId3 = "the 3rd Id";
    std::chrono::nanoseconds outputRate = std::chrono::milliseconds(1); // 1kHz

    uint64_t timeTicksPerSecond = 1000000000;

    boost::asio::io_context ioc;
    auto fileStream = std::make_shared<stream::FileStream>(ioc, fileName, true);
    auto producerSession = std::make_shared<ProducerSession>(fileStream, nlohmann::json(), logCallback);
    StreamWriter writer(fileStream);

    auto timeSignal = std::make_shared<LinearTimeSignal>(timeSignalId, tableId, timeTicksPerSecond, outputRate, writer, logCallback);
    auto syncSignal1 = std::make_shared<SynchronousSignal<double>>(signalId1, tableId, writer, logCallback);
    auto syncSignal2 = std::make_shared<SynchronousSignal<double>>(signalId2, tableId, writer, logCallback);
    auto syncSignal3 = std::make_shared<SynchronousSignal<double>>(signalId3, tableId, writer, logCallback);

    ASSERT_NE(syncSignal1->getNumber(), syncSignal2->getNumber());

    // adding a signal causes an "available" meta information to be written
    producerSession->addSignal(syncSignal1);

    ProducerSession::Signals allSignals;
    allSignals[signalId2] = syncSignal2;
    allSignals[signalId3] = syncSignal3;
    producerSession->addSignals(allSignals);

    SignalIds allSignalIds;
    allSignalIds.push_back(signalId1);
    allSignalIds.push_back(signalId2);
    allSignalIds.push_back(signalId3);
    // causes subscribe ack and complete signal describtion to be written
    size_t count = producerSession->subscribeSignals(allSignalIds);
    ASSERT_EQ(count, allSignalIds.size());

    // causes unsubscribe ack to be written
    count = producerSession->unsubscribeSignals(allSignalIds);
    ASSERT_EQ(count, allSignalIds.size());

    // removing a signal causes an "unavailable" meta information to be written
    producerSession->removeSignal(syncSignal1->getId());

    // one was removed already!
    count = producerSession->removeSignals(allSignalIds);
    ASSERT_EQ(count, allSignalIds.size()-1);
}
}
