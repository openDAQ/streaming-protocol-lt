#include "streaming_protocol/SynchronousSignal.hpp"

BEGIN_NAMESPACE_STREAMING_PROTOCOL

template <>
nlohmann::json SynchronousSignal<int8_t>::getMemberInformation() const
{
    return createMember(DATA_TYPE_INT8);
}

template <>
nlohmann::json SynchronousSignal<uint8_t>::getMemberInformation() const
{
    return createMember(DATA_TYPE_UINT8);
}

template <>
nlohmann::json SynchronousSignal<int16_t>::getMemberInformation() const
{
    return createMember(DATA_TYPE_INT16);
}

template <>
nlohmann::json SynchronousSignal<uint16_t>::getMemberInformation() const
{
    return createMember(DATA_TYPE_UINT16);
}

template <>
nlohmann::json SynchronousSignal<int32_t>::getMemberInformation() const
{
    return createMember(DATA_TYPE_INT32);
}

template <>
nlohmann::json SynchronousSignal<uint32_t>::getMemberInformation() const
{
    return createMember(DATA_TYPE_UINT32);
}

template <>
nlohmann::json SynchronousSignal<int64_t>::getMemberInformation() const
{
    return createMember(DATA_TYPE_INT64);
}

template <>
nlohmann::json SynchronousSignal<uint64_t>::getMemberInformation() const
{
    return createMember(DATA_TYPE_UINT64);
}

template <>
nlohmann::json SynchronousSignal<float>::getMemberInformation() const
{
    return createMember(DATA_TYPE_REAL32);
}

template <>
nlohmann::json SynchronousSignal<double>::getMemberInformation() const
{
    return createMember(DATA_TYPE_REAL64);
}

template <>
nlohmann::json SynchronousSignal<Complex32Type>::getMemberInformation() const
{
    return createMember(DATA_TYPE_COMPLEX32);
}

template <>
nlohmann::json SynchronousSignal<Complex64Type>::getMemberInformation() const
{
    return createMember(DATA_TYPE_COMPLEX64);
}

template <>
SampleType SynchronousSignal<int8_t>::getSampleType() const
{
    return SAMPLETYPE_S8;
}
template <>
SampleType SynchronousSignal<int16_t>::getSampleType() const
{
    return SAMPLETYPE_S16;
}
template <>
SampleType SynchronousSignal<int32_t>::getSampleType() const
{
    return SAMPLETYPE_S32;
}
template <>
SampleType SynchronousSignal<int64_t>::getSampleType() const
{
    return SAMPLETYPE_S64;
}
template <>
SampleType SynchronousSignal<uint8_t>::getSampleType() const
{
    return SAMPLETYPE_U8;
}
template <>
SampleType SynchronousSignal<uint16_t>::getSampleType() const
{
    return SAMPLETYPE_U16;
}
template <>
SampleType SynchronousSignal<uint32_t>::getSampleType() const
{
    return SAMPLETYPE_U32;
}
template <>
SampleType SynchronousSignal<uint64_t>::getSampleType() const
{
    return SAMPLETYPE_U64;
}
template <>
SampleType SynchronousSignal<float>::getSampleType() const
{
    return SAMPLETYPE_REAL32;
}
template <>
SampleType SynchronousSignal<double>::getSampleType() const
{
    return SAMPLETYPE_REAL64;
}
template <>
SampleType SynchronousSignal<Complex32Type>::getSampleType() const
{
    return SAMPLETYPE_COMPLEX32;
}
template <>
SampleType SynchronousSignal<Complex64Type>::getSampleType() const
{
    return SAMPLETYPE_COMPLEX64;
}
END_NAMESPACE_STREAMING_PROTOCOL
