#include <iostream>

#include "Controller.hpp"


#include "streaming_protocol/Defines.h"
#include "streaming_protocol/ProtocolHandler.hpp"


namespace daq::streaming_protocol {

    ProtocolHandler::ProtocolHandler(boost::asio::io_context& ioc, SignalContainer& signalContainer, StreamMetaCb streamMetaCb, LogCallback logCb)
        : m_ioc(ioc)
        , m_signalContainer(signalContainer)
        , m_streamMetaCb(streamMetaCb)
        , m_streamMeta(logCb)
        , m_metaInformation(logCb)
        , logCallback(logCb)
    {
    }
    
    void ProtocolHandler::start(std::unique_ptr<daq::stream::Stream> stream, CompletionCb completionCb)
    {
        m_stream = std::move(stream);
        m_completionCb = completionCb;
        m_stream->asyncInit(std::bind(&ProtocolHandler::onInitComplete, shared_from_this(), std::placeholders::_1));
    }

    void ProtocolHandler::startWithSyncInit(std::unique_ptr<daq::stream::Stream> stream, CompletionCb completionCb)
    {
        m_completionCb = completionCb;
        m_stream = std::move(stream);
        boost::system::error_code ec =m_stream->init();
        onInitComplete(ec);
    }

    void ProtocolHandler::stop()
    {
        auto doClose = [this]() {
            closeSession(boost::system::error_code(), "");
        };
        // dispatch, because it has to be done in the io context to omit concurrent access!
        m_ioc.dispatch(doClose);
    }

    void ProtocolHandler::subscribe(const SignalIds& signalIds)
    {
        if (m_stream)
        {
            try {
                Controller controller(m_ioc, m_streamMeta.streamId(), m_stream->remoteHost(), m_streamMeta.httpControlPort(), m_streamMeta.httpControlPath(), m_streamMeta.httpVersion(), logCallback);
                controller.asyncSubscribe(signalIds, [this](const boost::system::error_code& ec) {
                    if (ec) {
                        STREAMING_PROTOCOL_LOG_E("Control request failed: {}", ec.message());
                    }
                });
            }  catch (const std::runtime_error& e) {
                STREAMING_PROTOCOL_LOG_E("{} {}: Won't subscribe!", m_stream->endPointUrl(), e.what());
            }
        }
    }
    
    void ProtocolHandler::unsubscribe(const SignalIds& signalIds)
    {
        if (m_stream)
        {
            try {
                Controller controller(m_ioc, m_streamMeta.streamId(), m_stream->remoteHost(), m_streamMeta.httpControlPort(), m_streamMeta.httpControlPath(), m_streamMeta.httpVersion(), logCallback);
                controller.asyncUnsubscribe(signalIds, [this](const boost::system::error_code& ec) {
                    if (ec) {
                        STREAMING_PROTOCOL_LOG_E("Control request failed: {}", ec.message());
                    }
                });
            } catch(const std::runtime_error& e) {
                STREAMING_PROTOCOL_LOG_E("{} {}: Won't unsubscribe!", m_stream->endPointUrl(), e.what());
            }
        }
    }

    void daq::streaming_protocol::ProtocolHandler::closeSession(const boost::system::error_code &SessionEc, char const* what)
    {
        m_sessionEc = SessionEc;
        if (SessionEc) {
            STREAMING_PROTOCOL_LOG_E("{0}: {1}", what, SessionEc.message());
        }
        if (m_stream) {
            m_stream->asyncClose(std::bind(&ProtocolHandler::onClose, shared_from_this(), std::placeholders::_1));
        }
    }
    
    void ProtocolHandler::onInitComplete(const boost::system::error_code& ec)
    {
        if(ec) {
            closeSession(ec, "stream initialization failed!");
            return;
        }
        m_stream->asyncRead(std::bind(&ProtocolHandler::onHeader, shared_from_this(), std::placeholders::_1), sizeof(m_header));
    }
    
