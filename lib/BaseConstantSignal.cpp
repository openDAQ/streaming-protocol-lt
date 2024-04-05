#include "streaming_protocol/BaseConstantSignal.hpp"
#include "streaming_protocol/Unit.hpp"

BEGIN_NAMESPACE_STREAMING_PROTOCOL

BaseConstantSignal::BaseConstantSignal(const std::string &signalId, const std::string &tableId, iWriter &writer, LogCallback logCb)
    : BaseValueSignal(signalId, tableId, writer, logCb)
{
}

void BaseConstantSignal::writeSignalMetaInformation() const
{
    nlohmann::json dataSignal;
    dataSignal[METHOD] = META_METHOD_SIGNAL;
    dataSignal[PARAMS][META_TABLEID] = m_tableId;
    dataSignal[PARAMS][META_DEFINITION] = getMemberInformation();
    if (!m_interpretationObject.is_null()) {
        dataSignal[PARAMS][META_INTERPRETATION] = m_interpretationObject;
    }
    m_writer.writeMetaInformation(m_signalNumber, dataSignal);
}

nlohmann::json BaseConstantSignal::createMember(const std::string &dataType) const
{
    nlohmann::json memberInformation;
    memberInformation[META_NAME] = m_valueName;
    memberInformation[META_DATATYPE] = dataType;
    memberInformation[META_RULE] = META_RULETYPE_CONSTANT;
    if (m_unitId != Unit::UNIT_ID_NONE) {
        memberInformation[META_UNIT][META_UNIT_ID] = m_unitId;
        memberInformation[META_UNIT][META_DISPLAY_NAME] = m_unitDisplayName;
    }
    return memberInformation;
}

END_NAMESPACE_STREAMING_PROTOCOL
