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
#include "streaming_protocol/SynchronousSignal.hpp"

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
    class GeneratedSynchronousSignal: public iGeneratedSignal {
    public:
        using Values = std::vector < DataType >;

        /// \param signalId Signal name. Should be unique on this server.
        /// \param tableId Signal name. Should be unique on this server.
        /// \param functionParameters Parameters describing the function
        /// \param samplePeriod Samples are taken with this rate. Not to be confused with the signal frequency!
        GeneratedSynchronousSignal(const std::string& signalId,
                                   const std::string& tableId,
                                   const std::string& timeSignalId,
                                   FunctionParameters <DataType> functionParameters,
                                   std::chrono::nanoseconds samplePeriod,
                                   const std::chrono::time_point<std::chrono::system_clock> &currentTime,
                                   const std::uint64_t valueIndex,
                                   streaming_protocol::iWriter &writer)

            : logCallback(daq::streaming_protocol::Logging::logCallback())
            , m_signal(std::make_shared<streaming_protocol::SynchronousSignal<DataType>> (signalId, tableId, writer ,logCallback, valueIndex))
            , m_function(functionParameters)
            , m_period(1/functionParameters.frequency)
            , m_samplePeriod(samplePeriod)
            , m_samplePeriodDouble(std::chrono::duration < double > (m_samplePeriod).count())
            , m_lastProcessTime(currentTime)
            , m_currentTime(currentTime)
            , m_periodTimeDouble(0)
        {
            m_signal->setRange(functionParameters.range);
            m_signal->setPostScaling(functionParameters.postScaling);
            RelatedSignals relatedSignals;
            relatedSignals[META_TIME] = timeSignalId;
            m_signal->setRelatedSignals(relatedSignals);
        }
        /// not to be copied!
        GeneratedSynchronousSignal(const GeneratedSynchronousSignal&) = delete;
        ~GeneratedSynchronousSignal() = default;
        
        virtual void initAfterSubscribeAck() override {  }

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
                    Values values(samplesToProcess);
                    
                    for(unsigned int sampleIndex = 0; sampleIndex<samplesToProcess; ++sampleIndex) {
                        values[sampleIndex] = m_function.calculate(m_periodTimeDouble);
                        
                        
                        m_periodTimeDouble += m_samplePeriodDouble;
                        if (m_periodTimeDouble>=m_period-m_samplePeriodDouble) {
                            m_periodTimeDouble = 0.0;
                        }
#if defined(__linux) && ! defined(_LIBCPP_VERSION)
						m_lastProcessTime += m_samplePeriod;
#else
						m_lastProcessTime += std::chrono::duration_cast <std::chrono::microseconds> (m_samplePeriod);
#endif
                    }
                    result = m_signal->addData(values);
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
        std::shared_ptr<streaming_protocol::SynchronousSignal <DataType>> m_signal;

        Function < DataType > m_function;
        double m_period;
        
        std::chrono::nanoseconds m_samplePeriod;
        double m_samplePeriodDouble;
        std::chrono::time_point<std::chrono::system_clock> m_lastProcessTime;
        const std::chrono::time_point<std::chrono::system_clock> &m_currentTime;
        double m_periodTimeDouble;
    };
}
