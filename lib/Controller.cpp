#include <sstream>

#include <nlohmann/json.hpp>

#include "streaming_protocol/Defines.h"
#include "streaming_protocol/jsonrpc_defines.hpp"

#include "Controller.hpp"

namespace daq::streaming_protocol {
    unsigned int Controller::s_id = 0;

    Controller::Controller(boost::asio::io_context& ioc, const std::string& streamId, const std::string& address, const std::string& port, const std::string& target, unsigned int httpVersion,
                           daq::streaming_protocol::LogCallback logCb)
        : m_ioc(ioc)
        , m_streamId(streamId)
        , m_address(address)
        , m_port(port)
        , m_target(target)
        , m_httpVersion(httpVersion)
        , logCallback(logCb)
    {
        if(m_streamId.empty()) {
            throw std::runtime_error("No stream id provided");
        }
    }

    void Controller::execute(const nlohmann::json &request, HttpPost::ResultCb resultCb)
    {
        std::string requestString = request.dump();
        auto m_httpPost = std::make_shared < HttpPost> (m_ioc, m_address, m_port, m_target, m_httpVersion, logCallback);
        m_httpPost->run(requestString, resultCb);
    }

    void Controller::asyncSubscribe(const SignalIds& signalIds, ResultCb resultCb)
    {
        if(signalIds.empty()) {
            resultCb(boost::system::error_code());
            return;
        }

        STREAMING_PROTOCOL_LOG_I(": Subscribing: =====================");
        for (const auto & iter: signalIds) {
            STREAMING_PROTOCOL_LOG_I("{}", iter);
        }

        nlohmann::json request = createRequest(signalIds, META_METHOD_SUBSCRIBE);
        execute(request, resultCb);
    }

    void Controller::asyncUnsubscribe(const SignalIds& signalIds, ResultCb resultCb)
    {
        if(signalIds.empty()) {
            resultCb(boost::system::error_code());
            return;
        }

        STREAMING_PROTOCOL_LOG_I("{} signal(s): ==============", signalIds.size());
        for (const auto & iter: signalIds) {
            STREAMING_PROTOCOL_LOG_I("{}", iter);
        }
        STREAMING_PROTOCOL_LOG_I("====================================================");

        nlohmann::json request = createRequest(signalIds, META_METHOD_UNSUBSCRIBE);
        execute(request, resultCb);
    }

    nlohmann::json Controller::createRequest(const SignalIds& signalIds, const char* method)
    {
        nlohmann::json request;
        request[daq::jsonrpc::JSONRPC] = "2.0";
        request[daq::jsonrpc::METHOD] = m_streamId +"." + method;
        for (const std::string &signalRef: signalIds) {
            request[daq::jsonrpc::PARAMS].push_back(signalRef);
        }
        request[daq::jsonrpc::ID] = ++s_id;
        return request;
    }
}
