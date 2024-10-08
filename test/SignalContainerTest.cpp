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

#include "nlohmann/json.hpp"

#include "../include/streaming_protocol/Defines.h"
#include "../include/streaming_protocol/MetaInformation.hpp"
#include "../include/streaming_protocol/SignalContainer.hpp"
#include "../include/streaming_protocol/SubscribedSignal.hpp"

#include "../include/streaming_protocol/Logging.hpp"


namespace daq::streaming_protocol {
    unsigned int s_signalMetaSignalNumber;
    static std::string s_signalMetaMethod;
    static nlohmann::json s_signalMetaParameters;

    static std::vector < int32_t > s_measuredDataAsInt32;
    static std::vector < double > s_measuredDataAsDouble;

    static std::vector < uint8_t > s_measuredRawData;
    static unsigned int s_measuredRawDataSignalNumber;
    static uint64_t s_measuredRawDataTimestamp;
    static unsigned int s_measuredTimeSignalNumber = 999999; // Time signal's number as seen in the data callback

    static LogCallback logCallback = daq::streaming_protocol::Logging::logCallback();


    unsigned int s_dataSignalNumber = 1;
    static const nlohmann::json s_subscribeAckDataSignalDoc = R"(
    {
        "method" : "subscribe",
        "params" : {
            "signalId" : "DataSignal"
        }
    }
    )"_json;

    unsigned int s_anotherDataSignalNumber = 2;
    static const nlohmann::json s_subscribeAckAnotherDataSignalDoc = R"(
    {
        "method" : "subscribe",
        "params" : {
            "signalId" : "AnotherDataSignal"
        }
    }
    )"_json;

    unsigned int s_timeSignalNumber = 3;
    static const nlohmann::json s_subscribeAckTimeSignalDoc = R"(
    {
        "method" : "subscribe",
        "params" : {
            "signalId" : "TimeSignal"
        }
    }
    )"_json;

    nlohmann::json s_unsubscribeAckDoc = R"(
    {
        "method" : "unsubscribe"
    }
    )"_json;



    static nlohmann::json s_dataDoubleSignalMetaInformationDoc = R"(
    {
        "method" : "signal",
        "params" : {
            "definition" : {
                "dataType" : "real64"
            },
            "tableId" : "table"
        }
    }
    )"_json;

    static nlohmann::json s_dataInt32SignalMetaInformationDoc = R"(
    {
        "method" : "signal",
        "params" : {
            "definition" : {
                "dataType" : "int32"
            },
            "tableId" : "table"
        }
    }
    )"_json;


    static nlohmann::json s_anotherDoubleDataSignalMetaInformationDoc = R"(
    {
        "method" : "signal",
        "params" : {
            "definition" : {
                "dataType" : "int32"
            },
            "tableId" : "table"
        }
    }
    )"_json;

    /// openDAQ time resolution is 1
    /// delta is one => f_output = 1 Hz
    /// \note unit "s" as defined in opc-ua standard
    static nlohmann::json s_linearOpenDAQTimeSignalMetaInformationDoc = R"(
    {
        "method" : "signal",
        "params" : {
            "definition" : {
                "dataType" : "uint64",
                "rule" : "linear",
                "linear" : {
                    "delta" : 1
                },
                "unit" : {
                    "displayName": "s",
                    "unitId": 5457219,
                    "quantity": "time"
                },
                "resolution" : {
                    "num" : 1,
                    "denom" : 1
                },
                "absoluteReference" : "1970-01-01T00:00:00.0"
            },
            "tableId" : "table"
        }
    }
    )"_json;

    /// openDAQ time resolution is 1
    static nlohmann::json s_explicitOpenDAQTimeSignalMetaInformationDoc = R"(
    {
        "method" : "signal",
        "params" : {
            "definition" : {
                "dataType" : "uint64",
                "rule" : "explicit",
                "unit" : {
                    "displayName": "s",
                    "unitId": 5457219,
                    "quantity": "time"
                },
                "resolution" : {
                    "num" : 1,
                    "denom" : 1
                },
                "absoluteReference" : "1970-01-01T00:00:00.0"
            },
            "tableId" : "table"
        }
    }
    )"_json;


    static void signalMetaCb(const SubscribedSignal& subscribedSignal, const std::string& method, const nlohmann::json& params)
    {
        s_signalMetaSignalNumber = subscribedSignal.signalNumber();
        s_signalMetaMethod = method;
        s_signalMetaParameters = params;
    }


    static void dataAsRawCb(const SubscribedSignal& subscribedSignal, uint64_t timeStamp, const uint8_t* pValues, size_t size)
    {
        // raw data is always handled in one piece.
        s_measuredRawData.resize(size);
        memcpy(s_measuredRawData.data(), pValues, size);
        s_measuredRawDataSignalNumber = subscribedSignal.signalNumber();
        s_measuredRawDataTimestamp = timeStamp;
        auto foundTimeSignal = subscribedSignal.timeSignal();
        if (foundTimeSignal)
        {
            s_measuredTimeSignalNumber = foundTimeSignal->signalNumber();
        }
            
        size_t valueCount = size/subscribedSignal.dataValueSize();
        s_measuredDataAsDouble.resize(valueCount);
        subscribedSignal.interpretValuesAsDouble(pValues, valueCount, s_measuredDataAsDouble.data());
    }

    static void dataAsValueCb(const SubscribedSignal& subscribedSignal, uint64_t timeStamp, const uint8_t* pValues, size_t valueCount)
    {
        // raw data is always handled in one piece.
        s_measuredDataAsInt32.resize(valueCount);
        size_t size = valueCount*subscribedSignal.dataValueSize();
        memcpy(s_measuredDataAsInt32.data(), pValues, size);
        //s_measuredRawDataSignalNumber = subscribedSignal.signalNumber();
        //s_measuredRawDataTimestamp = timeStamp;

        //size_t valueCount = size/subscribedSignal.dataValueSize();
        //s_measuredDataAsDouble.resize(valueCount);
        //subscribedSignal.interpretValuesAsDouble(pValues, valueCount, s_measuredDataAsDouble.data());
    }


    static MetaInformation creataMetaInformation(const std::vector < uint8_t >& payload)
    {
        std::vector < uint8_t > package;
        package.resize(sizeof(METAINFORMATION_MSGPACK) + payload.size());
        memcpy(package.data(), &METAINFORMATION_MSGPACK, sizeof(METAINFORMATION_MSGPACK));
        memcpy(&package[sizeof(METAINFORMATION_MSGPACK)], payload.data(), payload.size());
        MetaInformation metaInformation(logCallback);
        metaInformation.interpret(package.data(), package.size());
        return metaInformation;
    }

    TEST(SignalContainerTest, unknown_signal_test)
    {
        // signal is unknown when there was no subscribe ack before
        int result;
        SignalContainer signalContainer(logCallback);
        MetaInformation metaInformation(logCallback);

        result = signalContainer.processMetaInformation(s_dataSignalNumber, metaInformation);
        ASSERT_NE(result, 0);
    }

    TEST(SignalContainerTest, subscribe_ack_test)
    {
        int result;
        SignalContainer signalContainer(logCallback);

        signalContainer.setSignalMetaCb(signalMetaCb);
        std::vector < uint8_t > payload;
        MetaInformation metaInformation(logCallback);

        {
            // no parameters at all!
            static const nlohmann::json incompleteSubscribeAckSignalDoc = R"(
            {
                "method" : "subscribe"
            }
            )"_json;
            payload = nlohmann::json::to_msgpack(incompleteSubscribeAckSignalDoc);
            metaInformation = creataMetaInformation(payload);
            result = signalContainer.processMetaInformation(s_dataSignalNumber, metaInformation);
            ASSERT_EQ(result, -1);
        }

        {
            // signal id is missing!
            static const nlohmann::json incompleteSubscribeAckSignalDoc = R"(
            {
                "method" : "subscribe",
                "params" : {
                }
            }
            )"_json;
            payload = nlohmann::json::to_msgpack(incompleteSubscribeAckSignalDoc);
            metaInformation = creataMetaInformation(payload);
            result = signalContainer.processMetaInformation(s_dataSignalNumber, metaInformation);
            ASSERT_EQ(result, -1);
        }


        // complete and valid
        payload = nlohmann::json::to_msgpack(s_subscribeAckDataSignalDoc);
        metaInformation = creataMetaInformation(payload);
        result = signalContainer.processMetaInformation(s_dataSignalNumber, metaInformation);
        ASSERT_EQ(result, 0);

        ASSERT_EQ(s_signalMetaSignalNumber, s_dataSignalNumber);
        ASSERT_EQ(s_signalMetaMethod, META_METHOD_SUBSCRIBE);
        ASSERT_EQ(s_signalMetaParameters, s_subscribeAckDataSignalDoc[PARAMS]);
    }

    TEST(SignalContainerTest, data_signal_test)
    {
        int result;
        SignalContainer signalContainer(logCallback);
        MetaInformation metaInformation(logCallback);

        // subscribe ack first
        std::vector < uint8_t > payload = nlohmann::json::to_msgpack(s_subscribeAckDataSignalDoc);
        metaInformation = creataMetaInformation(payload);
        signalContainer.processMetaInformation(s_dataSignalNumber, metaInformation);


        {
            // Explicit signal definition with invalid data type
            static nlohmann::json invalidDataTypeMetaInformationDoc = R"(
            {
                "method" : "signal",
                "params" : {
                    "definition" : {
                        "dataType" : "invalid"
                    }
                }
            }
            )"_json;


            payload = nlohmann::json::to_msgpack(invalidDataTypeMetaInformationDoc);
            metaInformation = creataMetaInformation(payload);
            result = signalContainer.processMetaInformation(s_dataSignalNumber, metaInformation);
            ASSERT_EQ(result, -1);

        }


        {
            // Valid explicit signal definition
            payload = nlohmann::json::to_msgpack(s_dataDoubleSignalMetaInformationDoc);
            metaInformation = creataMetaInformation(payload);
            result = signalContainer.processMetaInformation(s_dataSignalNumber, metaInformation);
            ASSERT_EQ(result, 0);
        }

        {
            // An implicit unknown rule for the signal
            static nlohmann::json unknownRuleSignalMetaInformationDoc = R"(
            {
                "method" : "signal",
                "params" : {
                    "definition" : {
                        "dataType" : "real64",
                        "rule" : "xxxxx"
                    },
                    "tableId" : "table"
                }
            }
            )"_json;
            payload = nlohmann::json::to_msgpack(unknownRuleSignalMetaInformationDoc);
            metaInformation = creataMetaInformation(payload);
            result = signalContainer.processMetaInformation(s_dataSignalNumber, metaInformation);
            ASSERT_EQ(result, -1);
        }



    }

    TEST(SignalContainerTest, time_signal_test)
    {
        int result;
        SignalContainer signalContainer(logCallback);

        // subscribe ack first
        std::vector < uint8_t > payload = nlohmann::json::to_msgpack(s_subscribeAckTimeSignalDoc);
        MetaInformation subscribeMetaInformation = creataMetaInformation(payload);
        signalContainer.processMetaInformation(s_timeSignalNumber, subscribeMetaInformation);


        payload = nlohmann::json::to_msgpack(s_linearOpenDAQTimeSignalMetaInformationDoc);
        MetaInformation metaInformation = creataMetaInformation(payload);
        result = signalContainer.processMetaInformation(s_timeSignalNumber, metaInformation);
        ASSERT_EQ(result, 0);
    }

    TEST(SignalContainerTest, unsubscribe_ack_test)
    {
        int result;
        SignalContainer signalContainer(logCallback);
        MetaInformation metaInformation(logCallback);

        {
            // non-existing signal

            ASSERT_EQ(signalContainer.setSignalMetaCb(signalMetaCb), 0);
            ASSERT_EQ(signalContainer.setSignalMetaCb(SignalMetaCb()), -1);


            std::vector < uint8_t > payload = nlohmann::json::to_msgpack(s_unsubscribeAckDoc);
            metaInformation = creataMetaInformation(payload);
            result = signalContainer.processMetaInformation(s_dataSignalNumber, metaInformation);
            ASSERT_EQ(result, -1);
        }

        {
            // unsubscribe ack of an existing signal

            // subscribe ack first
            std::vector < uint8_t > payload = nlohmann::json::to_msgpack(s_subscribeAckDataSignalDoc);
            metaInformation = creataMetaInformation(payload);
            signalContainer.processMetaInformation(s_dataSignalNumber, metaInformation);


            // signal meta tells about the table the signal belongs to
            payload = nlohmann::json::to_msgpack(s_dataDoubleSignalMetaInformationDoc);
            metaInformation = creataMetaInformation(payload);
            signalContainer.processMetaInformation(s_dataSignalNumber, metaInformation);

            signalContainer.setSignalMetaCb(signalMetaCb);


            payload = nlohmann::json::to_msgpack(s_unsubscribeAckDoc);
            metaInformation = creataMetaInformation(payload);
            result = signalContainer.processMetaInformation(s_dataSignalNumber, metaInformation);
            ASSERT_EQ(result, 0);
        }
    }

    TEST(SignalContainerTest, table_test)
    {
        // two signals belonging to the same table
        int result;
        SignalContainer signalContainer(logCallback);

        std::vector < uint8_t > payload;
        MetaInformation metaInformation(logCallback);

        // subscribe ack for two data signals
        payload = nlohmann::json::to_msgpack(s_subscribeAckDataSignalDoc);
        metaInformation = creataMetaInformation(payload);
        signalContainer.processMetaInformation(s_dataSignalNumber, metaInformation);

        payload = nlohmann::json::to_msgpack(s_subscribeAckAnotherDataSignalDoc);
        metaInformation = creataMetaInformation(payload);
        signalContainer.processMetaInformation(s_anotherDataSignalNumber, metaInformation);

        // subscribe ack for time signal
        payload = nlohmann::json::to_msgpack(s_subscribeAckTimeSignalDoc);
        metaInformation = creataMetaInformation(payload);
        signalContainer.processMetaInformation(s_timeSignalNumber, metaInformation);

        // all 3 deliver a signal meta with the same table id
        payload = nlohmann::json::to_msgpack(s_dataDoubleSignalMetaInformationDoc);
        metaInformation = creataMetaInformation(payload);
        result = signalContainer.processMetaInformation(s_dataSignalNumber, metaInformation);
        ASSERT_EQ(result, 0);

        payload = nlohmann::json::to_msgpack(s_anotherDoubleDataSignalMetaInformationDoc);
        metaInformation = creataMetaInformation(payload);
        result = signalContainer.processMetaInformation(s_anotherDataSignalNumber, metaInformation);
        ASSERT_EQ(result, 0);

        payload = nlohmann::json::to_msgpack(s_linearOpenDAQTimeSignalMetaInformationDoc);
        metaInformation = creataMetaInformation(payload);
        result = signalContainer.processMetaInformation(s_timeSignalNumber, metaInformation);
        ASSERT_EQ(result, 0);
    }

    TEST(SignalContainerTest, sync_measured_data_test)
    {

        std::vector < double > measuredData;
        for (size_t valueIndex = 0; valueIndex < 1024; ++valueIndex)
        {
            measuredData.push_back(valueIndex*0.1);
        }

        IndexedValue <uint64_t> indexedStartTime;
        indexedStartTime.index = 0;
        indexedStartTime.value = 20;

        ssize_t result;
        SignalContainer signalContainer(logCallback);
        std::vector < uint8_t > payload;
        MetaInformation metaInformation(logCallback);

        {
            // measured data for an unknown signal leads to error

            // process measured data and check arrival
            result = signalContainer.processMeasuredData(s_dataSignalNumber, reinterpret_cast< const uint8_t* >(measuredData.data()), measuredData.size()*sizeof(double));
            ASSERT_EQ(result, -1);
        }

        {
            // measured data for a table without time signal leads to error

            payload = nlohmann::json::to_msgpack(s_subscribeAckDataSignalDoc);
            metaInformation = creataMetaInformation(payload);
            signalContainer.processMetaInformation(s_dataSignalNumber, metaInformation);

            payload = nlohmann::json::to_msgpack(s_dataDoubleSignalMetaInformationDoc);
            metaInformation = creataMetaInformation(payload);
            signalContainer.processMetaInformation(s_dataSignalNumber, metaInformation);

            // process measured data and check arrival
            result = signalContainer.processMeasuredData(s_dataSignalNumber, reinterpret_cast< const uint8_t* >(measuredData.data()), measuredData.size()*sizeof(double));
            ASSERT_EQ(result, -1);
        }

        {
            // subscribe ack for data and time signals
            payload = nlohmann::json::to_msgpack(s_subscribeAckDataSignalDoc);
            metaInformation = creataMetaInformation(payload);
            signalContainer.processMetaInformation(s_dataSignalNumber, metaInformation);

            payload = nlohmann::json::to_msgpack(s_subscribeAckTimeSignalDoc);
            metaInformation = creataMetaInformation(payload);
            signalContainer.processMetaInformation(s_timeSignalNumber, metaInformation);

            // definition of time and data signal
            payload = nlohmann::json::to_msgpack(s_dataDoubleSignalMetaInformationDoc);
            metaInformation = creataMetaInformation(payload);
            signalContainer.processMetaInformation(s_dataSignalNumber, metaInformation);

            payload = nlohmann::json::to_msgpack(s_linearOpenDAQTimeSignalMetaInformationDoc);
            metaInformation = creataMetaInformation(payload);
            signalContainer.processMetaInformation(s_timeSignalNumber, metaInformation);


            ASSERT_EQ(signalContainer.setDataAsRawCb(dataAsRawCb), 0);
            ASSERT_EQ(signalContainer.setDataAsRawCb(DataAsRawCb()), -1);

            // set a start time
            result = signalContainer.processMeasuredData(s_timeSignalNumber, reinterpret_cast< const uint8_t* >(&indexedStartTime), sizeof(indexedStartTime));
            ASSERT_EQ(result, sizeof(indexedStartTime));

            result = signalContainer.processMeasuredData(s_dataSignalNumber, reinterpret_cast< const uint8_t* >(measuredData.data()), measuredData.size()*sizeof(double));
            ASSERT_EQ(result, measuredData.size()*sizeof(double));
            ASSERT_EQ(s_measuredDataAsDouble, measuredData);
            ASSERT_EQ(s_measuredRawDataSignalNumber, s_dataSignalNumber);
            // time stamp of first value equals start time!
            ASSERT_EQ(s_measuredRawDataTimestamp, indexedStartTime.value);

            ASSERT_EQ(s_measuredTimeSignalNumber, s_timeSignalNumber);

            ASSERT_EQ(memcmp(s_measuredRawData.data(), measuredData.data(), measuredData.size()*sizeof(double)), 0);
            ASSERT_EQ(s_measuredRawDataSignalNumber, s_dataSignalNumber);
            //ASSERT_EQ(s_measuredRawDataTimestamp, indexedStartTime.start+measuredData.size());
            // time stamp of first value equals start time!
            ASSERT_EQ(s_measuredRawDataTimestamp, indexedStartTime.value);

            result = signalContainer.processMeasuredData(s_dataSignalNumber, reinterpret_cast< const uint8_t* >(measuredData.data()), measuredData.size()*sizeof(double));
            ASSERT_EQ(result, measuredData.size()*sizeof(double));
            ASSERT_EQ(s_measuredDataAsDouble, measuredData);
            ASSERT_EQ(s_measuredRawDataSignalNumber, s_dataSignalNumber);

            ASSERT_EQ(s_measuredRawDataTimestamp, indexedStartTime.value+measuredData.size());

            ASSERT_EQ(memcmp(s_measuredRawData.data(), measuredData.data(), measuredData.size()*sizeof(double)), 0);
            ASSERT_EQ(s_measuredRawDataSignalNumber, s_dataSignalNumber);
            ASSERT_EQ(s_measuredRawDataTimestamp, indexedStartTime.value+measuredData.size());

        }

        {
            // some float values
            std::vector <float> measuredValuesFloat = { 56.7f, 45.3f, 100.0f};
            nlohmann::json dataSignaldataTypeReal32Doc = R"(
            {
                "method" : "signal",
                "params" : {
                    "definition" : {
                        "dataType" : "real32"
                    }
                }
            }
            )"_json;
            payload = nlohmann::json::to_msgpack(dataSignaldataTypeReal32Doc);
            metaInformation = creataMetaInformation(payload);
            signalContainer.processMetaInformation(s_dataSignalNumber, metaInformation);

            result = signalContainer.processMeasuredData(s_dataSignalNumber, reinterpret_cast< const uint8_t* >(measuredValuesFloat.data()), measuredValuesFloat.size()*sizeof(uint8_t));
            ASSERT_EQ(result, measuredValuesFloat.size()*sizeof(uint8_t));

            for (size_t index=0; index<s_measuredDataAsDouble.size(); ++index) {
                ASSERT_EQ(s_measuredDataAsDouble[index], measuredValuesFloat[index]);
            }

            ASSERT_EQ(s_measuredRawDataSignalNumber, s_dataSignalNumber);
        }

        {
            // some uint 8 values in two packages
            std::vector<uint8_t> measuredValuesUint8Part1 = { 15, 13, 100, 0, 6 };
            std::vector<uint8_t> measuredValuesUint8Part2 = { 16, 14, 101, 1 };
            nlohmann::json dataSignaldataTypeUintDoc = R"(
            {
                "method" : "signal",
                "params" : {
                    "definition" : {
                        "dataType" : "uint8"
                    }
                }
            }
            )"_json;
            payload = nlohmann::json::to_msgpack(dataSignaldataTypeUintDoc);
            metaInformation = creataMetaInformation(payload);
            signalContainer.processMetaInformation(s_dataSignalNumber, metaInformation);

            result = signalContainer.processMeasuredData(s_dataSignalNumber, measuredValuesUint8Part1.data(), measuredValuesUint8Part1.size()*sizeof(uint8_t));
            ASSERT_EQ(result, measuredValuesUint8Part1.size()*sizeof(uint8_t));
            ASSERT_EQ(s_measuredRawData, measuredValuesUint8Part1);
            for (size_t index=0; index<s_measuredDataAsDouble.size(); ++index) {
                ASSERT_EQ(s_measuredDataAsDouble[index], measuredValuesUint8Part1[index]);
            }
            ASSERT_EQ(s_measuredRawDataSignalNumber, s_dataSignalNumber);


            result = signalContainer.processMeasuredData(s_dataSignalNumber, measuredValuesUint8Part2.data(), measuredValuesUint8Part2.size()*sizeof(uint8_t));
            ASSERT_EQ(result, measuredValuesUint8Part2.size()*sizeof(uint8_t));
            ASSERT_EQ(s_measuredRawData, measuredValuesUint8Part2);
            for (size_t index=0; index<s_measuredDataAsDouble.size(); ++index) {
                ASSERT_EQ(s_measuredDataAsDouble[index], measuredValuesUint8Part2[index]);
            }
            ASSERT_EQ(s_measuredRawDataSignalNumber, s_dataSignalNumber);

        }

        {
            // just one uint 32 value
            uint32_t measuredValueUint32 = 89;
            nlohmann::json dataSignaldataTypeUintDoc = R"(
            {
                "method" : "signal",
                "params" : {
                    "definition" : {
                        "dataType" : "uint32"
                    }
                }
            }
            )"_json;
            payload = nlohmann::json::to_msgpack(dataSignaldataTypeUintDoc);
            metaInformation = creataMetaInformation(payload);
            signalContainer.processMetaInformation(s_dataSignalNumber, metaInformation);

            result = signalContainer.processMeasuredData(s_dataSignalNumber, reinterpret_cast< const uint8_t* >(&measuredValueUint32), sizeof(measuredValueUint32));
            ASSERT_EQ(result, sizeof(measuredValueUint32));
            ASSERT_EQ(s_measuredDataAsDouble[0], measuredValueUint32);
            ASSERT_EQ(s_measuredRawDataSignalNumber, s_dataSignalNumber);
        }

        {
            // just one int 32 value
            int32_t measuredValueint32 = -89;
            nlohmann::json dataSignaldataTypeintDoc = R"(
            {
                "method" : "signal",
                "params" : {
                    "definition" : {
                        "dataType" : "int32"
                    }
                }
            }
            )"_json;
            payload = nlohmann::json::to_msgpack(dataSignaldataTypeintDoc);
            metaInformation = creataMetaInformation(payload);
            signalContainer.processMetaInformation(s_dataSignalNumber, metaInformation);

            result = signalContainer.processMeasuredData(s_dataSignalNumber, reinterpret_cast< const uint8_t* >(&measuredValueint32), sizeof(measuredValueint32));
            ASSERT_EQ(result, sizeof(measuredValueint32));
            ASSERT_EQ(s_measuredDataAsDouble[0], measuredValueint32);
            ASSERT_EQ(s_measuredRawDataSignalNumber, s_dataSignalNumber);
        }


        {
            // just one uint 64 value
            uint64_t measuredValueUint64 = 1;
            measuredValueUint64 <<= 32;
            measuredValueUint64 += 89;
            nlohmann::json dataSignaldataTypeUint64Doc = R"(
            {
                "method" : "signal",
                "params" : {
                    "definition" : {
                        "dataType" : "uint64"
                    }
                }
            }
            )"_json;
            payload = nlohmann::json::to_msgpack(dataSignaldataTypeUint64Doc);
            metaInformation = creataMetaInformation(payload);
            signalContainer.processMetaInformation(s_dataSignalNumber, metaInformation);

            result = signalContainer.processMeasuredData(s_dataSignalNumber, reinterpret_cast< const uint8_t* >(&measuredValueUint64), sizeof(measuredValueUint64));
            ASSERT_EQ(result, sizeof(measuredValueUint64));
            ASSERT_EQ(s_measuredDataAsDouble[0], measuredValueUint64);
            ASSERT_EQ(s_measuredRawDataSignalNumber, s_dataSignalNumber);
        }

        {
            // just one int 64 value
            int64_t measuredValueint64 = -1;
            measuredValueint64 <<= 32;
            measuredValueint64 -= 89;
            nlohmann::json dataSignaldataTypeint64Doc = R"(
            {
                "method" : "signal",
                "params" : {
                    "definition" : {
                        "dataType" : "int64"
                    }
                }
            }
            )"_json;
            payload = nlohmann::json::to_msgpack(dataSignaldataTypeint64Doc);
            metaInformation = creataMetaInformation(payload);
            signalContainer.processMetaInformation(s_dataSignalNumber, metaInformation);

            result = signalContainer.processMeasuredData(s_dataSignalNumber, reinterpret_cast< const uint8_t* >(&measuredValueint64), sizeof(measuredValueint64));
            ASSERT_EQ(result, sizeof(measuredValueint64));
            ASSERT_EQ(s_measuredDataAsDouble[0], measuredValueint64);
            ASSERT_EQ(s_measuredRawDataSignalNumber, s_dataSignalNumber);
        }





        {
            // unsubscribe all signals
            payload = nlohmann::json::to_msgpack(s_unsubscribeAckDoc);
            metaInformation = creataMetaInformation(payload);
            result = signalContainer.processMetaInformation(s_dataSignalNumber, metaInformation);
            ASSERT_EQ(result, 0);
            payload = nlohmann::json::to_msgpack(s_unsubscribeAckDoc);
            metaInformation = creataMetaInformation(payload);
            result = signalContainer.processMetaInformation(s_timeSignalNumber, metaInformation);
            ASSERT_EQ(result, 0);
        }
    }







    TEST(SignalContainerTest, sync_measured_data_as_value_test)
    {

        std::vector < int32_t > measuredData;
        // more than can be handled in one block!
        for (size_t valueIndex = 0; valueIndex < 32; ++valueIndex)
        {
            measuredData.push_back(static_cast<int32_t>(valueIndex));
        }

        struct IndexedStartTime {
            uint64_t valueIndex;
            uint64_t start;
        };

        IndexedStartTime indexedStartTime;
        indexedStartTime.valueIndex = 0;
        indexedStartTime.start = 20;

        ssize_t result;
        SignalContainer signalContainer(logCallback);
        std::vector < uint8_t > payload;
        MetaInformation metaInformation(logCallback);


        {
            // subscribe ack for data and time signals
            payload = nlohmann::json::to_msgpack(s_subscribeAckDataSignalDoc);
            metaInformation = creataMetaInformation(payload);
            signalContainer.processMetaInformation(s_dataSignalNumber, metaInformation);

            payload = nlohmann::json::to_msgpack(s_subscribeAckTimeSignalDoc);
            metaInformation = creataMetaInformation(payload);
            signalContainer.processMetaInformation(s_timeSignalNumber, metaInformation);

            // definition of time and data signal
            payload = nlohmann::json::to_msgpack(s_dataInt32SignalMetaInformationDoc);
            metaInformation = creataMetaInformation(payload);
            signalContainer.processMetaInformation(s_dataSignalNumber, metaInformation);

            payload = nlohmann::json::to_msgpack(s_linearOpenDAQTimeSignalMetaInformationDoc);
            metaInformation = creataMetaInformation(payload);
            signalContainer.processMetaInformation(s_timeSignalNumber, metaInformation);

            ASSERT_EQ(signalContainer.setDataAsValueCb(DataAsValueCb()), -1);
            ASSERT_EQ(signalContainer.setDataAsValueCb(dataAsValueCb), 0);


            // set a start time
            result = signalContainer.processMeasuredData(s_timeSignalNumber, reinterpret_cast< const uint8_t* >(&indexedStartTime), sizeof(indexedStartTime));
            ASSERT_EQ(result, sizeof(indexedStartTime));

            // process measured data
            size_t measuredDataSize = measuredData.size()*sizeof(int32_t);
            result = signalContainer.processMeasuredData(s_dataSignalNumber, reinterpret_cast< const uint8_t* >(measuredData.data()), measuredDataSize);
            ASSERT_EQ(result, measuredDataSize);
            ASSERT_EQ(s_measuredDataAsInt32, measuredData);
        }
    }




    TEST(SignalContainerTest, async_measured_data_test)
    {
        uint64_t timestamp = 111;
        double measuredValue = 10;
        ssize_t result;
        SignalContainer signalContainer(logCallback);
        std::vector < uint8_t > payload;


        signalContainer.setDataAsRawCb(dataAsRawCb);

        {
            // subscribe ack for data and time signals
            payload = nlohmann::json::to_msgpack(s_subscribeAckDataSignalDoc);
            MetaInformation metaInformation = creataMetaInformation(payload);
            signalContainer.processMetaInformation(s_dataSignalNumber, metaInformation);

            payload = nlohmann::json::to_msgpack(s_subscribeAckTimeSignalDoc);
            metaInformation = creataMetaInformation(payload);
            signalContainer.processMetaInformation(s_timeSignalNumber, metaInformation);

            // definition of time and data signal
            payload = nlohmann::json::to_msgpack(s_dataDoubleSignalMetaInformationDoc);
            metaInformation = creataMetaInformation(payload);
            signalContainer.processMetaInformation(s_dataSignalNumber, metaInformation);

            payload = nlohmann::json::to_msgpack(s_explicitOpenDAQTimeSignalMetaInformationDoc);
            metaInformation = creataMetaInformation(payload);
            signalContainer.processMetaInformation(s_timeSignalNumber, metaInformation);



            // each value has its own timestamp that comes before the value of the data signal
            // set a start time
            result = signalContainer.processMeasuredData(s_timeSignalNumber, reinterpret_cast< const uint8_t* >(&timestamp), sizeof(timestamp));
            ASSERT_EQ(result, sizeof(timestamp));


            // process measured data and check arrival
            result = signalContainer.processMeasuredData(s_dataSignalNumber, reinterpret_cast< const uint8_t* >(&measuredValue), sizeof(measuredValue));
            ASSERT_EQ(result, sizeof(measuredValue));
            ASSERT_EQ(s_measuredDataAsDouble[0], measuredValue);
            ASSERT_EQ(s_measuredRawDataSignalNumber, s_dataSignalNumber);
            ASSERT_EQ(s_measuredRawDataTimestamp, timestamp);

            // unsubscribe all signals
            payload = nlohmann::json::to_msgpack(s_unsubscribeAckDoc);
            metaInformation = creataMetaInformation(payload);
            result = signalContainer.processMetaInformation(s_dataSignalNumber, metaInformation);
            ASSERT_EQ(result, 0);
            payload = nlohmann::json::to_msgpack(s_unsubscribeAckDoc);
            metaInformation = creataMetaInformation(payload);
            result = signalContainer.processMetaInformation(s_timeSignalNumber, metaInformation);
            ASSERT_EQ(result, 0);
        }
    }
}
