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

#pragma once

#include <chrono>
#include <functional>
#include <stdint.h>
#include <string>

#include <nlohmann/json.hpp>

#include "streaming_protocol/Unit.hpp"
#include "streaming_protocol/Types.h"
#include "streaming_protocol/Logging.hpp"

#if defined(_MSC_VER)
#define ssize_t int
#endif


namespace daq::streaming_protocol {

class SubscribedSignal;

/// Delivers timestamp and the following raw data in binary form.
/// For signals with an implicit time rule, several values may be delivered.
/// For signals with an explicit time only one value may be delivered per call.
/// \note The same callback method is called for all signals. the provided parameter subscribedSignal gives information about the actual signal the values belong to!
/// \param subscribedSignal Carries lots of usefull information about the signal the data is coming from. (i.e. signal number and signal id)
/// \param timeStamp Absolute time stamp of the first value
/// \param byteCount Number of bytes delivered. Not number of values!
using DataAsRawCb = std::function<void(const SubscribedSignal& subscribedSignal, uint64_t timeStamp, const uint8_t* data, size_t byteCount)>;
/// \param valueCount Number of values delivered. Not number of bytes!
using DataAsValueCb = std::function<void(const SubscribedSignal& subscribedSignal, uint64_t timeStamp, const uint8_t* data, size_t valueCount)>;

/// \addtogroup consumer
/// Interpretes and stores meta information of a subscribed signal.
/// Measured data of a subscribed signal is processed here.
class SubscribedSignal {
public:
    SubscribedSignal(unsigned int signalNumber, LogCallback logCb);
    virtual ~SubscribedSignal() = default;

    /// process measured data
    /// \return number of bytes processed, -1 on error
    ssize_t processMeasuredData(const unsigned char* pData, size_t size, std::shared_ptr < SubscribedSignal> timeSignal, DataAsRawCb cbRaw, DataAsValueCb cbValues);

    /// process signal related meta information.
    /// \return 0 on success, -1 on error
    int processSignalMetaInformation(const std::string& method, const nlohmann::json& params);

    /// \return the unique signal number. Signals have also an unique id which is a string.
    /// For efficiency reasons, the id is delivered only once of a subscribed is acknowledged.
    /// This meta information also carries the signal number. The connection of both is kept in this class.
    unsigned int signalNumber() const
    {
        return m_signalNumber;
    }

    /// \return the textual unique identifier of the signal.
    /// It is the first received information after a signal got subscribed.
    std::string signalId() const
    {
        return m_signalId;
    }

    std::string tableId() const
    {
        return m_tableId;
    }

    bool isTimeSignal() const
    {
        return m_isTimeSignal;
    }

    RuleType ruleType() const
    {
        return m_ruleType;
    }

    SampleType dataValueType() const
    {
        return m_dataValueType;
    }

    /// \return Number of byte per value depending on the data type (i.e. 8 for double)
    size_t dataValueSize() const
    {
        return m_dataValueSize;
    }

    std::string memberName() const
    {
        return m_memberName;
    }

    /// \return timestamp of the next value of this signal
    uint64_t time() const
    {
        return m_time;
    }

    /// \return linear time delta of the signal.
    /// >0 if signal has a linear time rule. If 0 the signal does not have a linear time rule!
    uint64_t timeLinearDelta() const
    {
        return m_linearDelta;
    }

    /// \return The unit or an empty string when there is no unit
    std::string unitDisplayName() const
    {
        return m_unit.displayName;
    }

    /// \return Unit quantity of the scalar signal member
    std::string unitQuantity() const
    {
        return m_unit.quantity;
    }

    /// \return Unit id of the scalar signal member
    int32_t unitId() const
    {
        return m_unit.id;
    }

    /// Set the current timestamp for the signal. It will be used for the next appearing value.
    void setTime(uint64_t timeStamp)
    {
        m_time = timeStamp;
    }

    /// Return the time signal for this signal. Only relevant for data signals.
    std::shared_ptr<SubscribedSignal> timeSignal() const
    {
        return m_timeSignal;
    }

    /// Set the time signal for this signal
    void setTimeSignal(std::shared_ptr<SubscribedSignal> timeSignal)
    {
        m_timeSignal = timeSignal;
    }

    uint64_t linearDelta() const
    {
        return m_linearDelta;
    }

    std::string timeBaseEpochAsString() const
    {
        return m_timeBaseEpochAsString;
    }

    uint64_t timeBaseFrequency() const
    {
        return m_timeBaseFrequency;
    }

    nlohmann::json interpretationObject() const
    {
        return m_interpretationObject;
    }

    nlohmann::json bitsInterpretationObject()  const
    {
        return m_bitsInterpretationObject;
    }

    /// @param count Number of values to process, not the number of bytes!
    size_t interpretValuesAsDouble(const unsigned char* pData, size_t count, double *doubleValueBuffer) const;

    /// \return Post scaling information of the scalar signal member
    PostScaling postScaling() const
    {
        return m_postScaling;
    }

    /// \return Range of the scalar signal member
    Range range() const
    {
        return m_range;
    }

    RelatedSignals relatedSignals() const
    {
        return m_relatedSignals;
    }

    /// has to be done if start value is being set
    void clearLinearValueIndex()
    {
        m_linearValueIndex = 0;
    }

private:

    template < typename DataType >
    inline void convert(const uint8_t *pData, size_t count, double* doubleValueBuffer) const
    {
        DataType value;
        if (m_ruleType == RULETYPE_EXPLICIT) {
            // values only
            for(size_t i=0; i<count; ++i) {
                memcpy(&value, pData, sizeof (value));
                doubleValueBuffer[i] = static_cast <double> (value);
                pData += sizeof(DataType);
            }
        } else {
            // for implicit signals, each value follows after a value index
            uint64_t valueIndex;
            for(size_t i=0; i<count; ++i) {
                memcpy(&valueIndex, pData, sizeof (valueIndex));
                pData += sizeof (valueIndex);
                memcpy(&value, pData, sizeof (value));
                doubleValueBuffer[i] = static_cast <double> (value);
                pData += sizeof(DataType);
            }
        }
    }

    SignalNumber m_signalNumber;
    std::string m_signalId;
    /// The table, the signal belomgs to
    std::string m_tableId;
    bool m_isTimeSignal;

    /// contains details if data type is bitField, array or struct
    nlohmann::json m_datatypeDetails;
    nlohmann::json m_bitsInterpretationObject;
    SampleType m_dataValueType;
    size_t m_dataValueSize;

    RuleType m_ruleType;
    std::string m_memberName;

    std::shared_ptr<SubscribedSignal> m_timeSignal;
    uint64_t m_time;
    uint64_t m_linearDelta;
    size_t m_linearValueIndex;
    std::string m_timeBaseEpochAsString;
    Resolution m_resolution;
    uint64_t m_timeBaseFrequency;
    Unit m_unit;
    Range m_range;
    PostScaling m_postScaling;

    nlohmann::json m_interpretationObject;
    LogCallback logCallback;
    RelatedSignals m_relatedSignals;
};
}
