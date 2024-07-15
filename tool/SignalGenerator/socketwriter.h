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
#include <cstddef>
#include <string>

#include "hbm/communication/socketnonblocking.h"

#include "iwriter.h"

namespace hbm::siggen{
	class SocketWriter: public iWriter {
	public:
		SocketWriter(hbm::communication::clientSocket_t socket, bool hbkMode);
		SocketWriter(SocketWriter && ) = default;
		virtual ~SocketWriter() = default;
		SocketWriter(const SocketWriter&) = delete;

		int writeMetaInformation(unsigned int signalNumber, const nlohmann::json &data);
		/// \param signalNumber 0: for stream related, >0: signal related
		int writeSignalData(unsigned int signalNumber, uint8_t *pData, size_t length);
	private:
		/// creates the transport header and the additional length field if size > 255
		/// \return depending on parameter 'size' the created header has 4 bytes or 8 bytes
		size_t createTransportHeader(TransportType type, unsigned int signalNumber, size_t size);
		int writeJsonMetaInformationString(unsigned int signalNumber, const std::string& data);
		int writeMsgPackMetaInformationString(unsigned int signalNumber, const std::vector < uint8_t >& data);

		hbm::communication::clientSocket_t m_pSocket;
		bool m_hbkMode;

		/// room for the mandatory header and the optional additional length
		uint32_t m_transportHeaderBuffer[2];
		hbm::communication::dataBlock_t m_datablocks[3];
	};
}
