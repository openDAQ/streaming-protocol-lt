#include "streaming_protocol/AsynchronousSignal.hpp"
#include "streaming_protocol/Types.h"

BEGIN_NAMESPACE_STREAMING_PROTOCOL


template <>
nlohmann::json AsynchronousSignal<int8_t>::getMemberInformation() const
{
    return createMember(DATA_TYPE_INT8);
}

template <>
nlohmann::json AsynchronousSignal<uint8_t>::getMemberInformation() const
{
    return createMember(DATA_TYPE_UINT8);
}

template <>
nlohmann::json AsynchronousSignal<int16_t>::getMemberInformation() const
{
    return createMember(DATA_TYPE_INT16);
}

template <>
nlohmann::json AsynchronousSignal<uint16_t>::getMemberInformation() const
{
    return createMember(DATA_TYPE_UINT16);
}

template <>
nlohmann::json AsynchronousSignal<int32_t>::getMemberInformation() const
{
    return createMember(DATA_TYPE_INT32);
}

template <>
nlohmann::json AsynchronousSignal<uint32_t>::getMemberInformation() const
{
    return createMember(DATA_TYPE_UINT32);
}

template <>
nlohmann::json AsynchronousSignal<int64_t>::getMemberInformation() const
{
    return createMember(DATA_TYPE_INT64);
}

template <>
nlohmann::json AsynchronousSignal<uint64_t>::getMemberInformation() const
{
    return createMember(DATA_TYPE_UINT64);
}

template <>
nlohmann::json AsynchronousSignal<float>::getMemberInformation() const
{
    return createMember(DATA_TYPE_REAL32);
}

template <>
nlohmann::json AsynchronousSignal<double>::getMemberInformation() const
{
    return createMember(DATA_TYPE_REAL64);
}

template <>
nlohmann::json AsynchronousSignal<Complex32Type>::getMemberInformation() const
{
    return createMember(DATA_TYPE_COMPLEX32);
}

template <>
nlohmann::json AsynchronousSignal<Complex64Type>::getMemberInformation() const
{
    return createMember(DATA_TYPE_COMPLEX64);
}

template <>
SampleType AsynchronousSignal<int8_t>::getSampleType() const
{
    return SAMPLETYPE_S8;
}
template <>
SampleType AsynchronousSignal<int16_t>::getSampleType() const
{
    return SAMPLETYPE_S16;
}
template <>
SampleType AsynchronousSignal<int32_t>::getSampleType() const
{
    return SAMPLETYPE_S32;
}
template <>
SampleType AsynchronousSignal<int64_t>::getSampleType() const
{
    return SAMPLETYPE_S64;
}
template <>
SampleType AsynchronousSignal<uint8_t>::getSampleType() const
{
    return SAMPLETYPE_U8;
}
template <>
SampleType AsynchronousSignal<uint16_t>::getSampleType() const
{
    return SAMPLETYPE_U16;
}
template <>
SampleType AsynchronousSignal<uint32_t>::getSampleType() const
{
    return SAMPLETYPE_U32;
}
template <>
SampleType AsynchronousSignal<uint64_t>::getSampleType() const
{
    return SAMPLETYPE_U64;
}
template <>
SampleType AsynchronousSignal<float>::getSampleType() const
{
    return SAMPLETYPE_REAL32;
}
template <>
SampleType AsynchronousSignal<double>::getSampleType() const
{
    return SAMPLETYPE_REAL64;
}
template <>
SampleType AsynchronousSignal<Complex32Type>::getSampleType() const
{
    return SAMPLETYPE_COMPLEX32;
}
template <>
SampleType AsynchronousSignal<Complex64Type>::getSampleType() const
{
    return SAMPLETYPE_COMPLEX64;
}
END_NAMESPACE_STREAMING_PROTOCOL
