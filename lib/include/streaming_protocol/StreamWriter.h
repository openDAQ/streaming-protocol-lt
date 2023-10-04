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
#include <cstddef>
#include <string>
#include <vector>

#include <nlohmann/json.hpp>

#include "stream/Stream.hpp"

#include "streaming_protocol/iWriter.hpp"
#include "streaming_protocol/Types.h"

namespace daq::streaming_protocol{
    /// Implementation of iWriter that can be used for any communication supported by libstream
    class StreamWriter: public streaming_protocol::iWriter {
    public:
        /// \param stream An initialized (i.e. connected) stream
        StreamWriter(std::shared_ptr < daq::stream::Stream > stream);
        StreamWriter(StreamWriter && ) = default;
        virtual ~StreamWriter() = default;
        StreamWriter(const StreamWriter&) = delete;

        virtual std::string id() const override;

        /// \param signalNumber 0: for stream related, >0: signal related
        int writeMetaInformation(unsigned int signalNumber, const nlohmann::json &data) override;
        /// \param signalNumber Must be > 0 since data is always signal related
        int writeSignalData(unsigned int signalNumber, const void *pData, size_t length) override;
    private:
        /// creates the transport header and the additional length field if size > 255
        /// \return depending on parameter 'size' the created header has 4 bytes or 8 bytes
        size_t createTransportHeader(TransportType type, unsigned int signalNumber, size_t size);
        int writeMsgPackMetaInformation(unsigned int signalNumber, const std::vector<uint8_t>& data);

        std::shared_ptr<daq::stream::Stream> m_stream;

        /// room for the mandatory header and the optional additional length
        uint32_t m_transportHeaderBuffer[2];
    };
}
