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

#include "MetaInformation.hpp"
#include "streaming_protocol/Logging.hpp"

namespace daq::streaming_protocol {
    /// \addtogroup consumer
    /// This class interpretes and holds stream related meta information
    class StreamMeta
    {
    public:
        explicit StreamMeta(LogCallback logCb);

        /// Stream related meta information that is relevant here is interpreted
        /// \param metaInformation The received stream related meta information
        /// \param sessionUrl the unique identifier of the running session. Used for logging.
        int processMetaInformation(const MetaInformation& metaInformation, const std::string& sessionUrl);

        /// Api version as delivered by the connected producer
        const std::string& apiVersion() const;

        /// Stream id as delivered by the connected producer
        const std::string& streamId() const;

        /// Relevant for the control port, which is used to subscibre/unsubscribe signals
        const std::string& httpControlPath() const;

        /// Relevant for the control port, which is used to subscibre/unsubscribe signals
        const std::string& httpControlPort() const;

        /// Relevant for the control port, which is used to subscibre/unsubscribe signals
        /// 10 for 1.0, 11 for 1.1
        unsigned int httpVersion() const;

    private:
        std::string m_apiVersion;
        std::string m_streamId;
        std::string m_httpControlPath;
        std::string m_httpControlPort;
        unsigned int m_httpVersion;
        LogCallback logCallback;
    };
}
