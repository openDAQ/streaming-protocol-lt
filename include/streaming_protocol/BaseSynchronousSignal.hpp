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

#include "streaming_protocol/BaseValueSignal.hpp"
#include "streaming_protocol/iWriter.hpp"

namespace daq::streaming_protocol{
    /// \addtogroup producer
    /// Abstract base class for producing signal data
    class BaseSynchronousSignal : public BaseValueSignal {
    public:
        BaseSynchronousSignal(const std::string& signalId, const std::string& tableId, iWriter &writer, LogCallback logCb, std::uint64_t valueIndex);

        virtual int addData(const void* data, size_t sampleCount) = 0;

        uint64_t getValueIndex()
        {
            return m_valueIndex;
        }

        void setValueIndex(uint64_t valueIndex)
        {
            m_valueIndex = valueIndex;
        }

        /// Signal meta information describes the signal. It is written once after signal got subscribed.
        void writeSignalMetaInformation() const override;

    protected:

        nlohmann::json createMember(const std::string& dataType) const;

        virtual nlohmann::json getMemberInformation() const = 0;
        
        uint64_t m_valueIndex;
    };
}
