#include "streaming_protocol/Defines.h"
#include "streaming_protocol/jsonrpc_defines.hpp"
#include "streaming_protocol/ProducerSession.hpp"
#include "streaming_protocol/Types.h"

namespace daq::streaming_protocol {
    ProducerSession::ProducerSession(std::shared_ptr<daq::stream::Stream> stream, const nlohmann::json& commandInterfaces,
                                     LogCallback logCb)
        : m_stream(stream)
        , m_writer(stream)
        , logCallback(logCb)
    {
        writeInitialMetaInformation(commandInterfaces);
    }

    void ProducerSession::start(ErrorCb errorCb)
    {
        m_errorCb = errorCb;
        doRead();
    }

    void ProducerSession::stop()
    {
        doClose();
    }

    void ProducerSession::addSignal(std::shared_ptr<BaseSignal> signal)
    {
        const std::string& signalId = signal->getId();
        m_allSignals[signalId] = signal;
        if (signal->isDataSignal()) {
            SignalIds signalIds;
            signalIds.push_back(signalId);
            writeAvailableMetaInformation(signalIds);
        }
    }

    size_t ProducerSession::subscribeSignals(const SignalIds &signalIds)
    {
        size_t count = 0;
        for (auto &signalIditer : signalIds) {
            const std::string& signalId = signalIditer;
            auto signalIter = m_allSignals.find(signalId);
            if (signalIter != m_allSignals.end()) {
                signalIter->second->subscribe();
                ++count;
            }
        }
        return count;
    }

    size_t ProducerSession::unsubscribeSignals(const SignalIds &signalIds)
    {
        size_t count = 0;
        for (auto &signalIditer : signalIds) {
            const std::string& signalId = signalIditer;
            auto signalIter = m_allSignals.find(signalId);
            if (signalIter != m_allSignals.end()) {
                signalIter->second->unsubscribe();
                ++count;
            }
        }
        return count;
    }

    void ProducerSession::writeInitialMetaInformation(const nlohmann::json& commandInterfaces)
    {
        nlohmann::json apiVersionMeta;
        apiVersionMeta[daq::jsonrpc::METHOD] = META_METHOD_APIVERSION;


        apiVersionMeta[daq::jsonrpc::PARAMS][VERSION] = OPENDAQ_LT_STREAM_VERSION;
        m_writer.writeMetaInformation(0, apiVersionMeta);

        nlohmann::json initMeta;
        initMeta[daq::jsonrpc::METHOD] = META_METHOD_INIT;
        initMeta[daq::jsonrpc::PARAMS][META_STREAMID] = m_writer.id();
        if (commandInterfaces.is_object()) {
            initMeta[daq::jsonrpc::PARAMS][COMMANDINTERFACES] = commandInterfaces;
        }
        m_writer.writeMetaInformation(0, initMeta);
    }

    void ProducerSession::writeAvailableMetaInformation(const SignalIds &signalIds)
    {
        nlohmann::json available;
        available[daq::jsonrpc::METHOD] = META_METHOD_AVAILABLE;
        for(const auto &iter : signalIds) {
            available[daq::jsonrpc::PARAMS][META_SIGNALIDS].push_back(iter);
        }
        m_writer.writeMetaInformation(0, available);
    }

    void ProducerSession::writeUnavailableMetaInformation(const SignalIds &signalIds)
    {
        nlohmann::json unavailable;
        unavailable[daq::jsonrpc::METHOD] = META_METHOD_UNAVAILABLE;
        for(const auto &iter : signalIds) {
            unavailable[daq::jsonrpc::PARAMS][META_SIGNALIDS].push_back(iter);
        }
        m_writer.writeMetaInformation(0, unavailable);
    }

    void ProducerSession::doRead()
    {
        m_stream->asyncReadSome(std::bind(&ProducerSession::onRead, shared_from_this(), std::placeholders::_1, std::placeholders::_2));
    }

    void ProducerSession::onRead(const boost::system::error_code &ec, std::size_t bytesRead)
    {
        if (ec) {
            // Stop on error.
            // Also on disconnect by the client (boost::asio::error::eof)
            // or stop by producer (boost::asio::error::operation_aborted)!
            m_errorCb(ec);
            return;
        }
        // we are not interested in the data and through it away!
        m_stream->consume(bytesRead);
        doRead();
    }

    void ProducerSession::doClose()
    {
        m_stream->asyncClose(std::bind(&ProducerSession::onClose, shared_from_this(), std::placeholders::_1));
    }

    void ProducerSession::onClose(const boost::system::error_code &ec)
    {
        if (ec) {
            STREAMING_PROTOCOL_LOG_E("Error on close: {}", ec.message());
        }
    }

    void ProducerSession::addSignals(const Signals& signals)
    {
        m_allSignals.insert(signals.begin(), signals.end());
        SignalIds signalIds;
        for (const auto& signal : signals) {
            if(signal.second->isDataSignal()) {
                signalIds.push_back(signal.first);
            }
        }

        if (!signalIds.empty()) {
            writeAvailableMetaInformation(signalIds);
        }
    }

    size_t ProducerSession::removeSignal(const std::string &signalId)
    {
        SignalIds signalIds;
        signalIds.push_back(signalId);
        return removeSignals(signalIds);
    }

    size_t ProducerSession::removeSignals(const SignalIds &signalIds)
    {
        size_t count = 0;
        SignalIds dataSignalIds;
        for (const auto& signalIdsIter: signalIds)
        {
            auto signalIter = m_allSignals.find(signalIdsIter);
            if (signalIter != m_allSignals.end()) {
                if (signalIter->second->isDataSignal()) {
                    dataSignalIds.push_back(signalIdsIter);
                }
                count += m_allSignals.erase(signalIdsIter);
            }
        }

        if (!dataSignalIds.empty()) {
            writeUnavailableMetaInformation(dataSignalIds);
        }
        return count;
    }
}
