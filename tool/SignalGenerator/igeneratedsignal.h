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

#include "streaming_protocol/AsynchronousSignal.hpp"
#include "streaming_protocol/BaseSignal.hpp"
#include "streaming_protocol/iWriter.hpp"
#include "streaming_protocol/Timefamily.hpp"

#include "defines.h"


namespace daq::streaming_protocol::siggen{

    static TimeFamily getTimeFamily(std::chrono::nanoseconds samplePeriod)
    {
         double samplePeriodDouble = std::chrono::duration < double > (samplePeriod).count();
         TimeFamily timeFamily;
         timeFamily.setPeriod(samplePeriodDouble);
         return timeFamily;
    }

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
    class iGeneratedSignal {
    public:
        /// To be done after subscribe acknowledge
        virtual void initAfterSubscribeAck() = 0;

        /// depending on current time and period, streaming data is being produced or not.
        /// \todo This logic has to be moved out of the streaming producer part into the signal generator part. Instead we need a method that gets the timestaamp and the value.
        virtual int process() = 0;

        virtual ~iGeneratedSignal() = default;

        virtual std::shared_ptr <BaseSignal> getSignal() const = 0;

    private:
    };
}
