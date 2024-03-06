#include <mutex>
#include <string>

#include "nlohmann/json.hpp"
#include "streaming_protocol/LinearTimeSignal.hpp"
#include "streaming_protocol/Unit.hpp"

#include <iostream>


namespace daq::streaming_protocol {

LinearTimeSignal::LinearTimeSignal(const std::string& signalId, const std::string& tableId, uint64_t timeTicksPerSecond, const std::chrono::nanoseconds &outputRate, iWriter &writer, LogCallback logCb)
    : BaseDomainSignal(signalId, tableId, timeTicksPerSecond, writer, logCb)
    , m_timeTicksPerSecond(timeTicksPerSecond)
    , m_outputRateInTicks(timeTicksPerSecond/1000000000 * outputRate.count())
{
}


daq::streaming_protocol::RuleType LinearTimeSignal::getTimeRule() const
{
    return RULETYPE_LINEAR;
}

uint64_t LinearTimeSignal::getTimeDelta() const
{
    return m_outputRateInTicks;
}

void daq::streaming_protocol::LinearTimeSignal::setOutputRate(uint64_t timeTicks)
{
    m_outputRateInTicks = timeTicks;
}

void LinearTimeSignal::setOutputRate(const std::chrono::nanoseconds &nanoseconds)
{
    setOutputRate(m_timeTicksPerSecond/1000000000 * nanoseconds.count());
}

void daq::streaming_protocol::LinearTimeSignal::writeSignalMetaInformation() const
{
    nlohmann::json timeSignal;
    timeSignal[METHOD] = META_METHOD_SIGNAL;
    timeSignal[PARAMS][META_TABLEID] = m_tableId;
    timeSignal[PARAMS][META_DEFINITION][META_NAME] = META_TIME;
    timeSignal[PARAMS][META_DEFINITION][META_RULE] = META_RULETYPE_LINEAR;

    timeSignal[PARAMS][META_DEFINITION][META_RULETYPE_LINEAR][META_DELTA] = m_outputRateInTicks;
    timeSignal[PARAMS][META_DEFINITION][META_DATATYPE] = DATA_TYPE_UINT64;

    timeSignal[PARAMS][META_DEFINITION][META_UNIT][META_UNIT_ID] = Unit::UNIT_ID_SECONDS;
    timeSignal[PARAMS][META_DEFINITION][META_UNIT][META_DISPLAY_NAME] = "s";
    timeSignal[PARAMS][META_DEFINITION][META_UNIT][META_QUANTITY] = META_TIME;
    if (!m_interpretationObject.is_null()) {
        timeSignal[PARAMS][META_INTERPRETATION] = m_interpretationObject;
    }

    timeSignal[PARAMS][META_DEFINITION][META_ABSOLUTE_REFERENCE] = m_epoch;
    timeSignal[PARAMS][META_DEFINITION][META_RESOLUTION][META_NUMERATOR] = 1;
    timeSignal[PARAMS][META_DEFINITION][META_RESOLUTION][META_DENOMINATOR] = m_timeTicksPerSecond;
    m_writer.writeMetaInformation(m_signalNumber, timeSignal);
}

}
