#include "streaming_protocol/BaseSynchronousSignal.hpp"
#include "streaming_protocol/Unit.hpp"

daq::streaming_protocol::BaseSynchronousSignal::BaseSynchronousSignal(const std::string &signalId, uint64_t outputRate, uint64_t timeTicksPerSecond, iWriter &writer, LogCallback logCb)
    : BaseSignal(signalId, timeTicksPerSecond, writer, logCb)
    , m_outputRateInTicks(outputRate)
    , m_valueIndex(0)
{
}

daq::streaming_protocol::RuleType daq::streaming_protocol::BaseSynchronousSignal::getTimeRule() const
{
    return RULETYPE_LINEAR;
}

uint64_t daq::streaming_protocol::BaseSynchronousSignal::getTimeDelta() const
{
    return m_outputRateInTicks;
}

uint64_t daq::streaming_protocol::BaseSynchronousSignal::getTimeStart() const
{
    return m_startInTicks;
}

void daq::streaming_protocol::BaseSynchronousSignal::setTimeStart(uint64_t timeTicks)
{
    // Start time is send separately since it is generated with the first measured value
    m_startInTicks = timeTicks;
    /// @warning here we rely on a data type uint64_t for the valueindex followed by the value itself. This is some implicit knowledge the client has to have.
    /// The size of a complete value it equals to sizeof(uint64_t) +
    struct BinaryUint64StartValue {
        uint64_t valueIndex;
        uint64_t timeStart;
    };
    BinaryUint64StartValue startValue;
    startValue.valueIndex = m_valueIndex; // initial start time!
    startValue.timeStart = timeTicks;
    int result = m_writer.writeSignalData(m_timeSignalNumber, reinterpret_cast<uint8_t*>(&startValue), sizeof(startValue));
    if (result < 0) {
        STREAMING_PROTOCOL_LOG_E("{}: Could not write signal time!", m_dataSignalNumber);
    }
}

void daq::streaming_protocol::BaseSynchronousSignal::setOutputRate(uint64_t timeTicks)
{
    m_outputRateInTicks = timeTicks;
}

void daq::streaming_protocol::BaseSynchronousSignal::writeSignalMetaInformation() const
{
    // in openDAQ time is a separate signal!
    nlohmann::json dataSignal;

    dataSignal[METHOD] = META_METHOD_SIGNAL;
    dataSignal[PARAMS][META_TABLEID] = m_signalId;
    dataSignal[PARAMS][META_DEFINITION] = getMemberInformation();
    if (!m_dataInterpretationObject.is_null()) {
        dataSignal[PARAMS][META_INTERPRETATION] = m_dataInterpretationObject;
    }
    m_writer.writeMetaInformation(m_dataSignalNumber, dataSignal);

    nlohmann::json timeSignal;
    timeSignal[METHOD] = META_METHOD_SIGNAL;
    timeSignal[PARAMS][META_TABLEID] = m_signalId;
    timeSignal[PARAMS][META_DEFINITION][META_NAME] = META_TIME;
    timeSignal[PARAMS][META_DEFINITION][META_RULE] = META_RULETYPE_LINEAR;

    timeSignal[PARAMS][META_DEFINITION][META_RULETYPE_LINEAR][META_DELTA] = m_outputRateInTicks;
    timeSignal[PARAMS][META_DEFINITION][META_DATATYPE] = DATA_TYPE_UINT64;

    timeSignal[PARAMS][META_DEFINITION][META_UNIT][META_UNIT_ID] = Unit::UNIT_ID_SECONDS;
    timeSignal[PARAMS][META_DEFINITION][META_UNIT][META_DISPLAY_NAME] = "s";
    if (!m_timeInterpretationObject.is_null()) {
        timeSignal[PARAMS][META_INTERPRETATION] = m_timeInterpretationObject;
    }

    timeSignal[PARAMS][META_DEFINITION][META_TIME][META_ABSOLUTE_REFERENCE] = m_epoch;
    timeSignal[PARAMS][META_DEFINITION][META_TIME][META_RESOLUTION][META_NUMERATOR] = 1;
    timeSignal[PARAMS][META_DEFINITION][META_TIME][META_RESOLUTION][META_DENOMINATOR] = m_timeTicksPerSecond;
    m_writer.writeMetaInformation(m_timeSignalNumber, timeSignal);
}

nlohmann::json daq::streaming_protocol::BaseSynchronousSignal::createMember(const std::string &dataType) const
{
    nlohmann::json memberInformation;
    memberInformation[META_NAME] = m_valueName;
    memberInformation[META_DATATYPE] = dataType;
    memberInformation[META_RULE] = META_RULETYPE_EXPLICIT;
    if (m_unitId != Unit::UNIT_ID_NONE) {
        memberInformation[META_UNIT][META_UNIT_ID] = m_unitId;
        memberInformation[META_UNIT][META_DISPLAY_NAME] = m_unitDisplayName;
    }

    return memberInformation;
}
