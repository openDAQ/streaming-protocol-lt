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
#include <iostream>
#include <memory>
#include <string>


#include "streaming_protocol/BaseSignal.hpp"
#include "streaming_protocol/iWriter.hpp"
#include "streaming_protocol/LinearTimeSignal.hpp"
#include "streaming_protocol/ExplicitTimeSignal.hpp"

#include "defines.h"
#include "function.h"
#include "igeneratedsignal.h"


namespace daq::streaming_protocol::siggen{
    /// TODO add documentation comment
    template < class SignalType >
    class GeneratedTimeSignal: public iGeneratedSignal {
    public:

        /// \param name Signal name. Should be unique on this server.
        /// \param outputRate outputrate of the table. Every signal in the table delivers the same output rate
        /// \param startTime The abolute start time can be used to enable a signal later than others. Usefull for implementing a phase shift between signals
        GeneratedTimeSignal(const std::string& signalId,
                            const std::string& tableId,
                            std::chrono::nanoseconds samplePeriod,
                            uint64_t timeTicksPerSecond,
                            const std::chrono::time_point<std::chrono::system_clock> &currentTime,
                            streaming_protocol::iWriter &writer);

        /// not to be copied!
        GeneratedTimeSignal(const GeneratedTimeSignal&) = delete;
        ~GeneratedTimeSignal() = default;
        
        virtual void initAfterSubscribeAck()
        {
            // write the timestamp of the first value to follow
            uint64_t timeTicks = BaseDomainSignal::timeTicksFromTime(m_currentTime, m_signal->getTimeTicksPerSecond());
            m_signal->setTimeStart(timeTicks);
        }

        virtual std::shared_ptr <BaseSignal> getSignal() const
        {
            return m_signal;
        }

        /// depending on current time and period, streaming data is being produced or not.
        int process()
        {
            return 0;
        }
        
    private:
        daq::streaming_protocol::LogCallback logCallback;
        const std::chrono::time_point<std::chrono::system_clock> &m_currentTime;
        std::shared_ptr<streaming_protocol::BaseDomainSignal> m_signal;
    };

    template <>
    inline GeneratedTimeSignal<streaming_protocol::ExplicitTimeSignal>::GeneratedTimeSignal(
        const std::string& signalId,
        const std::string& tableId,
        std::chrono::nanoseconds samplePeriod,
        uint64_t timeTicksPerSecond,
        const std::chrono::time_point<std::chrono::system_clock> &currentTime,
        streaming_protocol::iWriter &writer)
        : logCallback(daq::streaming_protocol::Logging::logCallback())
        , m_currentTime(currentTime)
        , m_signal(std::make_shared<streaming_protocol::ExplicitTimeSignal> (signalId, tableId, timeTicksPerSecond, writer, logCallback))
    {
    }

    template <>
    inline GeneratedTimeSignal<streaming_protocol::LinearTimeSignal>::GeneratedTimeSignal(
        const std::string& signalId,
        const std::string& tableId,
        std::chrono::nanoseconds samplePeriod,
        uint64_t timeTicksPerSecond,
        const std::chrono::time_point<std::chrono::system_clock> &currentTime,
        streaming_protocol::iWriter &writer)
        : logCallback(daq::streaming_protocol::Logging::logCallback())
        , m_currentTime(currentTime)
        , m_signal(std::make_shared<streaming_protocol::LinearTimeSignal> (signalId, tableId, timeTicksPerSecond, samplePeriod, writer, logCallback))
    {
    }
}
