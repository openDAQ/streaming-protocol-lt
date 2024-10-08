#include <gtest/gtest.h>

#include "nlohmann/json.hpp"

#include "streaming_protocol/Defines.h"
#include "streaming_protocol/SubscribedSignal.hpp"
#include "streaming_protocol/Types.h"
#include "streaming_protocol/Logging.hpp"



namespace daq::streaming_protocol {
    static LogCallback logCallback = daq::streaming_protocol::Logging::logCallback();

    TEST(SubscribedSignalTest, signalid_test)
    {
        int result;
        SignalNumber signalNumber = 9;

        {
            SubscribedSignal dataSignal(signalNumber, logCallback);
            ASSERT_EQ(dataSignal.signalId(), "");
            ASSERT_EQ(dataSignal.isTimeSignal(), false);
            ASSERT_EQ(dataSignal.ruleType(),RULETYPE_EXPLICIT);
        }

        {
            auto timeSignal = std::make_shared<SubscribedSignal>(1, logCallback);
            SubscribedSignal dataSignal(2, logCallback);
            dataSignal.setTimeSignal(timeSignal);
            ASSERT_EQ(dataSignal.timeSignal(), timeSignal);
        }

        {
            // signal id as number
            nlohmann::json metaSubscribe;
            int signalIdAsNumber = -8;
            metaSubscribe[META_SIGNALID] = signalIdAsNumber;

            SubscribedSignal dataSignal(signalNumber, logCallback);
            result = dataSignal.processSignalMetaInformation(META_METHOD_SUBSCRIBE, metaSubscribe);
            ASSERT_EQ(result, 0);
            ASSERT_EQ(dataSignal.signalId(), std::to_string(signalIdAsNumber));
        }

        {
            // signal id as string
            nlohmann::json metaSubscribe;
            std::string signalId = "da id!";
            metaSubscribe[META_SIGNALID] = signalId;

            SubscribedSignal dataSignal(signalNumber, logCallback);
            result = dataSignal.processSignalMetaInformation(META_METHOD_SUBSCRIBE, metaSubscribe);

            ASSERT_EQ(result, 0);
            ASSERT_EQ(dataSignal.signalId(), signalId);
        }

        {
            // invalid, signal id may not be an object
            nlohmann::json metaSubscribe;
            SubscribedSignal dataSignal(signalNumber, logCallback);


            std::string signalId = "da id!";
            metaSubscribe[META_SIGNALID]["bla"] = signalId;

            result = dataSignal.processSignalMetaInformation(META_METHOD_SUBSCRIBE, metaSubscribe);
            ASSERT_EQ(result, -1);
            ASSERT_EQ(dataSignal.signalId(), "");
        }

        {
            // invalid, signal id is empty
            nlohmann::json metaSubscribe;
            SubscribedSignal dataSignal(signalNumber, logCallback);

            result = dataSignal.processSignalMetaInformation(META_METHOD_SUBSCRIBE, metaSubscribe);
            ASSERT_EQ(result, -1);
            ASSERT_EQ(dataSignal.signalId(), "");
        }
    }

    TEST(SubscribedSignalTest, timeSignal_test)
    {
        SignalNumber signalNumber = 9;
        static const std::string tableId = "table id";

        nlohmann::json interpretationObject = R"(
      {
        "pi": 3.141,
        "happy": true
      }
        )"_json;

        SubscribedSignal timeSignal(signalNumber, logCallback);


        static const std::string unixEpoch = "1970-01-01";
        static const uint64_t numerator = 1;
        static const uint64_t denominator = 1000000000;
        static const uint64_t timeDelta = 3;

        Unit unit;
        unit.id = Unit::UNIT_ID_SECONDS;
        unit.displayName = "s";
        unit.quantity = META_TIME;

        /// linear time rule with delta = 0 is not valid!
        nlohmann::json metaTimeSignal;
        metaTimeSignal[META_TABLEID] = tableId;
        metaTimeSignal[META_DEFINITION][META_RULE] = META_RULETYPE_LINEAR;
        metaTimeSignal[META_DEFINITION][META_RULETYPE_LINEAR][META_DELTA] = 0;
        metaTimeSignal[META_DEFINITION][META_DATATYPE] = DATA_TYPE_UINT64;
        unit.compose(metaTimeSignal[META_DEFINITION]);
        metaTimeSignal[META_DEFINITION][META_ABSOLUTE_REFERENCE] = unixEpoch;
        metaTimeSignal[META_DEFINITION][META_RESOLUTION][META_DENOMINATOR] = denominator;
        metaTimeSignal[META_DEFINITION][META_RESOLUTION][META_NUMERATOR] = numerator;

        int result = timeSignal.processSignalMetaInformation(META_METHOD_SIGNAL, metaTimeSignal);
        ASSERT_EQ(result, -1);

        // change to a valid linear rule
        metaTimeSignal[META_DEFINITION][META_RULETYPE_LINEAR][META_DELTA] = timeDelta;
        metaTimeSignal[META_INTERPRETATION] = interpretationObject;

