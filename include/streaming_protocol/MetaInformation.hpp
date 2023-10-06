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

#include <cstdint>

#include "nlohmann/json.hpp"
#include "streaming_protocol/Logging.hpp"

namespace daq::streaming_protocol {
    /// \addtogroup consumer
    /// several types of meta information may be defined.
    /// -Type 2 means any meta data encoded using MessagePack (Used in openDAQ streaming protocol.
    /// Content of the meta information can be retrieved as structured document or in binary form depending on the type.
    class MetaInformation
    {
    public:
        explicit MetaInformation(LogCallback logCb);
        MetaInformation(const MetaInformation&) = default;
        MetaInformation(MetaInformation&&) = default;
        MetaInformation& operator = (const MetaInformation&) = default;

        int interpret(const uint8_t* data, size_t size);
        std::string method() const;
        nlohmann::json params() const;

        /// \return meta data as structured data; empty if data could not be parsed
        /// \code
        /// {
        ///   ”method”: < the type of meta data >,
        ///   ”params”: < value >
        /// }
        /// \endcode
        const nlohmann::json& jsonContent() const;

        /// \return the meta information type
        uint32_t type() const;

    private:
        uint32_t m_metaInformationType;
        nlohmann::json m_jsonContent;
        LogCallback logCallback;
    };
}
