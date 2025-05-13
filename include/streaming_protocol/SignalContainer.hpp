/*
 * Copyright 2022-2025 openDAQ d.o.o.
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
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>

#include <nlohmann/json.hpp>

#include "SubscribedSignal.hpp"
#include "Table.hpp"
#include "Types.h"
#include "streaming_protocol/Logging.hpp"

namespace daq::streaming_protocol {
    class MetaInformation;

    /// \note The same callback method is called for all meta information related to any signal. The provided parameter subscribedSignal gives information about the actual signal the meta onformation belong to!
    /// \param subscribedSignal Carries lots of usefull information about the signal the data is coming from. (i.e. signal number and signal id). It also carries signal number and signal id to identify the signal.
    using SignalMetaCb = std::function<void(SubscribedSignal& subscribedSignal, const std::string& method, const nlohmann::json& params)>;

    /// \addtogroup consumer
    /// This class is used by stream consumers
    /// -It contains all subscribed signals.
    /// -It interpretes all signal related meta information
    /// -Callback functions may be registered in order to get informed about signal related meta information and measured data.
    class SignalContainer {
    public:
        explicit SignalContainer(LogCallback logCb);
        SignalContainer(const SignalContainer& op) = delete;
        SignalContainer& operator=(const SignalContainer& op) = delete;

        /// \warning set callback function before calling start(), otherwise you will miss meta information received.
        int setSignalMetaCb(SignalMetaCb cb);

        /// \warning set callback function before subscribing signals, otherwise you will miss measured values received.
        int setDataAsRawCb(DataAsRawCb cb);
        
        /// \warning set callback function before subscribing signals, otherwise you will miss measured values received.
        int setDataAsValueCb(DataAsValueCb cb);

        /// new subscribed signals are added with arrival of subscribe acknowledge
        int processMetaInformation(SignalNumber signalNumber, const MetaInformation& metaInformation);

        /// \param data Data to process
        /// \param len Number of bytes to process
        /// \return number of bytes processed or -1 if signal is unknown.
        /// \note We require to be called with complete values only!
        ssize_t processMeasuredData(SignalNumber signalNumber, const unsigned char* data, size_t len);

    private:
        /// Signal number is the key
        using Signals = std::unordered_map < SignalNumber, std::shared_ptr < SubscribedSignal > >;

        /// Table id is the key
        using Tables = std::unordered_map < std::string, Table >;

        /// signal number of the status signal is the key, signal id id of the datat signal is the value
        using StatusSources = std::unordered_map < SignalNumber, std::string >;


        /// processes measured data and keeps meta information about all subscribed signals
        Signals m_subscribedSignals;
        Tables m_tables;
        StatusSources m_statusSources;

        SignalMetaCb m_signalMetaCb;
        DataAsRawCb m_dataAsRawCb;
        DataAsValueCb m_dataAsValueCb;
        LogCallback logCallback;
    };
}
