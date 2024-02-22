#include <mutex>
#include <string>

#include "nlohmann/json.hpp"
#include "streaming_protocol/BaseSignal.hpp"

#include <iostream>

#include "streaming_protocol/Types.h"
#include "streaming_protocol/Unit.hpp"

namespace daq::streaming_protocol {

SignalNumber BaseSignal::s_signalNumberCounter = 0;
std::mutex BaseSignal::s_signalNumberMtx;

BaseSignal::BaseSignal(const std::string& signalId, const std::string& tableId, iWriter &writer, LogCallback logCb)
    : m_signalNumber(nextSignalNumber())
    , m_signalId(signalId)
    , m_tableId(tableId)
    , m_writer(writer)
    , logCallback(logCb)
{
}

void BaseSignal::subscribe()
{
    nlohmann::json subscribe;
    subscribe[METHOD] = META_METHOD_SUBSCRIBE;
    subscribe[PARAMS][META_SIGNALID] = m_signalId;
    m_writer.writeMetaInformation(m_signalNumber, subscribe);
    writeSignalMetaInformation();
}

void BaseSignal::unsubscribe()
{
    nlohmann::json unsubscribe;
    unsubscribe[METHOD] = META_METHOD_UNSUBSCRIBE;
    m_writer.writeMetaInformation(m_signalNumber, unsubscribe);
}


std::string BaseSignal::getId() const
{
    return m_signalId;
}

std::string BaseSignal::getTableId() const
{
    return m_tableId;
}


SignalNumber BaseSignal::getNumber() const
{
    return m_signalNumber;
}


void BaseSignal::setInterpretationObject(const nlohmann::json object) {
    m_interpretationObject = object;
}

nlohmann::json BaseSignal::getInterpretationObject() const
{
    return m_interpretationObject;
}

SignalNumber BaseSignal::nextSignalNumber()
{
    std::lock_guard guard(s_signalNumberMtx);
    ++s_signalNumberCounter;
    if ((s_signalNumberCounter & SIGNAL_NUMBER_MASK) == 0) {
        // Signal number 0 is reserved for stream related stuff and may not be used as signal number!
        ++s_signalNumberCounter;
    }
    return s_signalNumberCounter & SIGNAL_NUMBER_MASK;
}

}