        result = timeSignal.processSignalMetaInformation(META_METHOD_SIGNAL, metaTimeSignal);
        ASSERT_EQ(result, 0);

        ASSERT_EQ(timeSignal.unitId(), unit.id);
        ASSERT_EQ(timeSignal.unitDisplayName(), unit.displayName);
        ASSERT_EQ(timeSignal.unitQuantity(), unit.quantity);
        ASSERT_EQ(timeSignal.tableId(), tableId);
        ASSERT_EQ(timeSignal.interpretationObject(), interpretationObject);
        ASSERT_EQ(timeSignal.timeBaseFrequency(), denominator/numerator);
        ASSERT_EQ(timeSignal.timeBaseEpochAsString(), unixEpoch);
        ASSERT_EQ(timeSignal.linearDelta(), timeDelta);





















    }

    /// prepare time signal and attach it to synchronous data signal
    void static prepareSignals(SubscribedSignal& dataSignal, std::shared_ptr < SubscribedSignal> timeSignal, uint64_t startTime)
    {
        int result;

        {
            nlohmann::json metaTimeSubscribe;
            std::string timeSignalId = "da time!";
            metaTimeSubscribe[META_SIGNALID] = timeSignalId;
            result = timeSignal->processSignalMetaInformation(META_METHOD_SUBSCRIBE, metaTimeSubscribe);
            ASSERT_EQ(result, 0);
        }

        {
            uint64_t deltaTime = 1;
            nlohmann::json metaTimeSignal;
            metaTimeSignal[META_TABLEID] = dataSignal.tableId();
            metaTimeSignal[META_DEFINITION][META_RULE] = META_RULETYPE_LINEAR;
            metaTimeSignal[META_DEFINITION][META_RULETYPE_LINEAR][META_DELTA] = deltaTime;
            metaTimeSignal[META_DEFINITION][META_DATATYPE] = DATA_TYPE_UINT64;
            result = timeSignal->processSignalMetaInformation(META_METHOD_SIGNAL, metaTimeSignal);
            ASSERT_EQ(result, 0);
        }

        /// timestamp of the next value to be delivered
        timeSignal->setTime(startTime);

        {
            nlohmann::json metaDataSubscribe;
            std::string dataSignalId = "da data!";
            metaDataSubscribe[META_SIGNALID] = dataSignalId;
            result = dataSignal.processSignalMetaInformation(META_METHOD_SUBSCRIBE, metaDataSubscribe);
            ASSERT_EQ(result, 0);
        }

        {
            nlohmann::json metaDataSignal;
            metaDataSignal[META_TABLEID] = dataSignal.tableId();
            metaDataSignal[META_RELATEDSIGNALS][0][META_TYPE] = META_TIME;
            metaDataSignal[META_RELATEDSIGNALS][0][META_SIGNALID] = timeSignal->signalId();
            result = timeSignal->processSignalMetaInformation(META_METHOD_SIGNAL, metaDataSignal);
            ASSERT_EQ(result, 0);
        }
    }

    TEST(SubscribedSignalTest, dataCb_uint16_test)
    {
        SignalNumber signalNumber = 9;
        int result;
        uint64_t startTime = 1000;
        uint64_t signalDelayIndex = 10;

        auto timeSignal = std::make_shared<SubscribedSignal>(signalNumber+1, logCallback);
        SubscribedSignal dataSignal(signalNumber, logCallback);
        prepareSignals(dataSignal, timeSignal, startTime);

        nlohmann::json metaDataSignal;
        metaDataSignal[META_VALUEINDEX] = signalDelayIndex; // this signal is late!
        metaDataSignal[META_DEFINITION][META_RULE] = META_RULETYPE_EXPLICIT;
        metaDataSignal[META_DEFINITION][META_DATATYPE] = DATA_TYPE_UINT16;

        result = dataSignal.processSignalMetaInformation(META_METHOD_SIGNAL, metaDataSignal);
        ASSERT_EQ(result, 0);
        ASSERT_EQ(dataSignal.ruleType(), RULETYPE_EXPLICIT);
        ASSERT_EQ(dataSignal.dataValueType(), SAMPLETYPE_U16);

        std::array < uint16_t, 3 > expectedData = { 1, 2, 3};

        uint64_t deliveredFirstTimestamp;
        auto dataAsValueCb = [&](const SubscribedSignal& subscribedSignal, uint64_t timeStamp, const uint8_t* deliveredData, size_t valueCount)
        {
            std::vector <double> doubleData(valueCount);
            deliveredFirstTimestamp = timeStamp;
            subscribedSignal.interpretValuesAsDouble(deliveredData, valueCount, doubleData.data());
            auto dataIter = doubleData.begin();
            for (auto expectedIter : expectedData) {
                double data = *dataIter;
                double expected = expectedIter;
                ASSERT_EQ(data, expected);
                ++dataIter;
            }
            ASSERT_EQ(valueCount, sizeof(expectedData)/sizeof(expectedData[0]));
        };

        auto dataAsRawCb = [&](const SubscribedSignal& subscribedSignal, uint64_t timeStamp, const uint8_t* deliveredData, size_t byteCount)
        {
            ASSERT_EQ(byteCount, sizeof(expectedData));
            ASSERT_EQ(memcmp(deliveredData, expectedData.data(), byteCount),0);
        };

        ssize_t processResult = dataSignal.processMeasuredData(reinterpret_cast <unsigned char*> (expectedData.data()), sizeof(expectedData), timeSignal, dataAsRawCb, dataAsValueCb);
        /// check for the delay!
        ASSERT_EQ(deliveredFirstTimestamp, (startTime+signalDelayIndex*timeSignal->linearDelta()));
        ASSERT_EQ(processResult, sizeof(expectedData));

        {
            // check unit
            Unit unit;
            unit.displayName = "V";
            unit.id = 6;
            nlohmann::json metaUnit;

            /// \warning no checking of display naming vs. unit id
            unit.compose(metaUnit[META_DEFINITION]);
            result = dataSignal.processSignalMetaInformation(META_METHOD_SIGNAL, metaUnit);
            ASSERT_EQ(result, 0);
            ASSERT_EQ(dataSignal.unitDisplayName(), unit.displayName);
            ASSERT_EQ(dataSignal.unitId(), unit.id);
            ASSERT_TRUE(dataSignal.unitQuantity().empty());
        }
    }

    /// Signals not following an explicit rule while have an value index attached to each value
    TEST(SubscribedSignalTest, dataCb_uint32_with_constant_rule_test)
    {
        SignalNumber signalNumber = 9;
        int result;
        uint64_t startTime = 1000;


#pragma pack(push, 1)
        struct Uint32WithValueIndex
        {
            uint64_t valueIndex;
            uint32_t value;
        };
#pragma pack(pop)


        auto timeSignal = std::make_shared<SubscribedSignal>(signalNumber+1, logCallback);
        SubscribedSignal dataSignal(signalNumber, logCallback);
        prepareSignals(dataSignal, timeSignal, startTime);

        nlohmann::json metaDataSignal;
        metaDataSignal[META_DEFINITION][META_RULE] = META_RULETYPE_CONSTANT;
        metaDataSignal[META_DEFINITION][META_DATATYPE] = DATA_TYPE_UINT32;

        result = dataSignal.processSignalMetaInformation(META_METHOD_SIGNAL, metaDataSignal);
        ASSERT_EQ(result, 0);
        ASSERT_EQ(dataSignal.ruleType(), RULETYPE_CONSTANT);
        ASSERT_EQ(dataSignal.dataValueType(), SAMPLETYPE_U32);

        Uint32WithValueIndex expectedData[] = {
            { 0, 5},
            { 1, 6},
            { 2, 7}
        };

        uint64_t deliveredFirstTimestamp;
        auto dataAsValueCb = [&](const SubscribedSignal& subscribedSignal, uint64_t timeStamp, const uint8_t* deliveredData, size_t valueCount)
        {
            std::vector <double> doubleData(valueCount);
            deliveredFirstTimestamp = timeStamp;
            subscribedSignal.interpretValuesAsDouble(deliveredData, valueCount, doubleData.data());
            auto dataIter = doubleData.begin();
            for (auto expectedIter : expectedData) {
                double data = *dataIter;
                double expectedAsDouble = expectedIter.value;
                ASSERT_EQ(data, expectedAsDouble);
                ++dataIter;
            }
            ASSERT_EQ(valueCount, sizeof(expectedData)/sizeof(expectedData[0]));
        };

        auto dataAsRawCb = [&](const SubscribedSignal& subscribedSignal, uint64_t timeStamp, const uint8_t* deliveredData, size_t byteCount)
        {
            ASSERT_EQ(byteCount, sizeof(expectedData));
            ASSERT_EQ(memcmp(deliveredData, expectedData, byteCount),0);
        };

        size_t dataSize = sizeof(expectedData);
        ssize_t processResult = dataSignal.processMeasuredData(reinterpret_cast <unsigned char*> (expectedData), dataSize, timeSignal, dataAsRawCb, dataAsValueCb);
        ASSERT_EQ(deliveredFirstTimestamp, startTime);
        ASSERT_EQ(processResult, sizeof(expectedData));

        {
            // check unit
            Unit unit;
            unit.displayName = "V";
            unit.id = 6;
            nlohmann::json metaUnit;

            /// \warning no checking of display naming vs. unit id
            unit.compose(metaUnit[META_DEFINITION]);
            result = dataSignal.processSignalMetaInformation(META_METHOD_SIGNAL, metaUnit);
            ASSERT_EQ(result, 0);
            ASSERT_EQ(dataSignal.unitDisplayName(), unit.displayName);
            ASSERT_EQ(dataSignal.unitId(), unit.id);
        }
    }

    TEST(SubscribedSignalTest, dataCb_int8_test)
    {
        SignalNumber signalNumber = 9;
        int result;
        uint64_t startTime = 1000;

        auto timeSignal = std::make_shared<SubscribedSignal>(signalNumber+1, logCallback);
        SubscribedSignal dataSignal(signalNumber, logCallback);
        prepareSignals(dataSignal, timeSignal, startTime);

        nlohmann::json metaDataSignal;
        metaDataSignal[META_DEFINITION][META_RULE] = META_RULETYPE_EXPLICIT;
        metaDataSignal[META_DEFINITION][META_DATATYPE] = DATA_TYPE_INT8;
        result = dataSignal.processSignalMetaInformation(META_METHOD_SIGNAL, metaDataSignal);
        ASSERT_EQ(result, 0);
        ASSERT_EQ(dataSignal.dataValueType(), SAMPLETYPE_S8);
        int8_t expectedData[] = { -1, -2, -3};

        uint64_t deliveredFirstTimestamp;
        auto dataAsValueCb = [&](const SubscribedSignal& subscribedSignal, uint64_t timeStamp, const uint8_t* deliveredData, size_t valueCount)
        {
            std::vector <double> doubleData(valueCount);
            deliveredFirstTimestamp = timeStamp;
            subscribedSignal.interpretValuesAsDouble(deliveredData, valueCount, doubleData.data());
            auto dataIter = doubleData.begin();
            for (auto expectedIter : expectedData) {
                double data = *dataIter;
                double expectedAsDouble = expectedIter;
                ASSERT_EQ(data, expectedAsDouble);
                ++dataIter;
            }
            ASSERT_EQ(valueCount, sizeof(expectedData)/sizeof(expectedData[0]));
        };

        auto dataAsRawCb = [&](const SubscribedSignal& subscribedSignal, uint64_t timeStamp, const uint8_t* deliveredData, size_t byteCount)
        {
            ASSERT_EQ(byteCount, sizeof(expectedData));
            ASSERT_EQ(memcmp(deliveredData, expectedData, byteCount),0);
        };

        ssize_t processResult = dataSignal.processMeasuredData(reinterpret_cast <unsigned char*> (expectedData), sizeof(expectedData), timeSignal, dataAsRawCb, dataAsValueCb);
        ASSERT_EQ(processResult, sizeof(expectedData));
    }

    TEST(SubscribedSignalTest, dataCb_int16_test)
    {
        SignalNumber signalNumber = 9;
        int result;
        uint64_t startTime = 1000;

        auto timeSignal = std::make_shared<SubscribedSignal>(signalNumber+1, logCallback);
        SubscribedSignal dataSignal(signalNumber, logCallback);
        prepareSignals(dataSignal, timeSignal, startTime);

        nlohmann::json metaDataSignal;
        metaDataSignal[META_DEFINITION][META_RULE] = META_RULETYPE_EXPLICIT;
        metaDataSignal[META_DEFINITION][META_DATATYPE] = DATA_TYPE_INT16;
        result = dataSignal.processSignalMetaInformation(META_METHOD_SIGNAL, metaDataSignal);
        ASSERT_EQ(result, 0);
        ASSERT_EQ(dataSignal.dataValueType(), SAMPLETYPE_S16);
        ASSERT_EQ(dataSignal.ruleType(), RULETYPE_EXPLICIT);
        int16_t expectedData[] = { -1, -2, -3};

        uint64_t deliveredFirstTimestamp;
        auto dataAsValueCb = [&](const SubscribedSignal& subscribedSignal, uint64_t timeStamp, const uint8_t* deliveredData, size_t valueCount)
        {
            std::vector <double> doubleData(valueCount);
            deliveredFirstTimestamp = timeStamp;
            subscribedSignal.interpretValuesAsDouble(deliveredData, valueCount, doubleData.data());
            auto dataIter = doubleData.begin();
            for (auto expectedIter : expectedData) {
                double data = *dataIter;
                double expectedAsDouble = expectedIter;
                ASSERT_EQ(data, expectedAsDouble);
                ++dataIter;
            }
            ASSERT_EQ(valueCount, sizeof(expectedData)/sizeof(expectedData[0]));
        };

        auto dataAsRawCb = [&](const SubscribedSignal& subscribedSignal, uint64_t timeStamp, const uint8_t* deliveredData, size_t byteCount)
        {
            ASSERT_EQ(byteCount, sizeof(expectedData));
            ASSERT_EQ(memcmp(deliveredData, expectedData, byteCount),0);
        };
        size_t dataSize = sizeof(expectedData);
        ssize_t processResult = dataSignal.processMeasuredData(reinterpret_cast <unsigned char*> (expectedData), dataSize, timeSignal, dataAsRawCb, dataAsValueCb);
        ASSERT_EQ(processResult, sizeof(expectedData));
    }

    TEST(SubscribedSignalTest, dataCb_bitfield32_test)
    {
        SignalNumber signalNumber = 9;
        int result;
        uint64_t startTime = 1000;


        auto timeSignal = std::make_shared<SubscribedSignal>(signalNumber+1, logCallback);
        SubscribedSignal dataSignal(signalNumber, logCallback);
        prepareSignals(dataSignal, timeSignal, startTime);

        nlohmann::json metaDataSignal;
        metaDataSignal[META_DEFINITION][META_RULE] = META_RULETYPE_EXPLICIT;
        metaDataSignal[META_DEFINITION][META_DATATYPE] = DATA_TYPE_BITFIELD;
        metaDataSignal[META_DEFINITION][DATA_TYPE_BITFIELD][META_DATATYPE] = DATA_TYPE_UINT32;

        result = dataSignal.processSignalMetaInformation(META_METHOD_SIGNAL, metaDataSignal);
        ASSERT_EQ(result, 0);
        ASSERT_EQ(dataSignal.ruleType(), RULETYPE_EXPLICIT);
        ASSERT_EQ(dataSignal.dataValueType(), SAMPLETYPE_BITFIELD32);

        uint32_t expectedData[] = {
            0x01000000,
            0x02000000,
            0x03000000
        };

        uint64_t deliveredFirstTimestamp;
        auto dataAsValueCb = [&](const SubscribedSignal& subscribedSignal, uint64_t timeStamp, const uint8_t* deliveredData, size_t valueCount)
        {
            deliveredFirstTimestamp = timeStamp;
            ASSERT_EQ(valueCount, sizeof(expectedData)/sizeof(expectedData[0]));
        };

        auto dataAsRawCb = [&](const SubscribedSignal& subscribedSignal, uint64_t timeStamp, const uint8_t* deliveredData, size_t byteCount)
        {
            ASSERT_EQ(byteCount, sizeof(expectedData));
            ASSERT_EQ(memcmp(deliveredData, expectedData, byteCount),0);
        };

        ssize_t processResult = dataSignal.processMeasuredData(reinterpret_cast <unsigned char*> (expectedData), sizeof(expectedData), timeSignal, dataAsRawCb, dataAsValueCb);
        ASSERT_EQ(deliveredFirstTimestamp, startTime);
        ASSERT_EQ(processResult, sizeof(expectedData));
    }

    TEST(SubscribedSignalTest, dataCb_bitfield64_with_constant_rule_test)
    {
        SignalNumber signalNumber = 9;
        int result;
        uint64_t startTime = 1000;

        struct BitFieldWithValueIndex
        {
            uint64_t valueIndex;
            int64_t bitfield;
        };

        auto timeSignal = std::make_shared<SubscribedSignal>(signalNumber+1, logCallback);
        SubscribedSignal dataSignal(signalNumber, logCallback);
        prepareSignals(dataSignal, timeSignal, startTime);

        nlohmann::json metaDataSignal;
        metaDataSignal[META_DEFINITION][META_RULE] = META_RULETYPE_CONSTANT;
        metaDataSignal[META_DEFINITION][META_DATATYPE] = DATA_TYPE_BITFIELD;
        metaDataSignal[META_DEFINITION][DATA_TYPE_BITFIELD][META_DATATYPE] = DATA_TYPE_UINT64;

        result = dataSignal.processSignalMetaInformation(META_METHOD_SIGNAL, metaDataSignal);
        ASSERT_EQ(result, 0);
        ASSERT_EQ(dataSignal.ruleType(), RULETYPE_CONSTANT);
        ASSERT_EQ(dataSignal.dataValueType(), SAMPLETYPE_BITFIELD64);

        BitFieldWithValueIndex expectedData[] = {
            { 0, 0x0100000000},
            { 1, 0x0200000000},
            { 2, 0x0300000000}
        };

        uint64_t deliveredFirstTimestamp;
        auto dataAsValueCb = [&](const SubscribedSignal& subscribedSignal, uint64_t timeStamp, const uint8_t* deliveredData, size_t valueCount)
        {
            deliveredFirstTimestamp = timeStamp;
            ASSERT_EQ(valueCount, sizeof(expectedData)/sizeof(expectedData[0]));
        };

        auto dataAsRawCb = [&](const SubscribedSignal& subscribedSignal, uint64_t timeStamp, const uint8_t* deliveredData, size_t byteCount)
        {
            ASSERT_EQ(byteCount, sizeof(expectedData));
            ASSERT_EQ(memcmp(deliveredData, expectedData, byteCount),0);
        };

        ssize_t processResult = dataSignal.processMeasuredData(reinterpret_cast <unsigned char*> (expectedData), sizeof(expectedData), timeSignal, dataAsRawCb, dataAsValueCb);
        ASSERT_EQ(deliveredFirstTimestamp, startTime);
        ASSERT_EQ(processResult, sizeof(expectedData));
    }

    TEST(SubscribedSignalTest, dataCb_bitfieldunsupported_test)
    {
        SignalNumber signalNumber = 9;
        int result;
        uint64_t startTime = 1000;


        auto timeSignal = std::make_shared<SubscribedSignal>(signalNumber+1, logCallback);
        SubscribedSignal dataSignal(signalNumber, logCallback);
        prepareSignals(dataSignal, timeSignal, startTime);

        nlohmann::json metaDataSignal;
        metaDataSignal[META_DEFINITION][META_RULE] = META_RULETYPE_EXPLICIT;
        metaDataSignal[META_DEFINITION][META_DATATYPE] = DATA_TYPE_BITFIELD;
        metaDataSignal[META_DEFINITION][DATA_TYPE_BITFIELD][META_DATATYPE] = DATA_TYPE_UINT16;

        result = dataSignal.processSignalMetaInformation(META_METHOD_SIGNAL, metaDataSignal);
        ASSERT_NE(result, 0);
    }


    TEST(SubscribedSignalTest, dataCb_complex32_test)
    {
        SignalNumber signalNumber = 9;
        int result;
        uint64_t startTime = 1000;

        auto timeSignal = std::make_shared<SubscribedSignal>(signalNumber+1, logCallback);
        SubscribedSignal dataSignal(signalNumber, logCallback);
        prepareSignals(dataSignal, timeSignal, startTime);

        nlohmann::json metaDataSignal;
        metaDataSignal[META_DEFINITION][META_RULE] = META_RULETYPE_EXPLICIT;
        metaDataSignal[META_DEFINITION][META_DATATYPE] = DATA_TYPE_COMPLEX32;
        result = dataSignal.processSignalMetaInformation(META_METHOD_SIGNAL, metaDataSignal);
        ASSERT_EQ(result, 0);
        ASSERT_EQ(dataSignal.dataValueType(), SAMPLETYPE_COMPLEX32);
        Complex32Type expectedData[] = {
            { -1, -1},
            { -2, -2},
            { -3, -3}};

        uint64_t deliveredFirstTimestamp;
        auto dataAsValueCb = [&](const SubscribedSignal& subscribedSignal, uint64_t timeStamp, const uint8_t* deliveredData, size_t valueCount)
        {
            deliveredFirstTimestamp = timeStamp;
            ASSERT_EQ(valueCount, sizeof(expectedData)/sizeof(expectedData[0]));
        };

        auto dataAsRawCb = [&](const SubscribedSignal& subscribedSignal, uint64_t timeStamp, const uint8_t* deliveredData, size_t byteCount)
        {
            ASSERT_EQ(byteCount, sizeof(expectedData));
            ASSERT_EQ(memcmp(deliveredData, expectedData, byteCount),0);
        };

        ssize_t processResult = dataSignal.processMeasuredData(reinterpret_cast <unsigned char*> (expectedData), sizeof(expectedData), timeSignal, dataAsRawCb, dataAsValueCb);
        ASSERT_EQ(processResult, sizeof(expectedData));
    }

    TEST(SubscribedSignalTest, dataCb_complex64_test)
    {
        SignalNumber signalNumber = 9;
        int result;
        uint64_t startTime = 1000;

        auto timeSignal = std::make_shared<SubscribedSignal>(signalNumber+1, logCallback);
        SubscribedSignal dataSignal(signalNumber, logCallback);
        prepareSignals(dataSignal, timeSignal, startTime);

        nlohmann::json metaDataSignal;
        metaDataSignal[META_DEFINITION][META_RULE] = META_RULETYPE_EXPLICIT;
        metaDataSignal[META_DEFINITION][META_DATATYPE] = DATA_TYPE_COMPLEX64;
        result = dataSignal.processSignalMetaInformation(META_METHOD_SIGNAL, metaDataSignal);
        ASSERT_EQ(result, 0);
        ASSERT_EQ(dataSignal.dataValueType(), SAMPLETYPE_COMPLEX64);
        Complex64Type expectedData[] = {
            { -1, -1},
            { -2, -2},
            { -3, -3}};

        uint64_t deliveredFirstTimestamp;
        auto dataAsValueCb = [&](const SubscribedSignal& subscribedSignal, uint64_t timeStamp, const uint8_t* deliveredData, size_t valueCount)
        {
            deliveredFirstTimestamp = timeStamp;
            ASSERT_EQ(valueCount, sizeof(expectedData)/sizeof(expectedData[0]));
        };

        auto dataAsRawCb = [&](const SubscribedSignal& subscribedSignal, uint64_t timeStamp, const uint8_t* deliveredData, size_t byteCount)
        {
            ASSERT_EQ(byteCount, sizeof(expectedData));
            ASSERT_EQ(memcmp(deliveredData, expectedData, byteCount),0);
        };

        ssize_t processResult = dataSignal.processMeasuredData(reinterpret_cast <unsigned char*> (expectedData), sizeof(expectedData), timeSignal, dataAsRawCb, dataAsValueCb);
        ASSERT_EQ(processResult, sizeof(expectedData));
    }

    TEST(SubscribedSignalTest, dataCb_array_of_int16_test)
    {
        SignalNumber signalNumber = 9;
        int result;
        uint64_t startTime = 1000;
        static const unsigned int count = 3;

        auto timeSignal = std::make_shared<SubscribedSignal>(signalNumber+1, logCallback);
        SubscribedSignal dataSignal(signalNumber, logCallback);
        prepareSignals(dataSignal, timeSignal, startTime);

        nlohmann::json metaDataSignal;
        metaDataSignal[META_DEFINITION][META_RULE] = META_RULETYPE_EXPLICIT;
        metaDataSignal[META_DEFINITION][META_DATATYPE] = DATA_TYPE_ARRAY;

        metaDataSignal[META_DEFINITION][DATA_TYPE_ARRAY][META_DATATYPE] = DATA_TYPE_INT16;
        metaDataSignal[META_DEFINITION][DATA_TYPE_ARRAY][META_COUNT] = count;
        result = dataSignal.processSignalMetaInformation(META_METHOD_SIGNAL, metaDataSignal);
        ASSERT_EQ(result, 0);
        ASSERT_EQ(dataSignal.dataValueType(), SAMPLETYPE_ARRAY);
        ASSERT_EQ(dataSignal.dataValueSize(), sizeof(int16_t)*count);
        std::array <int16_t, count > expectedData[] = { {-1, -2, -3}, {1, 2, 3}, {10, 20, 30}};

        uint64_t deliveredFirstTimestamp;
        auto dataAsValueCb = [&](const SubscribedSignal& subscribedSignal, uint64_t timeStamp, const uint8_t* deliveredData, size_t valueCount)
        {
            deliveredFirstTimestamp = timeStamp;
            ASSERT_EQ(valueCount, sizeof(expectedData)/sizeof(expectedData[0]));
        };

        auto dataAsRawCb = [&](const SubscribedSignal& subscribedSignal, uint64_t timeStamp, const uint8_t* deliveredData, size_t byteCount)
        {
            ASSERT_EQ(byteCount, sizeof(expectedData));
            ASSERT_EQ(memcmp(deliveredData, expectedData, byteCount),0);
        };

        ssize_t processResult = dataSignal.processMeasuredData(reinterpret_cast <unsigned char*> (expectedData), sizeof(expectedData), timeSignal, dataAsRawCb, dataAsValueCb);
        ASSERT_EQ(processResult, sizeof(expectedData));
    }

    TEST(SubscribedSignalTest, dataCb_struct_test)
    {
        SignalNumber signalNumber = 9;
        int result;
        uint64_t startTime = 1000;
        static const unsigned int count = 4;

        struct Struct {
            std::array <int16_t, count > Int16Array;
            int32_t Int32;
        };

        auto timeSignal = std::make_shared<SubscribedSignal>(signalNumber+1, logCallback);
        SubscribedSignal dataSignal(signalNumber, logCallback);
        prepareSignals(dataSignal, timeSignal, startTime);

        nlohmann::json metaDataSignal;
        metaDataSignal[META_DEFINITION][META_RULE] = META_RULETYPE_EXPLICIT;
        metaDataSignal[META_DEFINITION][META_DATATYPE] = DATA_TYPE_STRUCT;
        metaDataSignal[META_DEFINITION][META_NAME] = "theStruct";

        metaDataSignal[META_DEFINITION][DATA_TYPE_STRUCT][0][META_NAME] = "Int16Array";
        //metaDataSignal[META_DEFINITION][DATA_TYPE_STRUCT][0][META_DATATYPE] = DATA_TYPE_INT16;
        metaDataSignal[META_DEFINITION][DATA_TYPE_STRUCT][0][META_DATATYPE] = DATA_TYPE_ARRAY;
        metaDataSignal[META_DEFINITION][DATA_TYPE_STRUCT][0][DATA_TYPE_ARRAY][META_DATATYPE] = DATA_TYPE_INT16;
        metaDataSignal[META_DEFINITION][DATA_TYPE_STRUCT][0][DATA_TYPE_ARRAY][META_COUNT] = count;

        metaDataSignal[META_DEFINITION][DATA_TYPE_STRUCT][1][META_NAME] = "Int32";
        metaDataSignal[META_DEFINITION][DATA_TYPE_STRUCT][1][META_DATATYPE] = DATA_TYPE_INT32;
        result = dataSignal.processSignalMetaInformation(META_METHOD_SIGNAL, metaDataSignal);
        ASSERT_EQ(result, 0);
        ASSERT_EQ(dataSignal.dataValueType(), SAMPLETYPE_STRUCT);
        ASSERT_EQ(dataSignal.dataValueSize(), sizeof(Struct));
        Struct expectedData[] = {
            { {1,2,3,4}, -2},
            { {5,6,7,8}, -4},
            { {10,11,12,13}, -8}
        };

        uint64_t deliveredFirstTimestamp;
        auto dataAsValueCb = [&](const SubscribedSignal& subscribedSignal, uint64_t timeStamp, const uint8_t* deliveredData, size_t valueCount)
        {
            deliveredFirstTimestamp = timeStamp;
            ASSERT_EQ(valueCount, sizeof(expectedData)/sizeof(expectedData[0]));
        };

        auto dataAsRawCb = [&](const SubscribedSignal& subscribedSignal, uint64_t timeStamp, const uint8_t* deliveredData, size_t byteCount)
        {
            ASSERT_EQ(byteCount, sizeof(expectedData));
            ASSERT_EQ(memcmp(deliveredData, expectedData, byteCount),0);
        };

        ssize_t processResult = dataSignal.processMeasuredData(reinterpret_cast <unsigned char*> (expectedData), sizeof(expectedData), timeSignal, dataAsRawCb, dataAsValueCb);
        ASSERT_EQ(processResult, sizeof(expectedData));
    }

	TEST(SubscribedSignalTest, dataCb_double_test)
	{
		SignalNumber signalNumber = 9;
		int result;
		uint64_t startTime = 1000;
		uint64_t signalDelayIndex = 10;

        auto timeSignal = std::make_shared<SubscribedSignal>(signalNumber+1, logCallback);
		SubscribedSignal dataSignal(signalNumber, logCallback);
		prepareSignals(dataSignal, timeSignal, startTime);

		Range range;
		PostScaling postScaling;
		range.low = -20;
		range.high = 30;

		postScaling.offset = 10.1;
		postScaling.scale = 10.0;

		nlohmann::json metaDataSignal;
		metaDataSignal[META_VALUEINDEX] = signalDelayIndex; // this signal is late!
		metaDataSignal[META_DEFINITION][META_RULE] = META_RULETYPE_EXPLICIT;
		metaDataSignal[META_DEFINITION][META_DATATYPE] = DATA_TYPE_REAL64;

		range.compose(metaDataSignal[META_DEFINITION]);
		postScaling.compose(metaDataSignal[META_DEFINITION]);

		result = dataSignal.processSignalMetaInformation(META_METHOD_SIGNAL, metaDataSignal);
		ASSERT_EQ(result, 0);
		ASSERT_EQ(dataSignal.ruleType(), RULETYPE_EXPLICIT);
		ASSERT_EQ(dataSignal.dataValueType(), SAMPLETYPE_REAL64);

		std::array < double, 3 > expectedData = { 1.1, 2.2, 3.3};

		uint64_t deliveredFirstTimestamp;
		auto dataAsValueCb = [&](const SubscribedSignal& subscribedSignal, uint64_t timeStamp, const uint8_t* deliveredData, size_t valueCount)
		{
			std::vector <double> doubleData(valueCount);
			deliveredFirstTimestamp = timeStamp;
			subscribedSignal.interpretValuesAsDouble(deliveredData, valueCount, doubleData.data());
			auto dataIter = doubleData.begin();
			for (auto expectedIter : expectedData) {
				double data = *dataIter;
				double expected = expectedIter;
				ASSERT_NEAR(data, expected, 0.000001);
				++dataIter;
			}
			ASSERT_EQ(valueCount, sizeof(expectedData)/sizeof(expectedData[0]));
		};

		auto dataAsRawCb = [&](const SubscribedSignal& subscribedSignal, uint64_t timeStamp, const uint8_t* deliveredData, size_t byteCount)
		{
			ASSERT_EQ(byteCount, sizeof(expectedData));
			ASSERT_EQ(memcmp(deliveredData, expectedData.data(), byteCount),0);
		};

		ssize_t processResult = dataSignal.processMeasuredData(reinterpret_cast <unsigned char*> (expectedData.data()), sizeof(expectedData), timeSignal, dataAsRawCb, dataAsValueCb);
		/// check for the delay!
		ASSERT_EQ(deliveredFirstTimestamp, (startTime+signalDelayIndex*timeSignal->linearDelta()));
		ASSERT_EQ(processResult, sizeof(expectedData));

		{
			// check unit
            nlohmann::json metaUnit;
            std::string unitDisplayName = "V";
            int32_t unitId = 6;

            Unit unit;
            unit.id = unitId;
            unit.displayName = unitDisplayName;
            /// \warning no checking of display naming vs. unit id
            unit.compose(metaUnit[META_DEFINITION]);
            result = dataSignal.processSignalMetaInformation(META_METHOD_SIGNAL, metaUnit);
            ASSERT_EQ(result, 0);
            ASSERT_EQ(dataSignal.unitDisplayName(), unit.displayName);
            ASSERT_EQ(dataSignal.unitId(), unit.id);
        }

        {
			// check range
			Range rangeResult =	dataSignal.range();
			ASSERT_EQ(range, rangeResult);
		}

		{
			// check post scaling
			PostScaling postScalingResult = dataSignal.postScaling();
			ASSERT_EQ(postScaling, postScalingResult);
		}
	}
}
