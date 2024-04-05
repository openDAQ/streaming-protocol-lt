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

#include <string>

#include "nlohmann/json.hpp"

#include "streaming_protocol/common.hpp"
#include "streaming_protocol/iWriter.hpp"
#include "streaming_protocol/BaseConstantSignal.hpp"

#include "Types.h"

BEGIN_NAMESPACE_STREAMING_PROTOCOL

template < class DataType >
/// \addtogroup producer
/// Class for producing constant signal data
class ConstantSignal : public BaseConstantSignal {
public:
    /// \param signalId The signal id. Unique on this streaming server.
    ConstantSignal(const std::string& signalId, const std::string& tableId, iWriter& writer, LogCallback logCb)
        : BaseConstantSignal(signalId, tableId, writer, logCb)
    {
    }

    ConstantSignal(const ConstantSignal&) = delete;
    ~ConstantSignal() = default;

    SampleType getSampleType() const override;

    int addData(const void* values, const uint64_t* indices, size_t valuesCount) override
    {
        size_t entrySize = sizeof(uint64_t) + sizeof(DataType);
        size_t dataSize = valuesCount * entrySize;
        uint8_t* signalData = static_cast<uint8_t*>(std::malloc(dataSize));

        const DataType* typedValues = static_cast<const DataType*>(values);
        for (size_t i = 0; i < valuesCount; ++i)
        {
            std::memcpy(signalData + i * entrySize, indices + i, sizeof(uint64_t));
            std::memcpy(signalData + i * entrySize + sizeof(uint64_t), typedValues + i, sizeof(DataType));
        }
        auto result = m_writer.writeSignalData(m_signalNumber, signalData, dataSize);
        std::free(signalData);

        return result;
    }

private:
    nlohmann::json getMemberInformation() const override;
};

END_NAMESPACE_STREAMING_PROTOCOL
