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

#include <chrono>
#include <gtest/gtest.h>

#include "stream/FileStream.hpp"

#include "streaming_protocol/AsynchronousSignal.hpp"
#include "streaming_protocol/StreamWriter.h"
#include "streaming_protocol/SynchronousSignal.hpp"
#include "streaming_protocol/Unit.hpp"

#include "streaming_protocol/Logging.hpp"

namespace daq::streaming_protocol {

static const uint64_t s_timeTicksPerSecond = 1000000000;
static LogCallback logCallback = daq::streaming_protocol::Logging::logCallback();


class TestSubscribeWriter : public iWriter {
public:
    TestSubscribeWriter()
        : unitId(Unit::UNIT_ID_NONE)
        , timeRule(RULETYPE_UNKNOWN)
        , timeDelta(0)
        , timeTicksPerSecond(0)
    {
    }

    /// \param signalNumber 0: for stream related, >0: signal related
    /// \note stream related meta information is about the complete session or the device
    /// \note signal related meta information is about this specific signal
    int writeMetaInformation(unsigned int signalNumber, const nlohmann::json &data) override
    {
        std::string method = data[METHOD];
        if (method==META_METHOD_SIGNAL) {

            if (data[PARAMS][META_DEFINITION].contains(META_UNIT)) {
                unitId = data[PARAMS][META_DEFINITION][META_UNIT][META_UNIT_ID];
                unitDisplayName = data[PARAMS][META_DEFINITION][META_UNIT][META_DISPLAY_NAME];
                if (data[PARAMS][META_DEFINITION][META_UNIT].contains(META_QUANTITY)) {
                    unitQuantity = data[PARAMS][META_DEFINITION][META_UNIT][META_QUANTITY];
                }

            } else {
                unitId = Unit::UNIT_ID_NONE;
                unitDisplayName.clear();
                unitQuantity.clear();
            }

            if(unitQuantity == META_TIME ) {
                std::string ruleAsString = data[PARAMS][META_DEFINITION][META_RULE];
                if (ruleAsString==META_RULETYPE_EXPLICIT) {
                    timeRule = RULETYPE_EXPLICIT;
                } else if (ruleAsString==META_RULETYPE_LINEAR) {
                    timeRule = RULETYPE_LINEAR;
                    timeDelta = data[PARAMS][META_DEFINITION][META_RULETYPE_LINEAR][META_DELTA];
                } else if (ruleAsString==META_RULETYPE_CONSTANT) {
                    timeRule = RULETYPE_CONSTANT;
                }
                auto const interpretationObjectIter = data[PARAMS].find(META_INTERPRETATION);
                if (interpretationObjectIter!=data[PARAMS].end()) {
                    timeInterpretationObject = *interpretationObjectIter;
                }
                epoch = data[PARAMS][META_DEFINITION][META_ABSOLUTE_REFERENCE];
                timeTicksPerSecond = data[PARAMS][META_DEFINITION][META_RESOLUTION][META_DENOMINATOR];
            } else {
                dataType = data[PARAMS][META_DEFINITION][META_DATATYPE];
                auto const interpretationObjectIter = data[PARAMS].find(META_INTERPRETATION);
                if (interpretationObjectIter!=data[PARAMS].end()) {
                    dataInterpretationObject = *interpretationObjectIter;
                }
            }
        }
        return 0;
    }
    /// \param id 0: for stream related, >0: signal related
    int writeSignalData(unsigned int signalNumber, const void *pData, size_t length) override
    {
        return 0;
    }

    std::string id() const override
    {
        return "";
    }

    std::string dataType;
    int32_t unitId;
    std::string unitDisplayName;
    std::string unitQuantity;

