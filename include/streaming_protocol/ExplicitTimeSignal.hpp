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

#include "streaming_protocol/BaseDomainSignal.hpp"
#include "streaming_protocol/iWriter.hpp"

namespace daq::streaming_protocol{
    /// \addtogroup producer
    /// Abstract base class for producing domain signal
    class ExplicitTimeSignal : public BaseDomainSignal {
    public:
        ExplicitTimeSignal(const std::string& signalId, const std::string& tableId, uint64_t timeTicksPerSecond, iWriter &writer, LogCallback logCb);

        /// A domain Signal has a Time Rule attached
        virtual RuleType getRuleType() const override;

        /// Signal meta information describes the signal. It is written once after signal got subscribed.
        void writeSignalMetaInformation() const override;

    protected:
        virtual nlohmann::json getMemberInformation() const override;
    };
}
