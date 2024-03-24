#include "streaming_protocol/BaseValueSignal.hpp"
#include "streaming_protocol/Unit.hpp"

daq::streaming_protocol::BaseValueSignal::BaseValueSignal(const std::string &signalId, const std::string &tableId, iWriter &writer, LogCallback logCb)
    : BaseSignal(signalId, tableId, writer, logCb)
    , m_valueName("value")
    , m_unitId(Unit::UNIT_ID_NONE)
{
}

int32_t daq::streaming_protocol::BaseValueSignal::getUnitId() const
{
    return m_unitId;
}

std::string daq::streaming_protocol::BaseValueSignal::getUnitDisplayName() const
{
    return m_unitDisplayName;
}

void daq::streaming_protocol::BaseValueSignal::setUnit(int32_t unitId, const std::string& displayName)
{
    m_unitId = unitId;
    m_unitDisplayName = displayName;
}

void daq::streaming_protocol::BaseValueSignal::setMemberName(const std::string &name)
{
    m_valueName = name;
}

std::string daq::streaming_protocol::BaseValueSignal::getMemberName() const
{
    return m_valueName;
}