    void ProtocolHandler::onHeader(const boost::system::error_code& ec)
    {
        if(ec) {
            closeSession(ec, "failed reading protocol header!");
            return;
        }
        m_stream->copyDataAndConsume(&m_header, sizeof(m_header));
        m_signalNumber = m_header & SIGNAL_NUMBER_MASK;
        m_type = static_cast < TransportType > ((m_header & TYPE_MASK) >> TYPE_SHIFT);
        m_length = (m_header & SIZE_MASK) >> SIZE_SHIFT;
        
        if (m_length == 0) {
            // length is to be found in additional length field
            m_stream->asyncRead(std::bind(&ProtocolHandler::onAdditionalLength, shared_from_this(), std::placeholders::_1), sizeof(m_length));
        } else {
            // read payload
            m_stream->asyncRead(std::bind(&ProtocolHandler::onPayload, shared_from_this(), std::placeholders::_1), m_length);
        }
    }
    
    void ProtocolHandler::onAdditionalLength(const boost::system::error_code& ec)
    {
        if(ec) {
            closeSession(ec, "failed reading addtional length field!");
            return;
        }
        m_stream->copyDataAndConsume(&m_length, sizeof(m_length));
        
        // read payload
        m_stream->asyncRead(std::bind(&ProtocolHandler::onPayload, shared_from_this(), std::placeholders::_1), m_length);
    }
    
    void ProtocolHandler::onPayload(const boost::system::error_code& ec)
    {
        if(ec) {
            closeSession(ec, "failed reading protocol payload!");
            return;
        }
        
        switch(m_type) {
        case TYPE_SIGNALDATA:
            if (m_signalContainer.processMeasuredData(m_signalNumber, m_stream->data(), m_length) < 0) {
                STREAMING_PROTOCOL_LOG_E("Failed to interprete measured data!");
            }
            break;
        case TYPE_METAINFORMATION:
            // instead of copying the data, we work on the data in the buffer directly.
            if (m_metaInformation.interpret(m_stream->data(), m_length)) {
                boost::system::error_code localEc = boost::system::errc::make_error_code(boost::system::errc::protocol_error);
                closeSession(localEc, "failed to interpret meta information!");
                return;
            }
            if (m_signalNumber == 0) {
                if (m_streamMeta.processMetaInformation(m_metaInformation, m_stream->endPointUrl()) < 0) {
                    boost::system::error_code localEc = boost::system::errc::make_error_code(boost::system::errc::protocol_error);
                    closeSession(localEc, "failed to interpret stream related meta information!");
                    return;
                }
                if (m_metaInformation.type()!=METAINFORMATION_MSGPACK) {
                    boost::system::error_code localEc = boost::system::errc::make_error_code(boost::system::errc::protocol_error);
                    closeSession(localEc, "unsupported meta information type");
                    return;
                }
                m_streamMetaCb(*this, m_metaInformation.method(), m_metaInformation.params());
            } else {
                if (m_signalContainer.processMetaInformation(m_signalNumber, m_metaInformation)<0 ) {
                    boost::system::error_code localEc = boost::system::errc::make_error_code(boost::system::errc::protocol_error);
                    std::string message;
                    message = "failed to interpret meta information for signal " + std::to_string(m_signalNumber) + "!";
                    closeSession(localEc, message.c_str());
                    return;
                }
            }
            break;
        default:
            {
                boost::system::error_code localEc = boost::system::errc::make_error_code(boost::system::errc::protocol_error);
                std::string message;
                message = "Received Invalid header type: " + std::to_string(m_type) +
                        ", signal number: " + std::to_string(m_signalNumber) +
                        ", length: " + std::to_string(m_length);
                closeSession(localEc, message.c_str());
                return;
            }
        }
        // Payload is processed.
        m_stream->consume(m_length);
        
        /// Now from the beginning: Next header...
        m_stream->asyncRead(std::bind(&ProtocolHandler::onHeader, shared_from_this(), std::placeholders::_1), sizeof(m_header));
    }

    void daq::streaming_protocol::ProtocolHandler::onClose(const boost::system::error_code &ec)
    {
        m_stream.reset();
        if (ec) {
            STREAMING_PROTOCOL_LOG_E("Error on close: {}", ec.message());
        }

        if(m_completionCb) {
            m_completionCb(m_sessionEc);
        }
    }
}
