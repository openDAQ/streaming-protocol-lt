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

#include <string>
#include <mutex>

#include "streaming_protocol/iWriter.hpp"
#include "streaming_protocol/Types.h"
#include "streaming_protocol/Logging.hpp"

/// \warning Since only C++ libraries under linux allow resolution down to nanoseconds, resolution is limited to microseconds
//#define TIME_GRANULARITY_NS

namespace daq::streaming_protocol{
    /// \addtogroup producer
    /// Abstrace base class for producing signal data
    class BaseSignal {
    public:
        /// \param writer The communication stream or session used to write the data to. Several signals can be added to it.
        BaseSignal(const std::string& signalId, const std::string& tableId, iWriter& writer, LogCallback logCb);
        /// not to be copied!
        BaseSignal(const BaseSignal&) = delete;
        
        virtual ~BaseSignal() = default;
        
        std::string getId() const;
        std::string getTableId() const;
        SignalNumber getNumber() const;
        virtual bool isDataSignal() const = 0;

        /// Acknowledge that signal got subscribed and send signal description according to current signal parameters.
        virtual void subscribe();
        virtual void unsubscribe();

        /// Automatically executed once on subscribe().
        /// \todo when having incremental changes this is not necessary:
        /// To be called upon change of signal description.
        virtual void writeSignalMetaInformation() const = 0;

        void setInterpretationObject(const nlohmann::json object);
        nlohmann::json getInterpretationObject() const;

    protected:

        static SignalNumber nextSignalNumber();

        SignalNumber m_signalNumber;
        /// on presentation layer, each signal is identified by its signal id
        std::string m_signalId;
        std::string m_tableId;

        nlohmann::json m_interpretationObject;

        iWriter& m_writer;
        LogCallback logCallback;

        static SignalNumber s_signalNumberCounter;
        static std::mutex s_signalNumberMtx;
    };
}
