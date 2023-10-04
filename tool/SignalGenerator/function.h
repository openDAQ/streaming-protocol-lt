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

#include <cmath>
#include <cstdint>

#include "defines.h"

namespace daq::streaming_protocol::siggen{
    template < typename T >
    struct FunctionParameters {
         /// selects the function type (i.e. constant, sine)
         daq::streaming_protocol::siggen::FunctionType functionType;
         /// Amplitude of the function (offset to peak)
         T amplitude;
         /// Constant offset of the function
         T offset;
         double frequency;
         /// relevant for daq::streaming_protocol::siggen::PATTERN_TYPE_SQUARE only
         double dutyCycle;
    };

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
    class Function {
    public:
        /// \param functionType selects the function pattern (i.e. constant, sine)
        /// \param amplitude Amplitude of the signal (offset to peak)
        /// \param offset Constant offset of the signal
        /// \param dutyCycle The duty cycle is relevant for square spattern only.
        Function(FunctionParameters <DataType>functionParameters)
            : m_functionParameters(functionParameters)
            , m_rampSlope(functionParameters.amplitude*functionParameters.frequency)
        {
        }
        /// not to be copied!
        Function(const Function&) = delete;
        ~Function() = default;

        /// depending on current time and period, streaming data is being produced or not.
        /// \param periodTime point of time within one period
        DataType calculate(double periodTime)
        {
            switch(m_functionParameters.functionType) {
            /// the signal generator has one or more synthe
            case FUNCTION_TYPE_CONSTANT:
                return m_functionParameters.offset;
            case FUNCTION_TYPE_SINE:
                {
                    /// \todo check end of period.
                    static const double PI = 3.141592653589793;
                    return static_cast <DataType> (sin(2*PI*m_functionParameters.frequency*periodTime)) * m_functionParameters.amplitude + m_functionParameters.offset;
                }
            case FUNCTION_TYPE_RECTANGLE:
                {
                    // check whether we have high or low level now
                    double periodCountDouble;
                    double periodCycle = modf(periodTime * m_functionParameters.frequency, &periodCountDouble);
                    if (periodCycle<m_functionParameters.dutyCycle) {
                        return m_functionParameters.offset + m_functionParameters.amplitude;
                    } else {
                        return m_functionParameters.offset - m_functionParameters.amplitude;
                    }
                }
            case FUNCTION_TYPE_SAWTOOTH:
                {
                    return calculateSawTooth(periodTime);
                }
                break;
            case FUNCTION_TYPE_IMPULSE:
                if (periodTime==0.0) {
                    return m_functionParameters.amplitude + m_functionParameters.offset;
                } else {
                    return m_functionParameters.offset;
                }
            }
            return DataType();
        }

    private:
        DataType calculateSawTooth(double periodTime)
        {
            double rampValue = periodTime * m_rampSlope;
            return static_cast < DataType > (rampValue + m_functionParameters.offset);
        }

        FunctionParameters <DataType> m_functionParameters;
        double m_rampSlope;
    };

    template <>
    inline int32_t Function<int32_t>::calculateSawTooth(double periodTime)
    {
        int32_t rampValue = static_cast <int32_t> (std::lround(periodTime * m_rampSlope));
        return rampValue + m_functionParameters.offset;
    }

    template <>
    inline int64_t Function<int64_t>::calculateSawTooth(double periodTime)
    {
        int64_t rampValue = static_cast <int64_t> (std::lround(periodTime * m_rampSlope));
        return rampValue + m_functionParameters.offset;
    }
}
