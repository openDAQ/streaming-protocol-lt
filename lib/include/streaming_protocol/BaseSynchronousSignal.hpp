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

#include "streaming_protocol/BaseSignal.hpp"
#include "streaming_protocol/iWriter.hpp"

namespace daq::streaming_protocol{
    /// \addtogroup producer
    /// Abstrace base class for producing signal data
    class BaseSynchronousSignal : public BaseSignal {
    public:
        BaseSynchronousSignal(const std::string& signalId, uint64_t outputRate, uint64_t timeTicksPerSecond, iWriter &writer, LogCallback logCb);

        virtual int addData(const void* data, size_t sampleCount) = 0;

        virtual RuleType getTimeRule() const override;
        uint64_t getTimeDelta() const;
        uint64_t getTimeStart() const;

        /// Tell the time stamp of the next value provided by addData()
        /// To be called before sending the first value and change of time.
        /// \param timeTicks Ticks since the epoch
        void setTimeStart(uint64_t timeTicks);

        /// Tell the time between two values
        /// To be called before sending the first value and on change of the output rate.
        void setOutputRate(uint64_t timeTicks);

        /// Signal meta information describes the signal. It is written once after signal got subscribed.
        void writeSignalMetaInformation() const override;

    protected:

        nlohmann::json createMember(const std::string& dataType) const;

        virtual nlohmann::json getMemberInformation() const = 0;

        uint64_t m_startInTicks;
        uint64_t m_outputRateInTicks;
        uint64_t m_valueIndex;
    };
}
