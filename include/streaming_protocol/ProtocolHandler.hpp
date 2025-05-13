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

#include <memory>

#include <boost/asio/io_context.hpp>

#include "MetaInformation.hpp"
#include "stream/Stream.hpp"
#include "SignalContainer.hpp"
#include "StreamMeta.hpp"
#include "Types.h"
#include "streaming_protocol/Logging.hpp"

namespace daq::streaming_protocol {
    /// \addtogroup consumer
    /// \todo Might be renamed to ConsumerSession"
    /// This class represents the streaming protocol consumer
    /// -Interpretes the transport protocol
    /// -Interpretes and holds stream related meta information
    /// -Delegates all signal related stuff to the provided signal container
    /// -Is responsible for subscribing/unsubscribing signals
    class ProtocolHandler: public std::enable_shared_from_this < ProtocolHandler >
    {
    public:
        /// callback function for stream related meta information. This also includes notification about signals getting available or becoming unavailable.
        using StreamMetaCb = std::function<void(ProtocolHandler& prtotocolHandler, const std::string& method, const nlohmann::json& params)>;
        using CompletionCb = std::function<void(const boost::system::error_code& ec)>;

        ProtocolHandler(boost::asio::io_context& ioc, SignalContainer& signalContainer, StreamMetaCb streamMetaCb, LogCallback logCb);
        /// after initializing the provided stream, the protocol is being received and processed until end of session or error.
        ///
        /// ----------------     ---------------     --------------------------     ----------------
        /// |              |     |             |     |                        |     |              |
        /// | init stream  |---->| read header |---->| read additional length |---->| read payload |---
        /// |              |  |  |             |  |  |  if length field = 0   |  |  |  and process |  |
        /// ----------------  |  ---------------  |  --------------------------  |  ----------------  |
        ///                   |                   |                              |                    |
        ///                   |                   --------------->---------------                     |
        ///                   |                           length field > 0                            |
        ///                   |                                                                       |
        ///                   -----------------------------------<------------------------------------
        ///                                                next package
        ///
        /// \param stream Data stream to consume data from. It delivers meta information and signal data
        void start(std::unique_ptr < daq::stream::Stream > stream, CompletionCb completionCb=CompletionCb());

        /// Init stream synchronously and start receive/process data
        void startWithSyncInit(std::unique_ptr < daq::stream::Stream > stream, CompletionCb completionCb=CompletionCb());

        void stop();

        void subscribe(const SignalIds& signalIds);
        void unsubscribe(const SignalIds& signalIds);
    private:

        /// Initiates the stream to be closed.
        /// \param SessionEc Session error code to be reported with the completion callback after closing
        void closeSession(const boost::system::error_code& SessionEc, const char *what);

        /// First read request after initalialization to request first header.
        void onInitComplete(const boost::system::error_code& ec);
        void onHeader(const boost::system::error_code& ec);
        void onAdditionalLength(const boost::system::error_code& ec);
        void onPayload(const boost::system::error_code& ec);
        /// The stream is destroyed. Completion callback is called with session error code
        void onClose(const boost::system::error_code& ec);

        boost::asio::io_context& m_ioc;
        SignalContainer& m_signalContainer;
        StreamMetaCb m_streamMetaCb;
        std::unique_ptr < daq::stream::Stream > m_stream;
        /// Will be set upon start with infomation from m_stream.
        /// We use this to omit possible race condition after reset of m_stream
        std::string m_remoteHost;
        CompletionCb m_completionCb;
        boost::system::error_code m_sessionEc;

        uint32_t m_header;
        uint32_t m_length;
        SignalNumber m_signalNumber;
        TransportType m_type;

        StreamMeta m_streamMeta;

        MetaInformation m_metaInformation;
        daq::streaming_protocol::LogCallback logCallback;
    };
}

