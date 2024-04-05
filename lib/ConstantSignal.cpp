#include "streaming_protocol/ConstantSignal.hpp"

BEGIN_NAMESPACE_STREAMING_PROTOCOL

template <>
nlohmann::json ConstantSignal<int8_t>::getMemberInformation() const
{
    return createMember(DATA_TYPE_INT8);
}

template <>
nlohmann::json ConstantSignal<uint8_t>::getMemberInformation() const
{
    return createMember(DATA_TYPE_UINT8);
}

template <>
nlohmann::json ConstantSignal<int16_t>::getMemberInformation() const
{
    return createMember(DATA_TYPE_INT16);
}

template <>
nlohmann::json ConstantSignal<uint16_t>::getMemberInformation() const
{
    return createMember(DATA_TYPE_UINT16);
}

template <>
nlohmann::json ConstantSignal<int32_t>::getMemberInformation() const
{
    return createMember(DATA_TYPE_INT32);
}

template <>
nlohmann::json ConstantSignal<uint32_t>::getMemberInformation() const
{
    return createMember(DATA_TYPE_UINT32);
}

template <>
nlohmann::json ConstantSignal<int64_t>::getMemberInformation() const
{
    return createMember(DATA_TYPE_INT64);
}

template <>
nlohmann::json ConstantSignal<uint64_t>::getMemberInformation() const
{
    return createMember(DATA_TYPE_UINT64);
}

template <>
nlohmann::json ConstantSignal<float>::getMemberInformation() const
{
    return createMember(DATA_TYPE_REAL32);
}

template <>
nlohmann::json ConstantSignal<double>::getMemberInformation() const
{
    return createMember(DATA_TYPE_REAL64);
}

template <>
SampleType ConstantSignal<int8_t>::getSampleType() const
{
    return SAMPLETYPE_S8;
}

template <>
SampleType ConstantSignal<int16_t>::getSampleType() const
{
    return SAMPLETYPE_S16;
}

template <>
SampleType ConstantSignal<int32_t>::getSampleType() const
{
    return SAMPLETYPE_S32;
}

template <>
SampleType ConstantSignal<int64_t>::getSampleType() const
{
    return SAMPLETYPE_S64;
}

template <>
SampleType ConstantSignal<uint8_t>::getSampleType() const
{
    return SAMPLETYPE_U8;
}

template <>
SampleType ConstantSignal<uint16_t>::getSampleType() const
{
    return SAMPLETYPE_U16;
}

template <>
SampleType ConstantSignal<uint32_t>::getSampleType() const
{
    return SAMPLETYPE_U32;
}

template <>
SampleType ConstantSignal<uint64_t>::getSampleType() const
{
    return SAMPLETYPE_U64;
}

template <>
SampleType ConstantSignal<float>::getSampleType() const
{
    return SAMPLETYPE_REAL32;
}

template <>
SampleType ConstantSignal<double>::getSampleType() const
{
    return SAMPLETYPE_REAL64;
}

END_NAMESPACE_STREAMING_PROTOCOL
