#include <cstring>
#include <iostream>


#include <nlohmann/json.hpp>

#include "streaming_protocol/Defines.h"
#include "streaming_protocol/MetaInformation.hpp"

namespace daq::streaming_protocol {
    MetaInformation::MetaInformation(LogCallback logCb)
        : m_metaInformationType(0)
        , m_jsonContent()
        , logCallback(logCb)
    {
    }

    int MetaInformation::interpret(const uint8_t* data, size_t size)
    {
        memcpy(&m_metaInformationType, data, sizeof(m_metaInformationType));
        switch (m_metaInformationType) {
        // we do support messagepack only!
        case METAINFORMATION_MSGPACK:
            {
                try {
                    m_jsonContent = nlohmann::json::from_msgpack(data+sizeof(m_metaInformationType), data+size);
                } catch (const nlohmann::json::parse_error& e) {
                    STREAMING_PROTOCOL_LOG_E("parsing meta information failed : {}", e.what());
                    return -1;
                }
            }
            break;
        default:
            // we do ignore stuff we don't understand
            return 0;
        }
        return 0;
    }

    std::string MetaInformation::method() const
    {
        const nlohmann::json::const_iterator& methodNode = m_jsonContent.find(METHOD);
        if (methodNode == m_jsonContent.end()) {
            return "";
        }
        return *methodNode;
    }

    nlohmann::json MetaInformation::params() const
    {
        const nlohmann::json::const_iterator& paramsNode = m_jsonContent.find(PARAMS);
        if (paramsNode == m_jsonContent.end()) {
            return {};
        }
        return *paramsNode;
    }

    const nlohmann::json &MetaInformation::jsonContent() const
    {
        return m_jsonContent;
    }

    uint32_t MetaInformation::type() const
    {
        return m_metaInformationType;
    }
}
