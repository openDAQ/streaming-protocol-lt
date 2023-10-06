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
#include <string>
#include <mutex>

#include "streaming_protocol/Defines.h"
#include "streaming_protocol/iWriter.hpp"
#include "streaming_protocol/Types.h"
#include "streaming_protocol/Logging.hpp"

/// \warning Since only C++ libraries under linux allow resolution down to nanoseconds, resolution is limited to microseconds
//#define TIME_GRANULARITY_NS

namespace daq::streaming_protocol{
    /// \addtogroup producer
    /// Abstrace base class for producing signal data
    class BaseSignal {
    public:
        /// \param writer The communication stream or session used to write the data to. Several signals can be added to it.
        BaseSignal(const std::string& signalId, uint64_t timeTicksPerSecond, iWriter& writer, LogCallback logCb);
        /// not to be copied!
        BaseSignal(const BaseSignal&) = delete;
        
        virtual ~BaseSignal() = default;
        
        std::string getId() const;
        SignalNumber getNumber() const;

        virtual SampleType getSampleType() const = 0;

        /// Each signal has a time signal attached
        virtual RuleType getTimeRule() const = 0;

        /// Acknowledge that signal got subscribed and send signal description according to current signal parameters.
        virtual void subscribe();
        virtual void unsubscribe();

        /// Automatically executed once on subscribe().
        /// \todo when having incremental changes this is not necessary:
        /// To be called upon change of signal description.
        virtual void writeSignalMetaInformation() const = 0;

        void setEpoch(const std::string& epoch);
        void setEpoch(const std::chrono::system_clock::time_point &epoch);
        std::string getEpoch() const;
        /// \param name Name of the root data member
        void setMemberName(const std::string& name);
        /// \return Name of the root data member
        std::string getMemberName() const;

        int32_t getUnitId() const;
        std::string getUnitDisplayName() const;

        void setTimeTicksPerSecond(uint64_t timeTicksPerSecond);
        uint64_t getTimeTicksPerSecond() const;

        //// \param unitId Unit::UNIT_ID_NONE for no unit
        void setUnit(int32_t unitId, const std::string& displayName);
        void setDataInterpretationObject(const nlohmann::json object);
        nlohmann::json getDataInterpretationObject() const;
        void setTimeInterpretationObject(const nlohmann::json object);
        nlohmann::json getTimeInterpretationObject() const;


        static uint64_t timeTicksFromNanoseconds(std::chrono::nanoseconds ns, uint64_t m_timeTicksPerSecond);
        static std::chrono::nanoseconds nanosecondsFromTimeTicks(uint64_t timeTicks, uint64_t m_timeTicksPerSecond);
        static uint64_t timeTicksFromTime(const std::chrono::time_point<std::chrono::system_clock> &time, uint64_t m_timeTicksPerSecond);
        /// \warning Since only C++ libraries under linux allow resolution down to nanoseconds, resolution is limited to microseconds
        static std::chrono::time_point<std::chrono::system_clock> timeFromTimeTicks(uint64_t timeTicks, uint64_t m_timeTicksPerSecond);

    protected:

        static SignalNumber nextSignalNumber();

        SignalNumber m_dataSignalNumber;
        SignalNumber m_timeSignalNumber;
        /// on presentation layer, each signal is identified by its signal id
        std::string m_signalId;
        std::string m_valueName;

        int32_t m_unitId;
        std::string m_unitDisplayName;
        nlohmann::json m_dataInterpretationObject;
        nlohmann::json m_timeInterpretationObject;

        uint64_t m_timeTicksPerSecond;
        std::string m_epoch = UNIX_EPOCH;
        iWriter& m_writer;
        LogCallback logCallback;

        static SignalNumber s_signalNumberCounter;
        static std::mutex s_signalNumberMtx;
    };
}
