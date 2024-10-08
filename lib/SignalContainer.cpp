#include <iostream>

#include <nlohmann/json.hpp>

#include "streaming_protocol/Defines.h"
#include "streaming_protocol/MetaInformation.hpp"
#include "streaming_protocol/SignalContainer.hpp"

#include "streaming_protocol/SubscribedSignal.hpp"

namespace daq::streaming_protocol {
    /// default callback does nothing...
    void nopSignalMetaCb(const SubscribedSignal&, const std::string&, const nlohmann::json&)
    {
    }
    
    /// default callback does nothing...
    void nopDataCb(const SubscribedSignal&, uint64_t, const uint8_t*, size_t)
    {
    }
    
    
    
SignalContainer::SignalContainer(LogCallback logCb)
    : m_subscribedSignals()
    , m_signalMetaCb(nopSignalMetaCb)
    , m_dataAsRawCb(nopDataCb)
    , m_dataAsValueCb(nopDataCb)
    , logCallback(logCb)
{
}

int SignalContainer::setSignalMetaCb(SignalMetaCb cb)
{
    if (!cb) {
        STREAMING_PROTOCOL_LOG_E("not a valid callback!");
        return -1;
    }
    m_signalMetaCb = cb;
    return 0;
}

int SignalContainer::setDataAsRawCb(DataAsRawCb cb)
{
    if (!cb) {
        STREAMING_PROTOCOL_LOG_E("not a valid callback!");
        return -1;
    }
    m_dataAsRawCb = cb;
    return 0;
}

int SignalContainer::setDataAsValueCb(DataAsValueCb cb)
{
    if (!cb) {
        std::cerr << "not a valid callback!";
        return -1;
    }
    m_dataAsValueCb = cb;
    return 0;
}

int SignalContainer::processMetaInformation(SignalNumber signalNumber, const MetaInformation &metaInformation)
{
    Signals::const_iterator signalIter;
    std::string method = metaInformation.method();
    const nlohmann::json& params = metaInformation.params();

    if (method == META_METHOD_UNSUBSCRIBE) {
        signalIter = m_subscribedSignals.find(signalNumber);
        if (signalIter == m_subscribedSignals.end()) {
            STREAMING_PROTOCOL_LOG_E("Got unsubscribe meta information for signal '{}' that was not subscribed before", signalNumber);
            return -1;
        }
        std::string tableId = signalIter->second->tableId();
        auto tableIter = m_tables.find(tableId);
        if (tableIter != m_tables.end()) {
            Table& table = tableIter->second;
            if (signalIter->second->isTimeSignal()) {
                table.timeSignalNumber = 0;
            } else {
                table.dataSignalNumbers.erase(signalNumber);
            }
            if ((table.timeSignalNumber == 0) && (table.dataSignalNumbers.empty())) {
                // table is empty!
                m_tables.erase(tableIter);
            }
        }
    } else if (method == META_METHOD_SUBSCRIBE) {
        // A new signal!
        const auto signalIdIter = params.find(META_SIGNALID);
        if (signalIdIter == params.end()) {
            STREAMING_PROTOCOL_LOG_E("Invalid subscribe ack: No signal id!");
            return -1;
        }
        auto subscribedSignal = std::make_unique < SubscribedSignal > (signalNumber, logCallback);
        //STREAMING_PROTOCOL_LOG_I(":\n\tGot subscribed! (signal number: {})", signalNumber);
        std::pair < Signals::iterator, bool > result = m_subscribedSignals.emplace(signalNumber, std::move(subscribedSignal));
        if (result.second==false) {
            STREAMING_PROTOCOL_LOG_E("Got duplicate subscribe ack for signal number {} with signal id {}!", signalNumber, signalIdIter->dump());
            return -1;
        }
        signalIter = result.first;
    } else {
        signalIter = m_subscribedSignals.find(signalNumber);
        if (signalIter == m_subscribedSignals.end()) {
            STREAMING_PROTOCOL_LOG_E("Got meta information '{}' of signal {}, that was not subscribed before. Aborting!", method, signalNumber);
            return -1;
        }
    }
    SubscribedSignal& signal = *signalIter->second;
    int result = signal.processSignalMetaInformation(method, params);
    if (result != 0) {
        return result;
    }

    // This has to happen after meta information was processed!
    if (method == META_METHOD_SIGNAL) {
        // Perhaps we need to add this to the table members. If it already exists, nothing is changed.
        auto signalNumberIter = m_subscribedSignals.find(signalNumber);
        const auto& subscribedSignal = signalNumberIter->second;
        std::string tableId = subscribedSignal->tableId();
        if (subscribedSignal->isTimeSignal()) {
            m_tables[tableId].timeSignalNumber = signalNumber;
        } else {
            m_tables[tableId].dataSignalNumbers.insert(signalNumber);
        }
    }

    m_signalMetaCb(signal, method, params);

    if (method == META_METHOD_UNSUBSCRIBE) {
        m_subscribedSignals.erase(signalIter);
    }
    return 0;
}

ssize_t SignalContainer::processMeasuredData(SignalNumber signalNumber, const unsigned char* data, size_t len)
{
    Signals::iterator signalIter = m_subscribedSignals.find(signalNumber);
    if (signalIter == m_subscribedSignals.end()) {
        STREAMING_PROTOCOL_LOG_E("Got data for signal '{}', that was not subscribed before", signalNumber);
        return -1;
    }

    auto& signal = signalIter->second;
    if(signal->isTimeSignal()) {
        RuleType ruleType = signal->ruleType();
        std::string tableId = signal->tableId();
        if (ruleType == RULETYPE_EXPLICIT) {
            // for explicit time rule, there is no value index
            const auto tableIter = m_tables.find(tableId);
            if (tableIter != m_tables.end()) {
                uint64_t timeStamp;
                memcpy(&timeStamp, data, sizeof(timeStamp));
                const auto dataSignalNumbers = tableIter->second.dataSignalNumbers;
                for(const auto signalNumberIter : dataSignalNumbers) {
                    auto dataSignal = m_subscribedSignals[signalNumberIter];
                    signal->setTime(timeStamp);
                    dataSignal->clearLinearValueIndex();
                    STREAMING_PROTOCOL_LOG_D("{}:\n\tTime is: {}", dataSignal->signalId(), timeStamp);
                }
            }
        } else {
            // for implicit time rule:
            // 1st 64 bit are value index, followed by 64 bit timestamp
            // get the related data signal and set its time
            const auto tableIter = m_tables.find(tableId);
            if (tableIter != m_tables.end()) {
                const Table& table = tableIter->second;
                auto *indexedTimeStamp = reinterpret_cast<const IndexedValue<uint64_t>*>(data);
                const auto dataSignalNumbers = table.dataSignalNumbers;
                for(const auto signalNumberIter : dataSignalNumbers) {
                    auto& dataSignal = m_subscribedSignals[signalNumberIter];
                    signal->setTime(indexedTimeStamp->value);
                    dataSignal->clearLinearValueIndex();
                    STREAMING_PROTOCOL_LOG_D("{}:\n\tStart time is: {}", dataSignal->signalId(), indexedTimeStamp->value);
                }
            }
        }
    } else {
        std::string tableId = signal->tableId();
        const auto& tableIter = m_tables.find(tableId);
        if (tableIter != m_tables.end()) {
            const Table& table = tableIter->second;
            unsigned int timeSignalNumber = table.timeSignalNumber;
            if (timeSignalNumber == 0) {
                STREAMING_PROTOCOL_LOG_E("No time signal available for signal id '{}', number {}, table '{}'!",
                                         signalIter->second->signalId(),
                                         signalNumber,
                                         tableId);
                return -1;
            }
            auto timeSignal = m_subscribedSignals[timeSignalNumber];
            signal->setTimeSignal(timeSignal);
            return signal->processMeasuredData(data, len, timeSignal, m_dataAsRawCb, m_dataAsValueCb);
        }
    }
    return (ssize_t) len;
}
}
