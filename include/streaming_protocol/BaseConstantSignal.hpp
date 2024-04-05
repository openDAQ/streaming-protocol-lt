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

#include "streaming_protocol/BaseValueSignal.hpp"
#include "streaming_protocol/iWriter.hpp"

namespace daq::streaming_protocol{
/// \addtogroup producer
/// Abstrace base class for producing constant rule signal data
class BaseConstantSignal : public BaseValueSignal {
public:
    BaseConstantSignal(const std::string& signalId, const std::string& tableId, iWriter &writer, LogCallback logCb);

    virtual int addData(const void* values, const uint64_t* indices, size_t valuesCount) = 0;

    /// Signal meta information describes the signal. It is written once after signal got subscribed.
    void writeSignalMetaInformation() const override;

protected:

    nlohmann::json createMember(const std::string& dataType) const;
    virtual nlohmann::json getMemberInformation() const = 0;
};

}