    RuleType timeRule;
    uint64_t timeDelta;
    std::string epoch;
    uint64_t timeTicksPerSecond;
    nlohmann::json dataInterpretationObject;
    nlohmann::json timeInterpretationObject;
};


/// \todo To be enabled after async signal is working again
//TEST(SignalTest, async_signalid_test)
//{
//    static const std::string signalId = "the Id";
//    static const std::string valueName = "value name";

//    static const int32_t unitId = Unit::UNIT_ID_SECONDS;
//    static const std::string unitDisplayName = "s";
//    static const nlohmann::json dataInterpretationObject = R"(
//  {
//    "pi": 3.141,
//    "happy": true
//  }
//    )"_json;

//    static const nlohmann::json timeInterpretationObject = R"(
//  {
//    "date": "2023-03-01"
//  }
//    )"_json;

//    TestSubscribeWriter writer;
//    AsynchronousSignal<double> asyncSignal(signalId, s_timeTicksPerSecond, writer, logCallback);
//    ASSERT_EQ(asyncSignal.getUnitId(), Unit::UNIT_ID_NONE);
//    asyncSignal.setUnit(unitId, unitDisplayName);
//    asyncSignal.setMemberName(valueName);
//    ASSERT_EQ(asyncSignal.getDataInterpretationObject(), nlohmann::json());
//    ASSERT_EQ(asyncSignal.getTimeInterpretationObject(), nlohmann::json());
//    asyncSignal.setDataInterpretationObject(dataInterpretationObject);
//    asyncSignal.setTimeInterpretationObject(timeInterpretationObject);
//    ASSERT_EQ(asyncSignal.getDataInterpretationObject(), dataInterpretationObject);
//    ASSERT_EQ(asyncSignal.getTimeInterpretationObject(), timeInterpretationObject);

//    ASSERT_EQ(asyncSignal.getTimeRule(), RULETYPE_EXPLICIT);
//    ASSERT_EQ(asyncSignal.getSampleType(), SAMPLETYPE_REAL64);
//    ASSERT_EQ(asyncSignal.getId(), signalId);
//    ASSERT_EQ(asyncSignal.getUnitId(), unitId);
//    ASSERT_EQ(asyncSignal.getUnitDisplayName(), unitDisplayName);
//    ASSERT_EQ(asyncSignal.getMemberName(), valueName);
//    ASSERT_EQ(asyncSignal.getTimeTicksPerSecond(), s_timeTicksPerSecond);

//    asyncSignal.subscribe(); // causes subscribe ack and all signal meta information to be written
//    ASSERT_EQ(writer.dataType, DATA_TYPE_REAL64);
//    ASSERT_EQ(writer.unitId, unitId);
//    ASSERT_EQ(writer.unitDisplayName, unitDisplayName);
//    ASSERT_EQ(writer.timeRule, RULETYPE_EXPLICIT);
//    ASSERT_EQ(writer.timeTicksPerSecond, s_timeTicksPerSecond);
//    ASSERT_EQ(writer.dataInterpretationObject, dataInterpretationObject);
//    ASSERT_EQ(writer.timeInterpretationObject, timeInterpretationObject);
//}

//TEST(SignalTest, async_sampletype_test)
//{
//    static const std::string signalId = "the Id";

//    TestSubscribeWriter writer;
//    {
//        AsynchronousSignal<int8_t> asyncSignal(signalId, s_timeTicksPerSecond, writer, logCallback);
//        ASSERT_EQ(asyncSignal.getSampleType(), SAMPLETYPE_S8);
//        asyncSignal.subscribe();
//        ASSERT_EQ(writer.dataType, DATA_TYPE_INT8);
//    }
//    {
//        AsynchronousSignal<int16_t> asyncSignal(signalId, s_timeTicksPerSecond, writer, logCallback);
//        ASSERT_EQ(asyncSignal.getSampleType(), SAMPLETYPE_S16);
//        asyncSignal.subscribe();
//        ASSERT_EQ(writer.dataType, DATA_TYPE_INT16);
//    }
//    {
//        AsynchronousSignal<int32_t> asyncSignal(signalId, s_timeTicksPerSecond, writer, logCallback);
//        ASSERT_EQ(asyncSignal.getSampleType(), SAMPLETYPE_S32);
//        asyncSignal.subscribe();
//        ASSERT_EQ(writer.dataType, DATA_TYPE_INT32);
//    }
//    {
//        AsynchronousSignal<int64_t> asyncSignal(signalId, s_timeTicksPerSecond, writer, logCallback);
//        ASSERT_EQ(asyncSignal.getSampleType(), SAMPLETYPE_S64);
//        asyncSignal.subscribe();
//        ASSERT_EQ(writer.dataType, DATA_TYPE_INT64);
//    }
//    {
//        AsynchronousSignal<uint8_t> asyncSignal(signalId, s_timeTicksPerSecond, writer, logCallback);
//        ASSERT_EQ(asyncSignal.getSampleType(), SAMPLETYPE_U8);
//        asyncSignal.subscribe();
//        ASSERT_EQ(writer.dataType, DATA_TYPE_UINT8);
//    }
//    {
//        AsynchronousSignal<uint16_t> asyncSignal(signalId, s_timeTicksPerSecond, writer, logCallback);
//        ASSERT_EQ(asyncSignal.getSampleType(), SAMPLETYPE_U16);
//        asyncSignal.subscribe();
//        ASSERT_EQ(writer.dataType, DATA_TYPE_UINT16);
//    }
//    {
//        AsynchronousSignal<uint32_t> asyncSignal(signalId, s_timeTicksPerSecond, writer, logCallback);
//        ASSERT_EQ(asyncSignal.getSampleType(), SAMPLETYPE_U32);
//        asyncSignal.subscribe();
//        ASSERT_EQ(writer.dataType, DATA_TYPE_UINT32);
//    }
//    {
//        AsynchronousSignal<uint64_t> asyncSignal(signalId, s_timeTicksPerSecond, writer, logCallback);
//        ASSERT_EQ(asyncSignal.getSampleType(), SAMPLETYPE_U64);
//        asyncSignal.subscribe();
//        ASSERT_EQ(writer.dataType, DATA_TYPE_UINT64);
//    }
//    {
//        AsynchronousSignal<float> asyncSignal(signalId, s_timeTicksPerSecond, writer, logCallback);
//        ASSERT_EQ(asyncSignal.getSampleType(), SAMPLETYPE_REAL32);
//        asyncSignal.subscribe();
//        ASSERT_EQ(writer.dataType, DATA_TYPE_REAL32);
//    }
//    {
//        AsynchronousSignal<double> asyncSignal(signalId, s_timeTicksPerSecond, writer, logCallback);
//        ASSERT_EQ(asyncSignal.getSampleType(), SAMPLETYPE_REAL64);
//        asyncSignal.subscribe();
//        ASSERT_EQ(writer.dataType, DATA_TYPE_REAL64);
//    }
//    {
//        AsynchronousSignal<Complex32Type> asyncSignal(signalId, s_timeTicksPerSecond, writer, logCallback);
//        ASSERT_EQ(asyncSignal.getSampleType(), SAMPLETYPE_COMPLEX32);
//        asyncSignal.subscribe();
//        ASSERT_EQ(writer.dataType, DATA_TYPE_COMPLEX32);
//    }
//    {
//        AsynchronousSignal<Complex64Type> asyncSignal(signalId, s_timeTicksPerSecond, writer, logCallback);
//        ASSERT_EQ(asyncSignal.getSampleType(), SAMPLETYPE_COMPLEX64);
//        asyncSignal.subscribe();
//        ASSERT_EQ(writer.dataType, DATA_TYPE_COMPLEX64);
//    }
//}

TEST(SignalTest, sync_sampletype_test)
{
    static const std::string signalId = "the Id";

    TestSubscribeWriter writer;
    {
        SynchronousSignal<int8_t> syncSignal(signalId, s_timeTicksPerSecond/10, s_timeTicksPerSecond, writer, logCallback);
        ASSERT_EQ(syncSignal.getSampleType(), SAMPLETYPE_S8);
        syncSignal.subscribe();
        ASSERT_EQ(writer.dataType, DATA_TYPE_INT8);
    }
    {
        SynchronousSignal<int16_t> syncSignal(signalId, s_timeTicksPerSecond/10, s_timeTicksPerSecond, writer, logCallback);
        ASSERT_EQ(syncSignal.getSampleType(), SAMPLETYPE_S16);
        syncSignal.subscribe();
        ASSERT_EQ(writer.dataType, DATA_TYPE_INT16);
    }
    {
        SynchronousSignal<int32_t> syncSignal(signalId, s_timeTicksPerSecond/10, s_timeTicksPerSecond, writer, logCallback);
        ASSERT_EQ(syncSignal.getSampleType(), SAMPLETYPE_S32);
        syncSignal.subscribe();
        ASSERT_EQ(writer.dataType, DATA_TYPE_INT32);
    }
    {
        SynchronousSignal<int64_t> syncSignal(signalId, s_timeTicksPerSecond/10, s_timeTicksPerSecond, writer, logCallback);
        ASSERT_EQ(syncSignal.getSampleType(), SAMPLETYPE_S64);
        syncSignal.subscribe();
        ASSERT_EQ(writer.dataType, DATA_TYPE_INT64);
    }
    {
        SynchronousSignal<uint8_t> syncSignal(signalId, s_timeTicksPerSecond/10, s_timeTicksPerSecond, writer, logCallback);
        ASSERT_EQ(syncSignal.getSampleType(), SAMPLETYPE_U8);
        syncSignal.subscribe();
        ASSERT_EQ(writer.dataType, DATA_TYPE_UINT8);
    }
    {
        SynchronousSignal<uint16_t> syncSignal(signalId, s_timeTicksPerSecond/10, s_timeTicksPerSecond, writer, logCallback);
        ASSERT_EQ(syncSignal.getSampleType(), SAMPLETYPE_U16);
        syncSignal.subscribe();
        ASSERT_EQ(writer.dataType, DATA_TYPE_UINT16);
    }
    {
        SynchronousSignal<uint32_t> syncSignal(signalId, s_timeTicksPerSecond/10, s_timeTicksPerSecond, writer, logCallback);
        ASSERT_EQ(syncSignal.getSampleType(), SAMPLETYPE_U32);
        syncSignal.subscribe();
        ASSERT_EQ(writer.dataType, DATA_TYPE_UINT32);
    }
    {
        SynchronousSignal<uint64_t> syncSignal(signalId, s_timeTicksPerSecond/10, s_timeTicksPerSecond, writer, logCallback);
        ASSERT_EQ(syncSignal.getSampleType(), SAMPLETYPE_U64);
        syncSignal.subscribe();
        ASSERT_EQ(writer.dataType, DATA_TYPE_UINT64);
    }
    {
        SynchronousSignal<float> syncSignal(signalId, s_timeTicksPerSecond/10, s_timeTicksPerSecond, writer, logCallback);
        ASSERT_EQ(syncSignal.getSampleType(), SAMPLETYPE_REAL32);
        syncSignal.subscribe();
        ASSERT_EQ(writer.dataType, DATA_TYPE_REAL32);
    }
    {
        SynchronousSignal<double> syncSignal(signalId, s_timeTicksPerSecond/10, s_timeTicksPerSecond, writer, logCallback);
        ASSERT_EQ(syncSignal.getSampleType(), SAMPLETYPE_REAL64);
        syncSignal.subscribe();
        ASSERT_EQ(writer.dataType, DATA_TYPE_REAL64);
    }
    {
        SynchronousSignal<Complex32Type> syncSignal(signalId, s_timeTicksPerSecond/10, s_timeTicksPerSecond, writer, logCallback);
        ASSERT_EQ(syncSignal.getSampleType(), SAMPLETYPE_COMPLEX32);
        syncSignal.subscribe();
        ASSERT_EQ(writer.dataType, DATA_TYPE_COMPLEX32);
    }
    {
        SynchronousSignal<Complex64Type> syncSignal(signalId, s_timeTicksPerSecond/10, s_timeTicksPerSecond, writer, logCallback);
        ASSERT_EQ(syncSignal.getSampleType(), SAMPLETYPE_COMPLEX64);
        syncSignal.subscribe();
        ASSERT_EQ(writer.dataType, DATA_TYPE_COMPLEX64);
    }
}


TEST(SignalTest, sync_signalid_test)
{
    std::string signalId = "the Id";
    std::chrono::nanoseconds outputRate = std::chrono::milliseconds(1); // 1kHz
    int32_t unitId = Unit::UNIT_ID_SECONDS;
    std::string unitDisplayName = "s";
    static const nlohmann::json dataInterpretationObject = R"(
  {
    "pi": 3.141,
    "happy": true
  }
    )"_json;

