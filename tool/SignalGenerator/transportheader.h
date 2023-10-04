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

#ifndef _HBM__SIGGEN__TRANSPORTHEADER_H
#define _HBM__SIGGEN__TRANSPORTHEADER_H

#include <cstddef>
#include <stdint.h>

namespace hbm {
	namespace siggen {
		enum type_t {
			TYPE_UNKNOWN = 0,
			TYPE_DATA = 1,
			TYPE_META = 2
		};

		class TransportHeader {
		public:
			TransportHeader(unsigned int id, type_t type, size_t size);

			TransportHeader(const TransportHeader&) = delete;
			TransportHeader& operator= (const TransportHeader&) = delete;

		private:
			uint32_t headerBig;
			uint32_t additionalSizeBig;
			size_t m_dataByteCount;
			unsigned int m_signalNumber;
			type_t m_type;
		};
	}
}

#endif
