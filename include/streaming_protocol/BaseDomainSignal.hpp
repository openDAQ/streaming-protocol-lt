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
    class BaseDomainSignal : public BaseSignal {
    public:
        BaseDomainSignal(const std::string& signalId, const std::string& tableId, uint64_t timeTicksPerSecond, iWriter &writer, LogCallback logCb);

        void setEpoch(const std::string& epoch);
        void setEpoch(const std::chrono::system_clock::time_point &epoch);
        std::string getEpoch() const;

        /// A domain Signal has a rule type attached
        virtual RuleType getRuleType() const = 0;

        virtual bool isDataSignal() const final
        {
            return false;
        }

        void setTimeTicksPerSecond(uint64_t timeTicksPerSecond);
        uint64_t getTimeTicksPerSecond() const;

        /// Tell the time stamp of the next value provided by addData()
        /// To be called before sending the first value and change of time.
        /// \param timeTicks Ticks since the epoch
        void setTimeStart(uint64_t timeTicks);
        uint64_t getTimeStart() const;

        static uint64_t timeTicksFromNanoseconds(std::chrono::nanoseconds ns, uint64_t m_timeTicksPerSecond);
        static std::chrono::nanoseconds nanosecondsFromTimeTicks(uint64_t timeTicks, uint64_t m_timeTicksPerSecond);
        static uint64_t timeTicksFromTime(const std::chrono::time_point<std::chrono::system_clock> &time, uint64_t m_timeTicksPerSecond);
        /// \warning Since only C++ libraries under linux allow resolution down to nanoseconds, resolution is limited to microseconds
        static std::chrono::time_point<std::chrono::system_clock> timeFromTimeTicks(uint64_t timeTicks, uint64_t m_timeTicksPerSecond);

    protected:
        virtual nlohmann::json getMemberInformation() const = 0;

        uint64_t m_startInTicks;

        uint64_t m_timeTicksPerSecond;
        std::string m_epoch = UNIX_EPOCH;
    };
}
