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

#include <cstdint>
#include <iostream>

#ifdef _WIN32
#include <WinSock2.h>
#else
#include <arpa/inet.h>
#endif

#include "hbm/communication/socketnonblocking.h"

#include "defines.h"
#include "socketwriter.h"

//#define VERBOSE


namespace hbm {
	namespace siggen{

		SocketWriter::SocketWriter(hbm::communication::clientSocket_t pSocket, bool hbkMode)
			: m_pSocket(std::move(pSocket))
			, m_hbkMode(hbkMode)
		{
		}

		int SocketWriter::writeMetaInformation(unsigned int signalNumber, const nlohmann::json &data)
		{
			if (m_hbkMode) {
				std::vector < uint8_t > msgpack = nlohmann::json::to_msgpack(data);
				return writeMsgPackMetaInformationString(signalNumber, msgpack);
			} else {
				std::string dataString = data.dump();
				return writeJsonMetaInformationString(signalNumber, dataString);
			}
		}


		int SocketWriter::writeJsonMetaInformationString(unsigned int signalNumber, const std::string& data)
		{
			// used by HBM streaming only which uses big endian!
			static const uint32_t bigMetaType = htonl(META_TYPE_JSON);
			size_t headerSize = createTransportHeader(TYPE_METAINFORMATION, signalNumber, data.length() + sizeof (bigMetaType));
			/// 3 parts: transport header, meta information type, meta information payload
			m_datablocks[0].pData = &m_transportHeaderBuffer[0];
			m_datablocks[0].size = headerSize;
			m_datablocks[1].pData = reinterpret_cast< const char*>(&bigMetaType);
			m_datablocks[1].size = sizeof (bigMetaType);
			m_datablocks[2].pData = data.c_str();
			m_datablocks[2].size = data.length();
			return static_cast < int > (m_pSocket->sendBlocks(m_datablocks, 3, false));
		}

		int SocketWriter::writeMsgPackMetaInformationString(unsigned int signalNumber, const std::vector < uint8_t >& data)
		{
			// used by HBK streaming only which uses little endian!
			static const uint32_t littleMetaType = META_TYPE_MSGPACK;
			size_t headerSize = createTransportHeader(TYPE_METAINFORMATION, signalNumber, data.size() + sizeof (littleMetaType));
			/// 3 parts: transport header (evtl. with optional additional length), meta information type, meta information payload
			m_datablocks[0].pData = &m_transportHeaderBuffer[0];
			m_datablocks[0].size = headerSize;
			m_datablocks[1].pData = reinterpret_cast< const char*>(&littleMetaType);
			m_datablocks[1].size = sizeof (littleMetaType);
			m_datablocks[2].pData = &data[0];
			m_datablocks[2].size = data.size();
			return static_cast < int > (m_pSocket->sendBlocks(m_datablocks, 3, false));
		}


		int SocketWriter::writeSignalData(unsigned int signalNumber, uint8_t *pData, size_t length)
		{
			size_t headerSize = createTransportHeader(TYPE_SIGNALDATA, signalNumber, length);
			/// 2 parts: transport header (evtl. with optional additional length), signal data payload
			m_datablocks[0].pData = &m_transportHeaderBuffer[0];
			m_datablocks[0].size = headerSize;
			m_datablocks[1].pData = pData;
			m_datablocks[1].size = length;
			return static_cast < int > (m_pSocket->sendBlocks(m_datablocks, 2, false));
		}

		size_t SocketWriter::createTransportHeader(TransportType type, unsigned int signalNumber, size_t size)
		{
			// lower 20 bit are signal id
			uint32_t header = signalNumber;
			header |= static_cast < uint32_t > (type << HEADER_TYPE_SHIFT);
			if (size<=256) {
				header |= size << HEADER_SIZE_SHIFT;
				if (m_hbkMode) {
					m_transportHeaderBuffer[0] = header;
				} else {
					m_transportHeaderBuffer[0] = htonl(header);
				}
#ifdef VERBOSE
				std::cout << "transport header for signal " << id << " with " << size << " byte\n";
#endif
				// size of the transport header
				return sizeof(uint32_t);
			} else {
				if (m_hbkMode) {
					m_transportHeaderBuffer[0] = header;
					m_transportHeaderBuffer[1] = static_cast < uint32_t > (size);
				} else {
					m_transportHeaderBuffer[0] = htonl(header);
					m_transportHeaderBuffer[1] = htonl(static_cast < uint32_t > (size));
				}
#ifdef VERBOSE
				std::cout << "transport header for signal " << id << " with " << size << " byte\n";
#endif
				// size of the transport header + size of additional length field
				return 2*sizeof(uint32_t);
			}
		}
	}
}


