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
#include "streaming_protocol/BaseSynchronousSignal.hpp"

#include "Types.h"

BEGIN_NAMESPACE_STREAMING_PROTOCOL

template < class DataType >
/// \addtogroup producer
/// Class for producing asynchronous signal data
class SynchronousSignal : public BaseSynchronousSignal {
public:
	using Values = std::vector < DataType >;

	/// \param id The signal id. Unique on this streaming server.
	/// \param name Signal name. Should be unique on this server.
	/// \param outputRate Values are created with this rate.
	/// \param timeFamily Describes the time ticks per second
        SynchronousSignal(const std::string& signalId, const std::string& tableId, iWriter& writer, LogCallback logCb, std::uint64_t valueIndex = 0)
                : BaseSynchronousSignal(signalId, tableId, writer, logCb, valueIndex)
	{
	}

	/// not to be copied!
	SynchronousSignal(const SynchronousSignal&) = delete;
	~SynchronousSignal() = default;

	virtual SampleType getSampleType() const override;

	/// synchronous values come without a timestamp!
	/// \note Call setStartTime() and setOutputRate() once before calling this. Otherwise the signal is not completely described and can not interpreted by the consumer.
	int addData(const Values& values)
	{
	    return addData(values.data(), values.size());
	}

	virtual int addData(const void* data, size_t sampleCount) override
	{
	    m_valueIndex += sampleCount;
	    return m_writer.writeSignalData(m_signalNumber, (uint8_t*)data, sampleCount * sizeof(DataType));
	}

private:

    virtual nlohmann::json getMemberInformation() const override;
};

END_NAMESPACE_STREAMING_PROTOCOL
