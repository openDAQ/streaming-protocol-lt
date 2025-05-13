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

#pragma once


#include <chrono>
#include <iostream>
#include <string>

#include "nlohmann/json.hpp"

#include "streaming_protocol/common.hpp"
#include "streaming_protocol/Types.h"

#include "streaming_protocol/iWriter.hpp"
#include "streaming_protocol/BaseValueSignal.hpp"
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
class AsynchronousSignal : public BaseValueSignal {
public:

    using ValueTuples = std::vector < AsyncValueTuple < DataType >>;

    /// \param signalId Signal identifier. Should be unique on this streaming producer.
    /// \param timeFamily Describes the time ticks per second
    AsynchronousSignal(const std::string& signalId, const std::string& tableId,
                       iWriter& writer, LogCallback logCb)
        : BaseValueSignal(signalId, tableId, writer, logCb)
    {
    }

    /// not to be copied!
    AsynchronousSignal(const AsynchronousSignal&) = delete;
    ~AsynchronousSignal() = default;

    virtual SampleType getSampleType() const override;

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

    /// asynchronous values always come with a separate timestamp!
    int addData(const ValueTuples& tuples)
    {
        // in the tabled protocol, we need to send timestamp and value separately
        int result;
        for (const auto tuple : tuples) 
        {
            result = m_writer.writeSignalData(m_signalNumber, reinterpret_cast<const uint8_t*>(&tuple.value), sizeof(tuple.value));
            if (result < 0) {
                STREAMING_PROTOCOL_LOG_E("{}: Could not write signal data!", m_signalNumber);
                return result;
            }
        }
        return 0;
    }

private:
    nlohmann::json createMember(const std::string& dataType) const
    {
        nlohmann::json memberInformation;
        memberInformation[META_NAME] = m_valueName;
        memberInformation[META_DATATYPE] = dataType;
        memberInformation[META_RULE] = META_RULETYPE_EXPLICIT;
        m_unit.compose(memberInformation);
        m_range.compose(memberInformation);
        m_postScaling.compose(memberInformation);
        return memberInformation;
    }

    /// Signal meta information describes the signal
    void writeSignalMetaInformation() const override
    {
        nlohmann::json dataSignal;
        dataSignal[METHOD] = META_METHOD_SIGNAL;
        dataSignal[PARAMS][META_TABLEID] = m_tableId;
        if (!m_interpretationObject.is_null()) {
            dataSignal[PARAMS][META_INTERPRETATION] = m_interpretationObject;
        }

        dataSignal[PARAMS][META_DEFINITION] = getMemberInformation();
        composeRelatedSignals(dataSignal[PARAMS]);
        m_writer.writeMetaInformation(m_signalNumber, dataSignal);
    }

    nlohmann::json getMemberInformation() const;
};

END_NAMESPACE_STREAMING_PROTOCOL