    static const nlohmann::json timeInterpretationObject = R"(
  {
    "date": "2023-03-01"
  }
    )"_json;

    TestSubscribeWriter writer;

    uint64_t outputRateInTicks = BaseSignal::timeTicksFromNanoseconds(outputRate, s_timeTicksPerSecond);
    SynchronousSignal<double> syncSignal(signalId, outputRateInTicks, s_timeTicksPerSecond, writer, logCallback);
    ASSERT_EQ(syncSignal.getTimeRule(), RULETYPE_LINEAR);

    ASSERT_EQ(syncSignal.getUnitId(), Unit::UNIT_ID_NONE);
    ASSERT_EQ(syncSignal.getId(), signalId);

    ASSERT_EQ(syncSignal.getTimeDelta(), outputRateInTicks);
    ASSERT_EQ(syncSignal.getSampleType(), SAMPLETYPE_REAL64);

    syncSignal.setUnit(unitId, unitDisplayName);
    ASSERT_EQ(syncSignal.getUnitId(), unitId);
    ASSERT_EQ(syncSignal.getUnitDisplayName(), unitDisplayName);

    ASSERT_EQ(syncSignal.getDataInterpretationObject(), nlohmann::json());
    ASSERT_EQ(syncSignal.getTimeInterpretationObject(), nlohmann::json());
    syncSignal.setDataInterpretationObject(dataInterpretationObject);
    syncSignal.setTimeInterpretationObject(timeInterpretationObject);
    ASSERT_EQ(syncSignal.getDataInterpretationObject(), dataInterpretationObject);
    ASSERT_EQ(syncSignal.getTimeInterpretationObject(), timeInterpretationObject);

    syncSignal.subscribe(); // causes subscribe ack and all signal meta information to be written to fileName
    ASSERT_EQ(writer.unitId, unitId);
    ASSERT_EQ(writer.unitDisplayName, unitDisplayName);
    ASSERT_EQ(writer.timeRule, RULETYPE_LINEAR);
    ASSERT_EQ(writer.timeDelta, outputRateInTicks);
    ASSERT_EQ(writer.timeTicksPerSecond, s_timeTicksPerSecond);
    ASSERT_EQ(writer.dataInterpretationObject, dataInterpretationObject);
    ASSERT_EQ(writer.timeInterpretationObject, timeInterpretationObject);
}

