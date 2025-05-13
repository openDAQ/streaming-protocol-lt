#include <cstring>
#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <stdint.h>
#include <sstream>

#include <nlohmann/json.hpp>

#include "streaming_protocol/Defines.h"
#include "streaming_protocol/SubscribedSignal.hpp"

#include "streaming_protocol/Logging.hpp"
#include "streaming_protocol/Types.h"

namespace daq::streaming_protocol {

SubscribedSignal::SubscribedSignal(unsigned int signalNumber, LogCallback logCb)
    : m_signalNumber(signalNumber)
    , m_signalId()
    , m_tableId()
    , m_isTimeSignal(false)
    , m_dataValueType(SAMPLETYPE_UNKNOWN)
    , m_dataValueSize(sizeof(float)) // may not be zero because it is used as divisor.
    , m_ruleType(RULETYPE_EXPLICIT) // explicit rule is default
    , m_time(0)
    , m_linearDelta(0)
    , m_linearDeltaJson(nullptr)
    , m_constRuleStartJson(nullptr)
    , m_linearValueIndex(0)
    , m_timeBaseFrequency(0)
    , logCallback(logCb)
{
}

ssize_t SubscribedSignal::processMeasuredData(const unsigned char* pData, size_t size, std::shared_ptr<SubscribedSignal> timeSignal, DataAsRawCb cbRaw, DataAsValueCb cbValues)
{
    auto timeSignalRule = RULETYPE_EXPLICIT;
    if (timeSignal)
        timeSignalRule = timeSignal->m_ruleType;

    switch (timeSignalRule) {
    case RULETYPE_LINEAR:
    {
        // Signals with a non-explicit rule will have a value index for each value
        size_t bytesPerValue;
        m_linearDelta = timeSignal->m_linearDelta;

        if (m_ruleType == RULETYPE_EXPLICIT) {
            // Since we deliver the time stamp of the first value,
            // We increment timestamp after execution of the callback methods.
            // short read is not allowed! We expect complete packages only!

            uint64_t timeStamp = timeSignal->time() + ((m_linearValueIndex - timeSignal->timeIndex()) * m_linearDelta);
            cbRaw(*this, timeStamp, pData, size);

            bytesPerValue = m_dataValueSize;
            size_t valueCount = size / bytesPerValue;
            cbValues(*this, timeStamp, pData, valueCount);
            m_linearValueIndex += valueCount;
        }
        else if (m_ruleType == RULETYPE_CONSTANT) {
            // for implicit signals, we get also the value index
            bytesPerValue = sizeof(uint64_t) + m_dataValueSize;

            cbRaw(*this, timeSignal->time(), pData, size);
            size_t valueCount = size / bytesPerValue;
            cbValues(*this, timeSignal->time(), pData, valueCount);
        }
        else {
            STREAMING_PROTOCOL_LOG_E("Linear data signal is not supported");
        }
        break;
    }
    case RULETYPE_EXPLICIT:
    {
        // In openDAQ streaming protocol, time stamp and value are delivered separately as explicit values
        // in the time signal and the data signal.
        // The device will deliver the time stamp before the value => Time (m_time) is already set! 
        if ((size % m_dataValueSize) != 0) {
            STREAMING_PROTOCOL_LOG_E("Data is not an even multiple of expected data size");
            return (ssize_t) size;
        }
        uint64_t time = timeSignal ? timeSignal->m_time : 0;
        cbRaw(*this, time, pData, size);
        cbValues(*this, time, pData, size / m_dataValueSize);
        break;
    }
    case RULETYPE_CONSTANT:
        STREAMING_PROTOCOL_LOG_E("Domain signal with constant rule is not supported  ({})", m_signalId);
        return -1;
    case RULETYPE_UNKNOWN:
        STREAMING_PROTOCOL_LOG_E("No rule for signal ", m_signalId);
        return -1;
    }

    return (ssize_t) size;
}


static size_t getDataTypeSize(const nlohmann::json& definitionNode)
{
    std::size_t count = 1;
    std::string name = definitionNode.value(META_NAME, "");

    // Check for one-dimensional arrays of primitives.
    if (definitionNode.count(META_DIMENSIONS) > 0)
    {
        auto dimensions = definitionNode[META_DIMENSIONS];
        if (!dimensions.is_array() ||
                dimensions.size() != 1 ||
                !dimensions[0].is_object())
            return 0;

        auto dimension = dimensions[0];
        if (dimension.value(META_RULE, "") != "linear" ||
                dimension.count(META_RULETYPE_LINEAR) != 1 ||
                !dimension[META_RULETYPE_LINEAR].is_object())
            return 0;

        auto linear = dimension[META_RULETYPE_LINEAR];
        if (linear.value(META_START, 0) != 1 ||
                linear.value(META_DELTA, 0) != 0)
            return 0;

        count = linear.value(META_SIZE, 1);
    }

    auto dataTypeIter = definitionNode.find(META_DATATYPE);
    if (dataTypeIter != definitionNode.end()) {
        nlohmann::json datatypeNode = *dataTypeIter;
        std::string dataType = datatypeNode;
        if (dataType == DATA_TYPE_UINT8) {
            return sizeof(uint8_t) * count;
        } else if (dataType == DATA_TYPE_UINT16) {
            return sizeof(uint16_t) * count;
        } else if (dataType == DATA_TYPE_UINT32) {
            return sizeof(uint32_t) * count;
        } else if (dataType == DATA_TYPE_UINT64) {
            return sizeof(uint64_t) * count;
        } else if (dataType == DATA_TYPE_INT8) {
            return sizeof(int8_t) * count;
        } else if (dataType == DATA_TYPE_INT16) {
            return sizeof(int16_t) * count;
        } else if (dataType == DATA_TYPE_INT32) {
            return sizeof(int32_t) * count;
        } else if (dataType == DATA_TYPE_INT64) {
            return sizeof(int64_t) * count;
        } else if (dataType == DATA_TYPE_REAL32) {
            return sizeof(float) * count;
        } else if (dataType == DATA_TYPE_REAL64) {
            return sizeof(double) * count;
        } else if (dataType == DATA_TYPE_COMPLEX32) {
            return sizeof(Complex32Type) * count;
        } else if (dataType == DATA_TYPE_COMPLEX64) {
            return sizeof(Complex64Type) * count;
        } else if (dataType == DATA_TYPE_BITFIELD) {
            nlohmann::json subDatatypeNode = definitionNode[DATA_TYPE_BITFIELD];
            return getDataTypeSize(subDatatypeNode) * count;
        } else if (dataType == DATA_TYPE_ARRAY) {
            nlohmann::json subDatatypeNode = definitionNode[DATA_TYPE_ARRAY];
            size_t arrayCount = subDatatypeNode[META_COUNT];
            return getDataTypeSize(subDatatypeNode) * count * arrayCount;
        } else if (dataType == DATA_TYPE_STRUCT) {
            size_t dataTypeSize = 0;
            for (const auto& subDatatypeNodeIter: definitionNode[DATA_TYPE_STRUCT] ) {
                const nlohmann::json& subDatatypeNode = subDatatypeNodeIter;
                dataTypeSize += getDataTypeSize(subDatatypeNode) * count;
            }
            return dataTypeSize;
        } else {
            return 0;
        }
    }
    return 0;
}

int SubscribedSignal::processSignalMetaInformation(const std::string& method, const nlohmann::json& params)
{
    if (method == META_METHOD_SUBSCRIBE) {
        /// this is the first signal related meta information to arrive!
        auto iter = params.find(META_SIGNALID);
        if (iter == params.end()) {
            // there needs to be the signal id!
            return -1;
        }
        // We allow a string or a number here.
        const nlohmann::json& node = iter.value();
        if (node.is_string()) {
            m_signalId = node;
        } else if (node.is_number()) {
            m_signalId = node.dump();
        } else {
            return -1;
        }
    } else if (method == META_METHOD_SIGNAL) {
        auto tableIdIter = params.find(META_TABLEID);
        if (tableIdIter!=params.end()) {
            const nlohmann::json& node = tableIdIter.value();
            if (node.is_string()) {
                m_tableId = node;
            } else if (node.is_number()) {
                m_tableId = node.dump();
            } else {
                return -1;
            }
        }

        auto valueIndexIter = params.find(META_VALUEINDEX);
        if (valueIndexIter!=params.end()) {
            m_linearValueIndex = valueIndexIter.value();
        }

        auto interpretationIter = params.find(META_INTERPRETATION);
        if (interpretationIter != params.end()) {
            m_interpretationObject = *interpretationIter;
        }

        auto definitionIter = params.find(META_DEFINITION);
        if (definitionIter != params.end()) {
            try {
                STREAMING_PROTOCOL_LOG_I("{}:\n\tSignal definition", m_signalId);

                const nlohmann::json& definitionNode = *definitionIter;

                nlohmann::json::const_iterator linearIter = definitionNode.find(META_RULETYPE_LINEAR);
                if (linearIter != definitionNode.end()) {
                    const nlohmann::json& linearNode = *linearIter;
                    nlohmann::json::const_iterator deltaIter = linearNode.find(META_DELTA);
                    if (deltaIter != linearNode.end()) {
                        m_linearDeltaJson = *deltaIter;
                        if (!m_linearDeltaJson.is_number()) {
                            STREAMING_PROTOCOL_LOG_E("{}: Non-numeric delta value is not allowed for linear rule!", m_signalId);
                            return -1;
                        }
                        m_linearDelta = *deltaIter;
                    }
                }

                nlohmann::json::const_iterator constantRuleIter = definitionNode.find(META_RULETYPE_CONSTANT);
                if (constantRuleIter != definitionNode.end()) {
                    const nlohmann::json& constantRuleNode = *constantRuleIter;
                    nlohmann::json::const_iterator startValueIter = constantRuleNode.find(META_START);
                    if (startValueIter != constantRuleNode.end()) {
                        m_constRuleStartJson = *startValueIter;
                        if (!m_constRuleStartJson.is_number()) {
                            STREAMING_PROTOCOL_LOG_E("{}: Non-numeric start value is not allowed for constant rule!", m_signalId);
                            return -1;
                        }
                        STREAMING_PROTOCOL_LOG_I("\tConst rule \"start\" value: {}\n", m_constRuleStartJson.dump());
                    }
                }

                auto ruleIter = definitionNode.find(META_RULE);
                if (ruleIter != definitionNode.end()) {
                    std::string rule = *ruleIter;
                    if (rule == META_RULETYPE_LINEAR) {
                        // always make sure that linear rule is valid.
                        // linear rule has to be delivered in a prior meta information at at the latest with this package.
                        if (m_linearDelta == 0) {
                            STREAMING_PROTOCOL_LOG_E("{}: Time delta of 0 is not allowed for linear rule!", m_signalId);
                            return -1;
                        }
                        m_ruleType = RULETYPE_LINEAR;
                    } else if (rule == META_RULETYPE_EXPLICIT) {
                        m_ruleType = RULETYPE_EXPLICIT;
                    } else if (rule == META_RULETYPE_CONSTANT) {
                        m_ruleType = RULETYPE_CONSTANT;
                    } else {
                        STREAMING_PROTOCOL_LOG_E("\tUnknown implicit rule\n");
                        return -1;
                    }
                }


                size_t dataValueSize = getDataTypeSize(definitionNode);
                if (dataValueSize) {
                    m_dataValueSize = dataValueSize;
                }

                nlohmann::json::const_iterator memberNameIter = definitionNode.find(META_NAME);
                if (memberNameIter != definitionNode.end()) {
                    m_memberName = *memberNameIter;
                    STREAMING_PROTOCOL_LOG_I("\tname: {}", m_memberName);
                }
                auto dataTypeIter = definitionNode.find(META_DATATYPE);
                if (dataTypeIter != definitionNode.end()) {
                    std::string dataType = *dataTypeIter;
                    if (dataType == DATA_TYPE_UINT8) {
                        m_dataValueType = SAMPLETYPE_U8;
                    } else if (dataType == DATA_TYPE_UINT16) {
                        m_dataValueType = SAMPLETYPE_U16;
                    } else if (dataType == DATA_TYPE_UINT32) {
                        m_dataValueType = SAMPLETYPE_U32;
                    } else if (dataType == DATA_TYPE_UINT64) {
                        m_dataValueType = SAMPLETYPE_U64;
                    } else if (dataType == DATA_TYPE_INT8) {
                        m_dataValueType = SAMPLETYPE_S8;
                    } else if (dataType == DATA_TYPE_INT16) {
                        m_dataValueType = SAMPLETYPE_S16;
                    } else if (dataType == DATA_TYPE_INT32) {
                        m_dataValueType = SAMPLETYPE_S32;
                    } else if (dataType == DATA_TYPE_INT64) {
                        m_dataValueType = SAMPLETYPE_S64;
                    } else if (dataType == DATA_TYPE_REAL32) {
                        m_dataValueType = SAMPLETYPE_REAL32;
                    } else if (dataType == DATA_TYPE_REAL64) {
                        m_dataValueType = SAMPLETYPE_REAL64;
                    } else if (dataType == DATA_TYPE_BITFIELD) {
                        /// Find details in the "bitField" object. They have the following form:
                        /// \code
                        /// {
                        ///   "bitField": {
                        ///     "bits": [
                        ///       {
                        ///         "description": "Data overrun",
                        ///         "index": 0,
                        ///         "uuid": "c214c128-2447-4cee-ba39-6227aed2eff4"
                        ///       },
                        ///            .
                        ///            .
                        ///            .
                        ///    ],
                        ///    "dataType": "uint64"
                        ///  },
                        ///  "dataType": "bitField",
                        ///}
                        ///
                        m_datatypeDetails = definitionNode[DATA_TYPE_BITFIELD];
                        m_bitsInterpretationObject = m_datatypeDetails["bits"];
                        std::string bitfieldDataType = m_datatypeDetails[META_DATATYPE];
                        if (bitfieldDataType == DATA_TYPE_UINT32) {
                            m_dataValueType = SAMPLETYPE_BITFIELD32;
                        } else if (bitfieldDataType == DATA_TYPE_UINT64) {
                            m_dataValueType = SAMPLETYPE_BITFIELD64;
                        } else {
                            return -1;
                        }
                    } else if (dataType == DATA_TYPE_COMPLEX32) {
                        m_dataValueType = SAMPLETYPE_COMPLEX32;
                    } else if (dataType == DATA_TYPE_COMPLEX64) {
                        m_dataValueType = SAMPLETYPE_COMPLEX64;
                    } else if (dataType == DATA_TYPE_ARRAY) {
                        /// An array has a fixed number of elements of the specified member
                        /// Find details in the "array" object. They have the following form:
                        /// \code
                        /// "array": {
                        ///   "count": 1024,
                        ///   "dataType" : "int32"
                        /// }
                        /// \endcode
                        /// \note only scalar array elements are supported here!
                        m_datatypeDetails = definitionNode[DATA_TYPE_ARRAY];
                        m_dataValueType = SAMPLETYPE_ARRAY;
                    } else if (dataType == DATA_TYPE_DYNAMIC_ARRAY) {
                        /// An array with variable number of elements.
                        /// Data package always consists of a uint32 with the number of members followed by the explicit content of the members
                        /// \code
                        /// "dynamicArray": {
                        ///   "dataType" : "int32"
                        /// }
                        /// \endcode
                        STREAMING_PROTOCOL_LOG_E("{}: Data type 'dynamicArray' is not supported!", m_signalId);
                        return -1;
                    } else if (dataType == DATA_TYPE_STRUCT) {
                        /// Struct constains an array of members
                        /// Find details in the "struct" object. They have the following form:
                        /// \code
                        /// "struct": [
                        ///   {
                        ///     "name": "amplitude",
                        ///     "dataType": "real64",
                        ///   },
                        ///   {
                        ///     "name": "frequency",
                        ///     "dataType": "real64",
                        ///   }
                        /// ]
                        /// \endcode
                        /// \note only scalar elements are supported here!
                        m_datatypeDetails = definitionNode[DATA_TYPE_STRUCT];
                        m_dataValueType = SAMPLETYPE_STRUCT;
                    } else {
                        STREAMING_PROTOCOL_LOG_E("{0}: Unknown datatype '{1}'", m_signalId, dataType);
                        return -1;
                    }
                }

                m_unit.parse(definitionNode);
                if (m_unit.quantity==META_TIME) {
                    m_isTimeSignal = true;
                }

                int result = m_resolution.parse(definitionNode, logCallback);
                if (result==1) {
                  if (m_unit.id == Unit::UNIT_ID_SECONDS || (m_unit.id == Unit::UNIT_ID_NONE && m_unit.displayName == "s")) {
                    // numerator/denominator gives time between ticks, or period, in the unit of the signal.
                    // Unit for time signals is seconds. We want the frequency here!
                    m_timeBaseFrequency = m_resolution.denominator / m_resolution.numerator;
                    STREAMING_PROTOCOL_LOG_I("\ttime resolution: {} Hz", m_resolution.toString());
                  } else {
                    STREAMING_PROTOCOL_LOG_E("\tFor time unit 's' is required!");
                    return -1;
                  }
                }

                // openDAQ streaming
                auto absoluteReferenceIter = definitionNode.find(META_ABSOLUTE_REFERENCE);
                if (absoluteReferenceIter != definitionNode.end()) {
                    m_timeBaseEpochAsString = absoluteReferenceIter.value();
                    STREAMING_PROTOCOL_LOG_I("\tabsolute reference: {}", m_timeBaseEpochAsString);
                }

                m_range.parse(definitionNode);
                m_postScaling.parse(definitionNode);

                if (m_isTimeSignal) {
                    // separate check because time chapter may only be send initialy, later changes won't have this again!
                    if (m_ruleType == RULETYPE_LINEAR) {
                        // Frequency may be smaller than 1 Hz
                        double frequency = static_cast<double>(m_timeBaseFrequency)/m_linearDelta;
                        STREAMING_PROTOCOL_LOG_I("\tSynchronous signal (linear time)\n");
                        STREAMING_PROTOCOL_LOG_I("\t\tLinear delta: {}", m_linearDelta);
                        STREAMING_PROTOCOL_LOG_I("\t\tFrequency: {} Hz", frequency);
                    } else if (m_ruleType == RULETYPE_EXPLICIT) {
                        STREAMING_PROTOCOL_LOG_I("\tAsynchronous signal (Explicit time)");
                    }
                }

                auto relatedSignalsIter = params.find(META_RELATEDSIGNALS);
                if (relatedSignalsIter != params.end()) {
                    m_relatedSignals.clear();
                    STREAMING_PROTOCOL_LOG_I("\tRelated signals", m_signalId);
                    const nlohmann::json& relatedSignals = *relatedSignalsIter;
                    if (relatedSignals.is_array()) {
                        for (const auto& arrayItem: relatedSignals) {
                            std::string type = arrayItem[META_TYPE];
                            std::string signalId = arrayItem[META_SIGNALID];
                            m_relatedSignals[type] = signalId;
                            STREAMING_PROTOCOL_LOG_I("\t\tsignal id: {}, type: ", signalId, type);
                        }
                    }
                }
            } catch(const nlohmann::json::exception& e) {
                STREAMING_PROTOCOL_LOG_E("{0}: Could not process signal meta information: {1}", m_signalId, e.what());
                return -1;
            }
        }
    }
    return 0;
}

size_t SubscribedSignal::interpretValuesAsDouble(const unsigned char *pData, size_t count, double* doubleValueBuffer) const
{
    switch (m_dataValueType) {
    case SAMPLETYPE_REAL32:
        convert < float >(pData, count, doubleValueBuffer);
        return count;
    case SAMPLETYPE_REAL64:
        convert < double >(pData, count, doubleValueBuffer);
        return count;
    case SAMPLETYPE_U8:
        convert < uint8_t >(pData, count, doubleValueBuffer);
        return count;
    case SAMPLETYPE_U16:
        convert < uint16_t >(pData, count, doubleValueBuffer);
        return count;
    case SAMPLETYPE_U32:
    case SAMPLETYPE_BITFIELD32:
        convert < uint32_t >(pData, count, doubleValueBuffer);
        return count;
    case SAMPLETYPE_U64:
    case SAMPLETYPE_BITFIELD64:
        convert < int64_t >(pData, count, doubleValueBuffer);
        return count;
    case SAMPLETYPE_S8:
        convert < int8_t >(pData, count, doubleValueBuffer);
        return count;
    case SAMPLETYPE_S16:
        convert < int16_t >(pData, count, doubleValueBuffer);
        return count;
    case SAMPLETYPE_S32:
        convert < int32_t >(pData, count, doubleValueBuffer);
        return count;
    case SAMPLETYPE_S64:
        convert < int64_t >(pData, count, doubleValueBuffer);
        return count;
    default:
        // All others are not supported
        return 0;
    }
}
}
