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
#include <cstdint>

namespace daq::streaming_protocol::siggen{
    /// Streming server listening port
    static const uint16_t StreamingPort = 7411;
    /// \warning sample rate may not be higher then this!
    static const uint64_t timeTicksPerSecond = 1000000000;

    enum FunctionType {
        /// just the offset
        FUNCTION_TYPE_CONSTANT,

        /// offset + amplitude * sin (omega t)
        FUNCTION_TYPE_SINE,
        FUNCTION_TYPE_RECTANGLE,

        /// slope = amplitude / period
        /// \example a ramp running from 0 to 159999 in one second:
        /// amplitude = 160000
        /// offset = 0
        /// frequency = 1Hz
        FUNCTION_TYPE_SAWTOOTH,

        /// One sample with level offset + amplitude at the beginning of each cycle
        FUNCTION_TYPE_IMPULSE,
    };

    /// \compat Adding nanoseconds to a point in time does work on Linux only
    /// Other platforms convert nanoseconds to microseconds and loose some accuracy
    static inline std::chrono::system_clock::time_point addNanosecondsToTimePoint(const std::chrono::system_clock::time_point &systemTime, std::chrono::nanoseconds nanoSeconds)
    {
#ifdef _SIGNAL_GENERATOR_NANOSECOND_RESOLUTION
        return systemTime + nanoSeconds;
#elif _SIGNAL_GENERATOR_MICROSECOND_RESOLUTION
        return systemTime + std::chrono::duration_cast < std::chrono::microseconds >(nanoSeconds);
#else
        #error no valid time resolution
#endif
    }
}