TEST(SignalTest, sync_time_test)
{
    std::string fileName = "theFile";
    std::string signalId = "the Id";
    std::chrono::nanoseconds outputRate = std::chrono::milliseconds(1); // 1kHz
    std::string unitDisplayName = "s";

    TestSubscribeWriter writer;

    uint64_t outputRateInTicks = BaseSignal::timeTicksFromNanoseconds(outputRate, s_timeTicksPerSecond);
    SynchronousSignal<float> syncSignal(signalId, outputRateInTicks, s_timeTicksPerSecond, writer, logCallback);

    ASSERT_EQ(syncSignal.getSampleType(), SAMPLETYPE_REAL32);


    /// ISO 8601:2004 format:
    /// -for date and utc time: YYYY-MM-DDThh:mm:ssZ
    /// -for date: YYYY-MM-DD
    std::string epoch = syncSignal.getEpoch();
    ASSERT_EQ(epoch, UNIX_EPOCH);

    auto currentUtcTime = std::chrono::system_clock::now();

    std::time_t tt = std::chrono::system_clock::to_time_t(currentUtcTime);
    std::tm tm = *std::gmtime(&tt); //GMT (UTC)
    std::stringstream currentUtcTimeAsString;
    currentUtcTimeAsString << std::put_time( &tm, "%Y-%m-%dT%H:%M:%SZ");
    syncSignal.setEpoch(currentUtcTime);
    epoch = syncSignal.getEpoch();
    ASSERT_EQ(epoch, currentUtcTimeAsString.str());
    syncSignal.writeSignalMetaInformation();
    ASSERT_EQ(writer.epoch, epoch);

    syncSignal.setEpoch(UNIX_EPOCH);
    epoch = syncSignal.getEpoch();
    ASSERT_EQ(epoch, UNIX_EPOCH);
    syncSignal.writeSignalMetaInformation();
    ASSERT_EQ(writer.epoch, UNIX_EPOCH);

    auto startTime = std::chrono::system_clock::now();
    uint64_t startTimeInTicks = BaseSignal::timeTicksFromTime(startTime, s_timeTicksPerSecond);
    syncSignal.setTimeStart(startTimeInTicks);
    ASSERT_EQ(startTimeInTicks, syncSignal.getTimeStart());
}

