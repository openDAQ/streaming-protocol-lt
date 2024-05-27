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

#include "streaming_protocol/iWriter.hpp"
#include "streaming_protocol/AsynchronousSignal.hpp"

#include "defines.h"
#include "function.h"
#include "igeneratedsignal.h"


namespace daq::streaming_protocol::siggen{

    /// A synthetic signal delivers meta information describing the properties of the signal a synthetic measured data.
    /// The properties:
    /// * frequency
    /// * enumeration for the pattern type (constant, sine, square, saw tooth...)
    /// * parameters specific for the pattern type (amplitude, offset, frequency, duty cycle)
    /// We set all properties on creation of the signal.
    /// DataType can of the following types:
    /// * float
    /// * double
    /// * int32_t
    template < class DataType >
    class GeneratedAsynchronousSignal: public iGeneratedSignal {
    public:
        /// \param signalId Signal identifier. Should be unique on this streaming producer.
        /// \param functionParameters Parameters describing the function
        /// \param samplePeriod Samples are taken with this rate. Not to be confused with the signal frequency!
        /// \param startTime The abolute start time can be used to enable a signal later than others. Usefull for implementing a phase shift between signals
        GeneratedAsynchronousSignal(const std::string& signalId,
                                    const std::string& tableId,
                                    const std::string& timeSignalId,
                                    FunctionParameters <DataType> functionParameters,
                                    std::chrono::nanoseconds samplePeriod,
                                    const std::chrono::time_point<std::chrono::system_clock> &currentTime,
                                    streaming_protocol::iWriter &writer)
            : logCallback(daq::streaming_protocol::Logging::logCallback())
            , m_signal(std::make_shared<AsynchronousSignal <DataType>> (signalId, tableId, writer, logCallback))
            , m_function(functionParameters)
            , m_period(1/functionParameters.frequency)
            , m_samplePeriod(samplePeriod)
            , m_samplePeriodDouble(std::chrono::duration < double > (m_samplePeriod).count())
            , m_lastProcessTime(currentTime)
            , m_currentTime(currentTime)
            , m_periodTimeDouble(0)
        {

            auto duration = std::chrono::duration<double> (m_lastProcessTime.time_since_epoch());
            double timeSinceUnixEpochDouble = duration.count();

            m_deltaTime = 1/m_samplePeriodDouble;
            m_timestamp = timeTicksPerSecond * timeSinceUnixEpochDouble;

            m_signal->setRange(functionParameters.range);
            m_signal->setPostScaling(functionParameters.postScaling);
            RelatedSignals relatedSignals;
            relatedSignals[META_TIME] = timeSignalId;
            m_signal->setRelatedSignals(relatedSignals);
        }
        /// not to be copied!
        GeneratedAsynchronousSignal(const GeneratedAsynchronousSignal&) = delete;
        ~GeneratedAsynchronousSignal() = default;

        virtual void initAfterSubscribeAck()
        {
            // nothing to do for asynchronous signal
        }

        virtual std::shared_ptr <BaseSignal> getSignal() const
        {
            return m_signal;
        }

        /// depending on current time and period, streaming data is being produced or not.
        int process()
        {
            int result = 0;
            if (m_currentTime>=m_lastProcessTime) {
                    // samples since last processing took place
                    auto microseconds = m_currentTime - m_lastProcessTime;
                    unsigned int samplesToProcess = static_cast < unsigned int > (microseconds / m_samplePeriod);

                    if (samplesToProcess) {
                        std::vector < streaming_protocol::AsyncValueTuple < DataType >> tuples(samplesToProcess);
                            for(unsigned int sampleIndex = 0; sampleIndex<samplesToProcess; ++sampleIndex) {
                                    tuples[sampleIndex].value = m_function.calculate(m_periodTimeDouble);
                                    tuples[sampleIndex].timeStamp = m_timestamp;
                                    m_timestamp += m_deltaTime;

                                    m_periodTimeDouble += m_samplePeriodDouble;
                                    if (m_periodTimeDouble>=m_period-m_samplePeriodDouble) {
                                            m_periodTimeDouble = 0.0;
                                    }
                                    m_lastProcessTime = addNanosecondsToTimePoint(m_lastProcessTime, m_samplePeriod);
                            }
                            result = m_signal->addData(tuples);
                            if (result<0) {
                                STREAMING_PROTOCOL_LOG_E("{}: Could not write signal data. Aborting!", m_signal->getNumber());
                                return result;
                            }
                        }
                }
            return result;
        }

    private:
        daq::streaming_protocol::LogCallback logCallback;
        std::shared_ptr < AsynchronousSignal <DataType> > m_signal;

        Function < DataType > m_function;
        double m_period;

        std::chrono::nanoseconds m_samplePeriod;
        double m_samplePeriodDouble;
        std::chrono::time_point<std::chrono::system_clock> m_lastProcessTime; /// can be in the future to allow starting later
        const std::chrono::time_point<std::chrono::system_clock> &m_currentTime;
        double m_periodTimeDouble;
        uint64_t m_timestamp;
        uint64_t m_deltaTime;
    };
}
