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

#include <cstdint>
#include <string>

#include "nlohmann/json.hpp"

namespace daq::streaming_protocol{
    class iWriter {
    public:
        /// Interface for writing produced data
        virtual ~iWriter() = default;

        /// \param signalNumber 0: for stream related, >0: signal related
        /// \note stream related meta information is about the complete session or the device
        /// \note signal related meta information is about this specific signal
        virtual int writeMetaInformation(unsigned int signalNumber, const nlohmann::json &data) = 0;
        /// \param signalNumber Must be > 0 since data is always signal related
        virtual int writeSignalData(unsigned int signalNumber, const void *pData, size_t length) = 0;

        virtual std::string id() const = 0;
    };
}
