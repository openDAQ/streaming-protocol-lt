#include <mutex>
#include <string>

#include "nlohmann/json.hpp"
#include "streaming_protocol/ExplicitTimeSignal.hpp"
#include "streaming_protocol/Unit.hpp"


#include <iostream>


namespace daq::streaming_protocol {

ExplicitTimeSignal::ExplicitTimeSignal(const std::string& signalId, const std::string tableId, uint64_t timeTicksPerSecond, iWriter &writer, LogCallback logCb)
    : BaseDomainSignal(signalId, tableId, timeTicksPerSecond, writer, logCb)
{
}

daq::streaming_protocol::RuleType daq::streaming_protocol::ExplicitTimeSignal::getTimeRule() const
{
    return RULETYPE_EXPLICIT;
}

    //        result = m_writer.writeSignalData(m_timeSignalNumber, reinterpret_cast<const uint8_t*>(&tuple.timeStamp), sizeof(tuple.timeStamp));
   //         if (result < 0) {
    //            STREAMING_PROTOCOL_LOG_E("{}: Could not write signal timestamp!", m_timeSignalNumber);
    //            return result;
    //        }

void daq::streaming_protocol::ExplicitTimeSignal::writeSignalMetaInformation() const
{
    nlohmann::json timeSignal;
    timeSignal[METHOD] = META_METHOD_SIGNAL;
    timeSignal[PARAMS][META_TABLEID] = m_signalId;
    timeSignal[PARAMS][META_DEFINITION][META_NAME] = META_TIME;
    timeSignal[PARAMS][META_DEFINITION][META_RULE] = META_RULETYPE_EXPLICIT;
    timeSignal[PARAMS][META_DEFINITION][META_DATATYPE] = DATA_TYPE_UINT64;

    timeSignal[PARAMS][META_DEFINITION][META_UNIT][META_UNIT_ID] = Unit::UNIT_ID_SECONDS;
    timeSignal[PARAMS][META_DEFINITION][META_UNIT][META_DISPLAY_NAME] = "s";
    timeSignal[PARAMS][META_DEFINITION][META_UNIT][META_QUANTITY] = META_TIME;
    timeSignal[PARAMS][META_DEFINITION][META_ABSOLUTE_REFERENCE] = m_epoch;
    timeSignal[PARAMS][META_DEFINITION][META_RESOLUTION][META_NUMERATOR] = 1;
    timeSignal[PARAMS][META_DEFINITION][META_RESOLUTION][META_DENOMINATOR] = m_timeTicksPerSecond;
    if (!m_interpretationObject.is_null()) {
        timeSignal[PARAMS][META_INTERPRETATION] = m_interpretationObject;
    }
    m_writer.writeMetaInformation(m_signalNumber, timeSignal);
}

}