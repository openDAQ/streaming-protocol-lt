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

#include <map>

#include "nlohmann/json.hpp"

#include "stream/Stream.hpp"

#include "streaming_protocol/BaseSignal.hpp"
#include "streaming_protocol/StreamWriter.h"
#include "streaming_protocol/Logging.hpp"

namespace daq::streaming_protocol {
    /// When a client connects to the streaming server, a producer session is created to handle the client.
    /// On construction it will:
    /// -Tell about the streaming protocol version
    /// -Tell how to reach the control service (when there is any)
    class ProducerSession : public std::enable_shared_from_this<ProducerSession>
    {
    public:
        /// \addtogroup producer
        /// signal id is the key
        using Signals = std::map < std::string, std::shared_ptr < BaseSignal>>;
        using ErrorCb = std::function < void (const boost::system::error_code&) >;

        /// \param stream used for writing meta information or data
        /// \param commandInterfaces information telling how to connect to control service may be empty
        /// for subscribing/unsubscribing signals
        /// \code
        /// {
        ///   "jsonrpc-http": {
        ///     "httpMethod": "POST",
        ///     "httpPath": "controlUrlPath",
        ///     "httpVersion": "1.0",
        ///     "port": "http"
        ///   }
        /// }
        /// \endcode
        ProducerSession(std::shared_ptr<daq::stream::Stream> stream, const nlohmann::json& commandInterfaces,
                        LogCallback logCb);
        ~ProducerSession() = default;

        /// Starts reading from stream, data received will be consumed and thrown away.
        /// On error stream is being reset and errorCb is called.
        void start(ErrorCb errorCb);
        void stop();

        /// Add a reference to signal to the session
        /// If it is a data signal, "Available" meta information is send for this signal id.
        /// subscribeSignals() has to be called afterwards to tell that the signal 
        /// is about to deliver data
        void addSignal(std::shared_ptr <BaseSignal> signal);

        /// Add several signals to the session
        /// "Available" meta information is send for all signals
        /// \param signals Colledtion of signals and their corresponding signal id
        void addSignals(const Signals& signals);

        /// Removes the reference to signal from the session. Does not destroy the signal itself!
        /// If it is a data signal, "Unavailable" meta information is send for this signal
        /// \return 1 if signal got removed succesfully
        size_t removeSignal(const std::string& signalId);

        /// Removes the reference to several signals from the session. Does not destroy the signals itself!
        /// "Unavailable" meta information is send for all signal ids of data signals
        /// \return number of signal succesfully removed
        size_t removeSignals(const SignalIds& signalIds);

        /// Tell that streaming starts for mentioned signals.
        /// Data is send by calling addData() of the actual signal.
        /// \return number of signals succesfully subscribed
        size_t subscribeSignals(const SignalIds& signalIds);

        /// Tell that streaming stops for mentioned signals.
        /// \return number of signals succesfully unsubscribed
        size_t unsubscribeSignals(const SignalIds& signalIds);
    private:
        /// Writes stream version (META_METHOD_APIVERSION)
        /// \note This has to happen at the very beginning of the session!
        /// Writes init meta information that tells about command interfaces and the session id.
        /// Both are important to subscribe/unsubscribe signals.
        /// \param commandInterfaces information telling how to connect to control service
        /// for subscribing/unsubscribing signals
        void writeInitialMetaInformation(const nlohmann::json& commandInterfaces);

        void writeAvailableMetaInformation(const SignalIds &signalIds);
        void writeUnavailableMetaInformation(const SignalIds &signalIds);

        void doRead();
        void onRead(const boost::system::error_code& ec, std::size_t bytesRead);

        void doClose();
        void onClose(const boost::system::error_code& ec);

        std::shared_ptr<daq::stream::Stream> m_stream;
        StreamWriter m_writer;
        Signals m_allSignals;
        ErrorCb m_errorCb;
        LogCallback logCallback;
    };
}
