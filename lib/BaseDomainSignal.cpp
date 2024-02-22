#include <mutex>
#include <string>

#include "nlohmann/json.hpp"
#include "streaming_protocol/BaseDomainSignal.hpp"

#include <iostream>


namespace daq::streaming_protocol {

BaseDomainSignal::BaseDomainSignal(const std::string& signalId, const std::string tableId, uint64_t timeTicksPerSecond, iWriter &writer, LogCallback logCb)
    : BaseSignal(signalId + "_time", tableId, writer, logCb)
    , m_timeTicksPerSecond(timeTicksPerSecond)
{
}

void BaseDomainSignal::setEpoch(const std::string& epoch)
{
    m_epoch = epoch;
}

void BaseDomainSignal::setEpoch(const std::chrono::system_clock::time_point& epoch)
{
    std::time_t time = std::chrono::system_clock::to_time_t(epoch);

    auto tf = *std::gmtime(&time);
    tf.tm_isdst = 1;

    char buf[64];
    std::strftime(buf, sizeof(buf), "%FT%TZ", &tf);
    m_epoch = buf;
}

void BaseDomainSignal::setTimeTicksPerSecond(uint64_t timeTicksPerSecond)
{
    m_timeTicksPerSecond = timeTicksPerSecond;
}

uint64_t BaseDomainSignal::getTimeTicksPerSecond() const
{
    return m_timeTicksPerSecond;
}

uint64_t BaseDomainSignal::timeTicksFromNanoseconds(std::chrono::nanoseconds ns, uint64_t m_timeTicksPerSecond)
{
    double nsAsUint64 = std::chrono::duration < double >(ns).count();
    return static_cast<uint64_t>(m_timeTicksPerSecond * nsAsUint64);
}

std::chrono::nanoseconds BaseDomainSignal::nanosecondsFromTimeTicks(uint64_t timeTicks, uint64_t m_timeTicksPerSecond)
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

uint64_t BaseDomainSignal::timeTicksFromTime(const std::chrono::time_point<std::chrono::system_clock> &time, uint64_t m_timeTicksPerSecond)
{
    // currently we use the unix epoch as fixed epoch!
    auto duration = std::chrono::duration<double>(time.time_since_epoch());
    double timeSinceUnixEpochDouble = duration.count();
    uint64_t timeTicks = static_cast<uint64_t>(timeSinceUnixEpochDouble * m_timeTicksPerSecond);
    return timeTicks;
}

std::chrono::time_point<std::chrono::system_clock> BaseDomainSignal::timeFromTimeTicks(uint64_t timeTicks, uint64_t m_timeTicksPerSecond)
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

std::string BaseDomainSignal::getEpoch() const
{
    return m_epoch;
}

uint64_t BaseDomainSignal::getTimeStart() const
{
    return m_startInTicks;
}

void BaseDomainSignal::setTimeStart(uint64_t timeTicks)
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
    int result = m_writer.writeSignalData(m_signalNumber, reinterpret_cast<uint8_t*>(&startValue), sizeof(startValue));
    if (result < 0) {
        STREAMING_PROTOCOL_LOG_E("{}: Could not write signal time!", m_signalNumber);
    }
}

}