TEST(SignalTest, sync_outputrate_test)
{
    std::string fileName = "theFile";
    std::string signalId = "the Id";
    std::chrono::nanoseconds outputRate = std::chrono::milliseconds(1); // 1kHz

    boost::asio::io_context ioc;
    auto fileStream = std::make_shared<stream::FileStream>(ioc, fileName, true);
    StreamWriter writer(fileStream);

    uint64_t outputRateInTicks = BaseSignal::timeTicksFromNanoseconds(outputRate, s_timeTicksPerSecond);
    SynchronousSignal<int32_t> syncSignal(signalId, outputRateInTicks, s_timeTicksPerSecond, writer, logCallback);
    ASSERT_EQ(syncSignal.getTimeDelta(), outputRateInTicks);
    ASSERT_EQ(syncSignal.getSampleType(), SAMPLETYPE_S32);

    outputRate = std::chrono::seconds(1); // 1Hz
    outputRateInTicks = BaseSignal::timeTicksFromNanoseconds(outputRate, s_timeTicksPerSecond);
    syncSignal.setOutputRate(outputRateInTicks);
    ASSERT_EQ(syncSignal.getTimeDelta(), outputRateInTicks);
}

/// time ticks run with 1GHz in this test
TEST(SignalTest, sync_ticks_to_nanoseconds_test)
{
    std::string fileName = "theFile";
    std::string signalId = "the Id";
    std::chrono::nanoseconds outputRate = std::chrono::milliseconds(1); // 1kHz

    boost::asio::io_context ioc;
    auto fileStream = std::make_shared<stream::FileStream>(ioc, fileName, true);
    StreamWriter writer(fileStream);

    uint64_t outputRateInTicks = BaseSignal::timeTicksFromNanoseconds(outputRate, s_timeTicksPerSecond);
    SynchronousSignal<int32_t> syncSignal(signalId, outputRateInTicks, s_timeTicksPerSecond, writer, logCallback);

    uint64_t timeTicks;
    std::chrono::nanoseconds ns;
    std::chrono::nanoseconds nsRequested;

    nsRequested = std::chrono::nanoseconds(17);
    timeTicks = syncSignal.timeTicksFromNanoseconds(nsRequested, s_timeTicksPerSecond);
    ASSERT_EQ(timeTicks, 17);
    ns = syncSignal.nanosecondsFromTimeTicks(timeTicks, s_timeTicksPerSecond);
    ASSERT_EQ(ns,nsRequested);

    nsRequested = std::chrono::microseconds(1);
    timeTicks = syncSignal.timeTicksFromNanoseconds(nsRequested, s_timeTicksPerSecond);
    ASSERT_EQ(timeTicks, s_timeTicksPerSecond/1000000);
    ns = syncSignal.nanosecondsFromTimeTicks(timeTicks, s_timeTicksPerSecond);
    ASSERT_EQ(ns,nsRequested);

    nsRequested = std::chrono::milliseconds(500);
    timeTicks = syncSignal.timeTicksFromNanoseconds(nsRequested, s_timeTicksPerSecond);
    ASSERT_EQ(timeTicks, s_timeTicksPerSecond/2);
    ns = syncSignal.nanosecondsFromTimeTicks(timeTicks, s_timeTicksPerSecond);
    ASSERT_EQ(ns,nsRequested);

    nsRequested = std::chrono::hours(1);
    timeTicks = syncSignal.timeTicksFromNanoseconds(nsRequested, s_timeTicksPerSecond);
    ASSERT_EQ(timeTicks, s_timeTicksPerSecond*3600);
    ns = syncSignal.nanosecondsFromTimeTicks(timeTicks, s_timeTicksPerSecond);
    ASSERT_EQ(ns, nsRequested);

    // one year
    nsRequested = std::chrono::hours(8760);
    timeTicks = syncSignal.timeTicksFromNanoseconds(nsRequested, s_timeTicksPerSecond);
    ASSERT_EQ(timeTicks, s_timeTicksPerSecond*3600*8760);
    ns = syncSignal.nanosecondsFromTimeTicks(timeTicks, s_timeTicksPerSecond);
    ASSERT_EQ(ns, nsRequested);

    // one thousand years
    nsRequested = std::chrono::hours(8760*1000);
    timeTicks = syncSignal.timeTicksFromNanoseconds(nsRequested, s_timeTicksPerSecond);
    ASSERT_EQ(timeTicks, s_timeTicksPerSecond*3600*8760*1000);
    ns = syncSignal.nanosecondsFromTimeTicks(timeTicks, s_timeTicksPerSecond);
    ASSERT_EQ(ns, nsRequested);
}


