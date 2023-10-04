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


#include <chrono>
#include <iostream>
#include <string>

#include "nlohmann/json.hpp"

#include "streaming_protocol/common.hpp"
#include "streaming_protocol/Types.h"

#include "streaming_protocol/iWriter.hpp"
#include "streaming_protocol/BaseSignal.hpp"
#include "streaming_protocol/Logging.hpp"
#include "streaming_protocol/Unit.hpp"


BEGIN_NAMESPACE_STREAMING_PROTOCOL

template < class DataType >
struct AsyncValueTuple {
	uint64_t timeStamp;
	DataType value;
};

/// \addtogroup producer
/// Class for producing asynchronous signal data
template < class DataType >
class AsynchronousSignal : public BaseSignal {
public:

    using ValueTuples = std::vector < AsyncValueTuple < DataType >>;

    /// \param signalId Signal identifier. Should be unique on this streaming producer.
    /// \param timeFamily Describes the time ticks per second
    AsynchronousSignal(const std::string& signalId,
                       uint64_t timeTicksPerSecond, iWriter& writer, LogCallback logCb)
        : BaseSignal(signalId, timeTicksPerSecond, writer, logCb)
    {
    }

    /// not to be copied!
    AsynchronousSignal(const AsynchronousSignal&) = delete;
    ~AsynchronousSignal() = default;

    virtual SampleType getSampleType() const override;

    virtual RuleType getTimeRule() const override
    {
        return RULETYPE_EXPLICIT;
    }

    /// asynchronous values always come with a separate timestamp!
    int addData(const ValueTuples& tuples)
    {
        return addDataTable(tuples.data(), tuples.size());
    }

    int addData(const DataType* data, uint64_t* timestamps, size_t sampleCount)
    {
        createTuples(data, timestamps, sampleCount, tupleBuffer);
        return addDataTable(tupleBuffer.data(), sampleCount);
    }

private:
    ValueTuples tupleBuffer;

    int addDataTable(const AsyncValueTuple<DataType>* tuples, size_t sampleCount)
    {
        // in the tabled protocol, we need to send timestamp and value separately
        int result;

        for (size_t i = 0; i < sampleCount; i++) {
            const AsyncValueTuple<DataType>& tuple = tuples[i];

            result = m_writer.writeSignalData(m_timeSignalNumber, reinterpret_cast<const uint8_t*>(&tuple.timeStamp), sizeof(tuple.timeStamp));
            if (result < 0) {
                STREAMING_PROTOCOL_LOG_E("{}: Could not write signal timestamp!", m_timeSignalNumber);
                return result;
            }

            result = m_writer.writeSignalData(m_dataSignalNumber, reinterpret_cast<const uint8_t*>(&tuple.value), sizeof(tuple.value));
            if (result < 0) {
                STREAMING_PROTOCOL_LOG_E("{}: Could not write signal data!", m_dataSignalNumber);
                return result;
            }
        }
        return 0;
    }

    void createTuples(const DataType* data, uint64_t* timestamps, size_t sampleCount, ValueTuples& tuplesOut)
    {
        if (tuplesOut.size() < sampleCount)
            tuplesOut.resize(sampleCount);

        for (size_t i = 0; i < sampleCount; i++)
        {
            tuplesOut[i].value = data[i];
            tuplesOut[i].timeStamp = timestamps[i];
        }
    }

    nlohmann::json createMember(const std::string& dataType) const
    {
        nlohmann::json memberInformation;
        memberInformation[META_NAME] = m_valueName;
        memberInformation[META_DATATYPE] = dataType;
        memberInformation[META_RULE] = META_RULETYPE_EXPLICIT;
        if (m_unitId != Unit::UNIT_ID_NONE) {
            memberInformation[META_UNIT][META_UNIT_ID] = m_unitId;
            memberInformation[META_UNIT][META_DISPLAY_NAME] = m_unitDisplayName;
        }

        return memberInformation;
    }



    /// Signal meta information describes the signal
    void writeSignalMetaInformation() const override
    {
        nlohmann::json dataSignal;
        dataSignal[METHOD] = META_METHOD_SIGNAL;
        dataSignal[PARAMS][META_TABLEID] = m_signalId;
        dataSignal[PARAMS][META_DEFINITION] = getMemberInformation();
        if (!m_dataInterpretationObject.is_null()) {
            dataSignal[PARAMS][META_INTERPRETATION] = m_dataInterpretationObject;
        }
        m_writer.writeMetaInformation(m_dataSignalNumber, dataSignal);
        nlohmann::json timeSignal;
        timeSignal[METHOD] = META_METHOD_SIGNAL;
        timeSignal[PARAMS][META_TABLEID] = m_signalId;
        timeSignal[PARAMS][META_DEFINITION][META_NAME] = META_TIME;
        timeSignal[PARAMS][META_DEFINITION][META_RULE] = META_RULETYPE_EXPLICIT;
        timeSignal[PARAMS][META_DEFINITION][META_DATATYPE] = DATA_TYPE_UINT64;

        timeSignal[PARAMS][META_DEFINITION][META_UNIT][META_UNIT_ID] = Unit::UNIT_ID_SECONDS;
        timeSignal[PARAMS][META_DEFINITION][META_UNIT][META_DISPLAY_NAME] = "s";
        timeSignal[PARAMS][META_DEFINITION][META_TIME][META_ABSOLUTE_REFERENCE] = m_epoch;
        timeSignal[PARAMS][META_DEFINITION][META_TIME][META_RESOLUTION][META_NUMERATOR] = 1;
        timeSignal[PARAMS][META_DEFINITION][META_TIME][META_RESOLUTION][META_DENOMINATOR] = m_timeTicksPerSecond;
        if (!m_timeInterpretationObject.is_null()) {
            timeSignal[PARAMS][META_INTERPRETATION] = m_timeInterpretationObject;
        }
        m_writer.writeMetaInformation(m_timeSignalNumber, timeSignal);
    }

    nlohmann::json getMemberInformation() const;
};

END_NAMESPACE_STREAMING_PROTOCOL
