#include <mutex>
#include <string>

#include "nlohmann/json.hpp"
#include "streaming_protocol/BaseSignal.hpp"

#include <iostream>

#include "streaming_protocol/Types.h"
#include "streaming_protocol/Unit.hpp"

namespace daq::streaming_protocol {

SignalNumber BaseSignal::s_signalNumberCounter = 0;
std::mutex BaseSignal::s_signalNumberMtx;

BaseSignal::BaseSignal(const std::string& signalId, uint64_t timeTicksPerSecond, iWriter &writer, LogCallback logCb)
    : m_dataSignalNumber(nextSignalNumber())
    , m_timeSignalNumber(nextSignalNumber())
    , m_signalId(signalId)
    , m_valueName("value")
    , m_unitId(Unit::UNIT_ID_NONE)
    , m_timeTicksPerSecond(timeTicksPerSecond)
    , m_writer(writer)
    , logCallback(logCb)
{
}

std::string BaseSignal::getId() const
{
    return m_signalId;
}

SignalNumber BaseSignal::getNumber() const
{
    return m_dataSignalNumber;
}

void BaseSignal::subscribe()
{
    // one signal gives a data signal and an artificial time signal
    nlohmann::json subscribeData;
    subscribeData[METHOD] = META_METHOD_SUBSCRIBE;
    subscribeData[PARAMS][META_SIGNALID] = m_signalId;
    m_writer.writeMetaInformation(m_dataSignalNumber, subscribeData);
    nlohmann::json subscribeTime;
    subscribeTime[METHOD] = META_METHOD_SUBSCRIBE;
    subscribeTime[PARAMS][META_SIGNALID] = m_signalId + "_time";
    m_writer.writeMetaInformation(m_timeSignalNumber, subscribeTime);
    writeSignalMetaInformation();
}

void BaseSignal::unsubscribe()
{
    nlohmann::json unsubscribe;
    unsubscribe[METHOD] = META_METHOD_UNSUBSCRIBE;
    m_writer.writeMetaInformation(m_dataSignalNumber, unsubscribe);
    unsubscribe[METHOD] = META_METHOD_UNSUBSCRIBE;
    m_writer.writeMetaInformation(m_timeSignalNumber, unsubscribe);
}

void BaseSignal::setEpoch(const std::string& epoch)
{
    m_epoch = epoch;
}

void BaseSignal::setEpoch(const std::chrono::system_clock::time_point& epoch)
{
    std::time_t time = std::chrono::system_clock::to_time_t(epoch);

    auto tf = *std::gmtime(&time);
    tf.tm_isdst = 1;

    char buf[64];
    std::strftime(buf, sizeof(buf), "%FT%TZ", &tf);
    m_epoch = buf;
}

void BaseSignal::setMemberName(const std::string &name)
{
    m_valueName = name;
}

std::string BaseSignal::getMemberName() const
{
    return m_valueName;
}

void BaseSignal::setTimeTicksPerSecond(uint64_t timeTicksPerSecond)
{
    m_timeTicksPerSecond = timeTicksPerSecond;
}

uint64_t BaseSignal::getTimeTicksPerSecond() const
{
    return m_timeTicksPerSecond;
}

uint64_t BaseSignal::timeTicksFromNanoseconds(std::chrono::nanoseconds ns, uint64_t m_timeTicksPerSecond)
{
    double nsAsUint64 = std::chrono::duration < double >(ns).count();
    return static_cast<uint64_t>(m_timeTicksPerSecond * nsAsUint64);
}

std::chrono::nanoseconds BaseSignal::nanosecondsFromTimeTicks(uint64_t timeTicks, uint64_t m_timeTicksPerSecond)
{
    uint64_t seconds = timeTicks / m_timeTicksPerSecond;
    uint64_t subTicks = timeTicks % m_timeTicksPerSecond;
    subTicks *= 1000000000;
    subTicks /= m_timeTicksPerSecond;
    std::chrono::nanoseconds ns(seconds*1000000000);
    std::chrono::nanoseconds subSecondsNs(subTicks);
    ns += subSecondsNs;
    return ns;
}

uint64_t BaseSignal::timeTicksFromTime(const std::chrono::time_point<std::chrono::system_clock> &time, uint64_t m_timeTicksPerSecond)
{
    // currently we use the unix epoch as fixed epoch!
    auto duration = std::chrono::duration<double>(time.time_since_epoch());
    double timeSinceUnixEpochDouble = duration.count();
    uint64_t timeTicks = static_cast<uint64_t>(timeSinceUnixEpochDouble * m_timeTicksPerSecond);
    return timeTicks;
}

std::chrono::time_point<std::chrono::system_clock> BaseSignal::timeFromTimeTicks(uint64_t timeTicks, uint64_t m_timeTicksPerSecond)
{
    // defaults to unix epoch which is the current epoch
    std::chrono::time_point<std::chrono::system_clock> time;

    std::chrono::nanoseconds ns = nanosecondsFromTimeTicks(timeTicks, m_timeTicksPerSecond);
#ifdef TIME_GRANULARITY_NS
    time += ns;
#else
    // many other platforms do not support resolution of 1ns for std::chrono::time_point<std::chrono::system_clock>
    std::chrono::microseconds microseconds = std::chrono::duration_cast<std::chrono::microseconds>(ns);
    time += microseconds;
#endif

    return time;
}

std::string BaseSignal::getEpoch() const
{
    return m_epoch;
}

int32_t BaseSignal::getUnitId() const
{
    return m_unitId;
}

std::string BaseSignal::getUnitDisplayName() const
{
    return m_unitDisplayName;
}

void BaseSignal::setUnit(int32_t unitId, const std::string& displayName)
{
    m_unitId = unitId;
    m_unitDisplayName = displayName;
}

void BaseSignal::setDataInterpretationObject(const nlohmann::json object) {
    m_dataInterpretationObject = object;
}

nlohmann::json BaseSignal::getDataInterpretationObject() const
{
    return m_dataInterpretationObject;
}

void BaseSignal::setTimeInterpretationObject(const nlohmann::json object)
{
    m_timeInterpretationObject = object;
}

nlohmann::json BaseSignal::getTimeInterpretationObject() const
{
    return m_timeInterpretationObject;
}

SignalNumber BaseSignal::nextSignalNumber()
{
    std::lock_guard guard(s_signalNumberMtx);
    ++s_signalNumberCounter;
    if ((s_signalNumberCounter & SIGNAL_NUMBER_MASK) == 0) {
        // Signal number 0 is reserved for stream related stuff and may not be used as signal number!
        ++s_signalNumberCounter;
    }
    return s_signalNumberCounter & SIGNAL_NUMBER_MASK;
}

}
