#include "streaming_protocol/BaseConstantSignal.hpp"
#include "streaming_protocol/Unit.hpp"

BEGIN_NAMESPACE_STREAMING_PROTOCOL

BaseConstantSignal::BaseConstantSignal(const std::string& signalId, const std::string& tableId, iWriter& writer, const nlohmann::json& defaultStartValue, LogCallback logCb)
    : BaseValueSignal(signalId, tableId, writer, logCb)
    , m_defaultStartValue(defaultStartValue)
{
}

void BaseConstantSignal::writeSignalMetaInformation() const
{
    nlohmann::json dataSignal;
    dataSignal[METHOD] = META_METHOD_SIGNAL;
    dataSignal[PARAMS][META_TABLEID] = m_tableId;
    if (!m_interpretationObject.is_null()) {
        dataSignal[PARAMS][META_INTERPRETATION] = m_interpretationObject;
    }

    dataSignal[PARAMS][META_DEFINITION] = getMemberInformation();
    composeRelatedSignals(dataSignal[PARAMS]);
    m_writer.writeMetaInformation(m_signalNumber, dataSignal);
}

nlohmann::json BaseConstantSignal::createMember(const std::string& dataType) const
{
    nlohmann::json memberInformation;
    memberInformation[META_NAME] = m_valueName;
    memberInformation[META_DATATYPE] = dataType;
    memberInformation[META_RULE] = META_RULETYPE_CONSTANT;

    m_unit.compose(memberInformation);
    m_range.compose(memberInformation);
    m_postScaling.compose(memberInformation);

    if (!m_defaultStartValue.is_null()) {
        memberInformation[META_RULETYPE_CONSTANT][META_START] = m_defaultStartValue;
    }
    return memberInformation;
}

END_NAMESPACE_STREAMING_PROTOCOL
