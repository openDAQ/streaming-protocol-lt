#include <mutex>
#include <string>

#include "nlohmann/json.hpp"
#include "streaming_protocol/ExplicitTimeSignal.hpp"
#include "streaming_protocol/Unit.hpp"


#include <iostream>


namespace daq::streaming_protocol {

ExplicitTimeSignal::ExplicitTimeSignal(const std::string& signalId, const std::string& tableId, uint64_t timeTicksPerSecond, iWriter &writer, LogCallback logCb)
    : BaseDomainSignal(signalId, tableId, timeTicksPerSecond, writer, logCb)
{
}

daq::streaming_protocol::RuleType daq::streaming_protocol::ExplicitTimeSignal::getRuleType() const
{
    return RULETYPE_EXPLICIT;
}

void daq::streaming_protocol::ExplicitTimeSignal::writeSignalMetaInformation() const
{
    nlohmann::json timeSignal;
    timeSignal[METHOD] = META_METHOD_SIGNAL;
    timeSignal[PARAMS][META_TABLEID] = m_tableId;
    timeSignal[PARAMS][META_DEFINITION] = getMemberInformation();

    if (!m_interpretationObject.is_null()) {
        timeSignal[PARAMS][META_INTERPRETATION] = m_interpretationObject;
    }
    m_writer.writeMetaInformation(m_signalNumber, timeSignal);
}

nlohmann::json ExplicitTimeSignal::getMemberInformation() const
{
    nlohmann::json memberInformation;
    memberInformation[META_NAME] = META_TIME;
    memberInformation[META_RULE] = META_RULETYPE_EXPLICIT;
    memberInformation[META_DATATYPE] = DATA_TYPE_UINT64;
    memberInformation[META_UNIT][META_UNIT_ID] = Unit::UNIT_ID_SECONDS;
    memberInformation[META_UNIT][META_DISPLAY_NAME] = "s";
    memberInformation[META_UNIT][META_QUANTITY] = META_TIME;
    memberInformation[META_ABSOLUTE_REFERENCE] = m_epoch;
    memberInformation[META_RESOLUTION][META_NUMERATOR] = 1;
    memberInformation[META_RESOLUTION][META_DENOMINATOR] = m_timeTicksPerSecond;

    return memberInformation;
}
}
