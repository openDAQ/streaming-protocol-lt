/*
 * Copyright 2022-2024 openDAQ d.o.o.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <chrono>
#include <fstream>
#include <iostream>
#include <memory>
#include <thread>

#include <sys/types.h>

#ifdef _WIN32
/// \warning: Some headers of MSVC define max. This leads problems with chrono::timepoint::max().
/// Do not change the sequence of the includes
#undef max
#endif

#include "boost/asio/error.hpp"

#include "nlohmann/json.hpp"

#include "streaming_protocol/Logging.hpp"
#include "streaming_protocol/ProducerSession.hpp"

#include "signalgenerator.h"


#ifdef _WIN32
#define getpid() _getpid()
#else
#include <unistd.h>
#endif

daq::streaming_protocol::siggen::SignalGenerator::SignalGenerator(std::chrono::system_clock::time_point startTime, std::shared_ptr<daq::stream::Stream> stream)
    : m_stream(stream)
    , m_writer(stream)
    , m_time(startTime)
    , logCallback(daq::streaming_protocol::Logging::logCallback())
{
    STREAMING_PROTOCOL_LOG_D("openDAQ LT Streaming Protocol!");
}

daq::streaming_protocol::siggen::SignalGenerator::~SignalGenerator()
{
    clear();
}

size_t daq::streaming_protocol::siggen::SignalGenerator::removeSignal(const std::string &signalId)
{
    return m_generatedSignals.erase(signalId);
}

int daq::streaming_protocol::siggen::SignalGenerator::configureFromFile(const std::string& configFileName)
{
    std::ifstream configFile;
    configFile.open(configFileName);
    if (!configFile) {
        STREAMING_PROTOCOL_LOG_E("could not open config file '{}'", configFileName);
        return -1;
    }
    nlohmann::json configJson;
    try	{
        configJson = nlohmann::json::parse(configFile);
    }
    catch (nlohmann::json::parse_error& e) {
        STREAMING_PROTOCOL_LOG_E("could not parse confige: {}", e.what());
        STREAMING_PROTOCOL_LOG_E("exception id: {} byte position of error: {}", e.id, e.byte);
        return -1;
    }
    return configure(configJson);
}


int daq::streaming_protocol::siggen::SignalGenerator::configure(const nlohmann::json &configJson)
{
    clear();
    std::chrono::nanoseconds processPeriod = daq::streaming_protocol::siggen::durationFromString(configJson.value<std::string>(PROCESSPERIOD, std::string()), std::chrono::microseconds(10));
    std::chrono::nanoseconds executionTime = daq::streaming_protocol::siggen::durationFromString(configJson.value<std::string>(EXECUTIONTIME, std::string()), std::chrono::seconds(10));
    configureTimes(processPeriod, executionTime);

    int result;

    for(const auto& iter: configJson["signals"].items()) {
        std::string id = iter.key();
        nlohmann::json value = iter.value();

        // real32, real64, int32, int64
        std::string dataTypeString = value.value<std::string>(META_DATATYPE, std::string(DATA_TYPE_REAL64));
        if (dataTypeString==DATA_TYPE_REAL64) {
            result = configureSignal<double>(id, value);
        } else if (dataTypeString==DATA_TYPE_REAL32) {
            result = configureSignal<float>(id, value);
        } else if (dataTypeString==DATA_TYPE_INT32) {
            result = configureSignal<int32_t>(id, value);
        } else if (dataTypeString==DATA_TYPE_INT64) {
            result = configureSignal<int64_t>(id, value);
        } else {
            STREAMING_PROTOCOL_LOG_E("invalid data type '{}'", dataTypeString);
            return -1;
        }
        if (result==-1) {
            return -1;
        }
    }
    return 0;
}

void daq::streaming_protocol::siggen::SignalGenerator::configureTimes(std::chrono::nanoseconds processPeriod, std::chrono::nanoseconds executionTime)
{
    m_processPeriod = processPeriod;
    m_executionTime = executionTime;
}

size_t daq::streaming_protocol::siggen::SignalGenerator::signalCount() const
{
    return m_generatedSignals.size();
}

void daq::streaming_protocol::siggen::SignalGenerator::clear()
{
    m_generatedSignals.clear();
}

void daq::streaming_protocol::siggen::SignalGenerator::stop()
{
    m_endTime = m_time;
}

void daq::streaming_protocol::siggen::SignalGenerator::start(bool realTime)
{
    if (m_executionTime==std::chrono::nanoseconds::zero()) {
        m_endTime = std::chrono::time_point<std::chrono::system_clock>::max();
        STREAMING_PROTOCOL_LOG_D("executing indefinitely");
    } else {
        m_endTime = addNanosecondsToTimePoint(m_time, m_executionTime);
        STREAMING_PROTOCOL_LOG_D("executing for {} µs", m_executionTime.count() / 1000);
    }

    // collect all signals that are involved
    ProducerSession::Signals allSignals;
    SignalIds allSignalIds;
    for (const auto &generatedSignal : m_generatedSignals)
    {
        allSignals[generatedSignal.first] = generatedSignal.second->getSignal();
        allSignalIds.push_back(generatedSignal.first);
    }

    // Streaming producer is created here! Session will be initiated
    auto producerSession = std::make_shared<ProducerSession>(m_stream, nlohmann::json(), logCallback);

    auto errorCb = [this](boost::system::error_code ec) {
        // Stop on error. Also on disconnect by the client or stop by producer!
        if (ec==boost::asio::error::eof) {
            STREAMING_PROTOCOL_LOG_I("Client disconnected. Stoppping session!");
        } else if (ec==boost::asio::error::operation_aborted) {
            STREAMING_PROTOCOL_LOG_I("Session stopped by producer!");
        } else {
            STREAMING_PROTOCOL_LOG_E("Error on read: {} Stopping session!", ec.message());
        }
        stop();
    };
    // start connection observation
    producerSession->start(errorCb);
    producerSession->addSignals(allSignals);
    producerSession->subscribeSignals(allSignalIds);
    // some signals require some addtional information to be send (i.e. start time for synchronous signals)
    for (const auto &iter : m_generatedSignals)
    {
        iter.second->initAfterSubscribeAck();
    }

    // Produce and stream data...
    while (m_time<=m_endTime) {
        for (auto &iter: m_generatedSignals) {
            if(iter.second->process()<0) {
                STREAMING_PROTOCOL_LOG_E("aborting session");
                return;
            }
        }

        if (realTime) {
            std::this_thread::sleep_for(m_processPeriod);
        }

        m_time = addNanosecondsToTimePoint(m_time, m_processPeriod);
    }

    // shut down the stream
    producerSession->unsubscribeSignals(allSignalIds);
    producerSession->removeSignals(allSignalIds);
    producerSession->stop();

    // Stream Session will be destroyed when leaving
    STREAMING_PROTOCOL_LOG_D("finished session!");
}