static void checkTime(const std::chrono::nanoseconds& timeRequested, const BaseSynchronousSignal& syncSignal)
{
    // defaults to unix epoch
    std::chrono::time_point<std::chrono::system_clock> time;
    std::chrono::time_point<std::chrono::system_clock> timeResult;

    uint64_t timeTicks;
    uint64_t timeTicksRequested;

    // add to unix epoch
#ifdef TIME_GRANULARITY_NS
    time += timeRequested;
#else
    std::chrono::microseconds microseconds = std::chrono::duration_cast<std::chrono::microseconds>(timeRequested);
    time += microseconds;
#endif
    timeTicks = syncSignal.timeTicksFromTime(time, s_timeTicksPerSecond);
    timeTicksRequested = syncSignal.timeTicksFromNanoseconds(timeRequested, s_timeTicksPerSecond);
    ASSERT_EQ(timeTicks, timeTicksRequested);

    // back from time time ticks to time
    timeResult = syncSignal.timeFromTimeTicks(timeTicksRequested, s_timeTicksPerSecond);
    ASSERT_EQ(time, timeResult);
}

TEST(SignalTest, sync_ticks_to_time_test)
{
    std::string fileName = "theFile";
    std::string signalId = "the Id";
    std::chrono::nanoseconds outputRate = std::chrono::milliseconds(1); // 1kHz

    boost::asio::io_context ioc;
    auto fileStream = std::make_shared<stream::FileStream>(ioc, fileName, true);
    StreamWriter writer(fileStream);

    uint64_t outputRateInTicks = BaseSignal::timeTicksFromNanoseconds(outputRate, s_timeTicksPerSecond);
    SynchronousSignal<int32_t> syncSignal(signalId, outputRateInTicks, s_timeTicksPerSecond, writer, logCallback);

    std::chrono::nanoseconds timeRequested;

    // resolution of std::chrono::system_clock on this implementation.
    // calculates the ticks per second:
    uint64_t chronoSystemClockTicksPerSecond = std::chrono::system_clock::period::den / std::chrono::system_clock::period::num;
    std::cout << "std::chrono::system_clock ticks per second: " << chronoSystemClockTicksPerSecond << std::endl;
    // Test under Linux complied with g++ gives 1 000 000 000, which allows resolution of 1ns
    // Test under Windows complied with MSVC 2019 gives 10 000 000, which allows resolution of 100ns


#ifdef TIME_GRANULARITY_NS
    timeRequested = std::chrono::nanoseconds(17);
    checkTime(timeRequested, syncSignal);
#endif

    timeRequested = std::chrono::microseconds(5);
    checkTime(timeRequested, syncSignal);

    timeRequested = std::chrono::milliseconds(500);
    checkTime(timeRequested, syncSignal);

    timeRequested = std::chrono::seconds(1);
    checkTime(timeRequested, syncSignal);

    timeRequested = std::chrono::hours(1);
    checkTime(timeRequested, syncSignal);

    // one year
    timeRequested = std::chrono::hours(8760);
    checkTime(timeRequested, syncSignal);

    // one thousand years
    timeRequested = std::chrono::hours(8760*1000);
    checkTime(timeRequested, syncSignal);

    // two thousand years
    timeRequested = std::chrono::hours(8760*2000);
    checkTime(timeRequested, syncSignal);

#ifdef TIME_GRANULARITY_NS
    // ten thousand years
    timeRequested = std::chrono::hours(8760*10000);
    checkTime(timeRequested, syncSignal);
#endif
}
}
