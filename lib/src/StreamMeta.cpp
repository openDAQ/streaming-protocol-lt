#include <iostream>

#include "utils/strings.hpp"


#include "streaming_protocol/Defines.h"
#include "streaming_protocol/StreamMeta.hpp"

namespace daq::streaming_protocol {
StreamMeta::StreamMeta(LogCallback logCb)
    : logCallback(logCb)
{
}

int StreamMeta::processMetaInformation(const MetaInformation& metaInformation, const std::string& sessionUrl)
{
    std::string method = metaInformation.method();
    nlohmann::json params = metaInformation.params();
    try {
        // stream related meta information
        if (method == META_METHOD_APIVERSION) {
            //{
            //  "method": "apiVersion",
            //  "params": {
            //    "version": "1.0.0"
            //  }
            //}
            const auto iter = params.find(VERSION);
            if (iter != params.end()) {
                m_apiVersion = *iter;
                STREAMING_PROTOCOL_LOG_D("{}: {}:{}", sessionUrl, META_METHOD_APIVERSION, m_apiVersion);


                size_t pos = 0;
                std::string s = m_apiVersion;
                std::vector < unsigned int > tokens;
                while ((pos = s.find('.')) != std::string::npos) {
                    tokens.push_back(std::stoul(s.substr(0, pos)));
                    s.erase(0, pos + 1);
                }
                tokens.push_back(std::stoul(s));

                if (tokens.size() != 3) {
                    STREAMING_PROTOCOL_LOG_E("{}: Invalid format", META_METHOD_APIVERSION);
                    return -1;
                }

                // we expect at least 0.6.0
                if ((tokens[0] < 1) && (tokens[1] < 6)) {
                    STREAMING_PROTOCOL_LOG_E("{}: Must be at least 0.6.0! Got: {}", META_METHOD_APIVERSION, m_apiVersion);
                    return -1;
                }
            } else {
                STREAMING_PROTOCOL_LOG_E("{}: Missing version information", META_METHOD_APIVERSION);
                return -1;
            }
        } else if (method == META_METHOD_INIT) {
            // This gives important information needed to control the daq stream.
            m_streamId = params.value(META_STREAMID, "");
            STREAMING_PROTOCOL_LOG_D("{}: this is {}", sessionUrl, m_streamId);

            {
                const auto & supported_features = params.find("supported");
                if (supported_features != params.end()) {
                    for (auto& el : supported_features->object().items()) {
                        STREAMING_PROTOCOL_LOG_D("{}: supported feature: {}", sessionUrl, el.key());
                    }
                }
            }

            {
                const auto & iter = params.find("commandInterfaces");
                if (iter!=params.end()) {
                    for (const nlohmann::json& element: *iter) {
                        STREAMING_PROTOCOL_LOG_D("{}: command interfaces: {}", sessionUrl, element.dump(2));
                        static const char POST[] = "post";
                        if (strncasecmp(element["httpMethod"].get<std::string>().c_str(), POST, sizeof(POST)) == 0) {
                            m_httpControlPath = element["httpPath"];
                            std::string httpVersionString = element["httpVersion"];
                            httpVersionString.erase(std::remove(httpVersionString.begin(), httpVersionString.end(), '.'), httpVersionString.end());
                            m_httpVersion = std::stoi(httpVersionString);

                            // Do not overwrite if control port is already set.
                            if (m_httpControlPort.empty()) {
                                m_httpControlPort = element["port"];
                            }
                        }
                    }

                    STREAMING_PROTOCOL_LOG_D("http control path: {}", m_httpControlPath);
                    STREAMING_PROTOCOL_LOG_D("http control port: {}", m_httpControlPort);
                }
            }
        } else if (method == META_METHOD_ALIVE) {
            // check the fill level
            const nlohmann::json::const_iterator& fillLevelIter = params.find(META_FILLLEVEL);
            if (fillLevelIter != params.end()) {
                unsigned int fillLevel = *fillLevelIter;
                if (fillLevel >= 50) {
                    STREAMING_PROTOCOL_LOG_D("Fill level: {}", fillLevel);
                }
            }
        } else if (method == META_METHOD_AVAILABLE) {
            // handled elsewhere...
        } else if (method == META_METHOD_UNAVAILABLE) {
            // handled elsewhere...
        } else {
            // unknown stuff is ignored
            STREAMING_PROTOCOL_LOG_D("{}: Unhandled stream related meta information {}", sessionUrl, metaInformation.jsonContent().dump());
            return 0;
        }

        return 0;
    } catch(const std::runtime_error& e) {
        STREAMING_PROTOCOL_LOG_E("{}", e.what());
        return 0;
    }
}

unsigned int StreamMeta::httpVersion() const
{
    return m_httpVersion;
}

const std::string &StreamMeta::httpControlPort() const
{
    return m_httpControlPort;
}

const std::string &StreamMeta::httpControlPath() const
{
    return m_httpControlPath;
}

const std::string &StreamMeta::streamId() const
{
    return m_streamId;
}

const std::string &StreamMeta::apiVersion() const
{
    return m_apiVersion;
}
}
