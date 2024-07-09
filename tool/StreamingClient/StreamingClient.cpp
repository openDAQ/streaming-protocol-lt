/*
 * Copyright 2022-2023 Blueberry d.o.o.
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
#include <csignal>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <unordered_map>

#include "boost/program_options.hpp"
#include <boost/asio/io_context.hpp>

#include "stream/TcpClientStream.hpp"
#include "stream/WebsocketClientStream.hpp"

#include "streaming_protocol/Defines.h"
#include "streaming_protocol/Logging.hpp"
#include "streaming_protocol/ProtocolHandler.hpp"
#include "streaming_protocol/SignalContainer.hpp"
#include "streaming_protocol/SubscribedSignal.hpp"

// LogCallback
static daq::streaming_protocol::LogCallback logCallback = daq::streaming_protocol::Logging::logCallback();

// The io_context is required for all I/O
boost::asio::io_context s_ioc;

#define DOUBLE_VALUE_BUFFER_SIZE 1024
double doubleValueBuffer[DOUBLE_VALUE_BUFFER_SIZE];

class SignalInfo
{

public:
    SignalInfo(const std::string& signalId)
            : m_signalId(signalId)
            , m_startTime(std::chrono::system_clock::now())
            , m_valueCount(0)
    {
    }

    void incrementValueCount(size_t count)
    {
        m_valueCount += count;
    }

    void print() const
    {
        std::chrono::time_point<std::chrono::system_clock> t2 = std::chrono::system_clock::now();
        auto milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(t2 - m_startTime);
        std::cout << m_signalId << ":\n\tUnsubscribed after " << milliseconds.count() << " milliseconds" << std::endl;
        std::cout << "\t" << m_valueCount << "(" << (static_cast< double >(m_valueCount) / milliseconds.count() * 1000) << " values per second)" << std::endl;
    }
private:
    std::string m_signalId;
    /// set on subscribe ack
    std::chrono::time_point<std::chrono::system_clock> m_startTime;
    /// cleared on subscribe ack and incremented on arrival of values
    size_t m_valueCount;

};

/// signal number is key
using SignalInfos = std::unordered_map < unsigned int, SignalInfo >;
using SignalFilterParameter = std::vector < std::string >;
using SignalFilter = std::set < std::string >;
static SignalInfos s_signalInfos;
static SignalFilter s_signalFilter;


/// Convert to double and print to stdout
/// \todo timestamp is to be taken from subscribedSignal
//static void valueDataCb(const daq::streaming_protocol::SubscribedSignal& subscribedSignal, //uint64_t timeStamp, const uint8_t* data, size_t valueCount)
//{
//    const uint8_t* pPos = data;
//    size_t valuesToProcess;
static void rawDataCb(const daq::streaming_protocol::SubscribedSignal& subscribedSignal, uint64_t timeStamp, const uint8_t* data, size_t size)
{
    const uint8_t* pPos = data;
    size_t valuesToProcess;
    size_t valuesConverted;
    size_t valueCount = size/subscribedSignal.dataValueSize();

    std::cout << subscribedSignal.signalId() << " time: " << timeStamp << " (value(s)=" << valueCount << "): ";
    std::cout << std::endl;

    size_t valuesRemaining = valueCount;
    while (valuesRemaining) {
        if (valuesRemaining > DOUBLE_VALUE_BUFFER_SIZE) {
            valuesToProcess = DOUBLE_VALUE_BUFFER_SIZE;
        } else {
            valuesToProcess = valuesRemaining;
        }
        valuesConverted = subscribedSignal.interpretValuesAsDouble(pPos, valuesToProcess, doubleValueBuffer);

        for(size_t valueIndex=0; valueIndex<valuesConverted; ++valueIndex) {
            std::cout << doubleValueBuffer[valueIndex] << " ";
        }
        valuesRemaining -= valuesToProcess;
    }
    // update statistics...
    s_signalInfos.at(subscribedSignal.signalNumber()).incrementValueCount(valueCount);

    std::cout << subscribedSignal.unitDisplayName() << " \n" << std::endl;
}

static void signalMetaCb(const daq::streaming_protocol::SubscribedSignal& subscribedSignal, const std::string& method, const nlohmann::json& params)
{
    if (method == daq::streaming_protocol::META_METHOD_SUBSCRIBE) {
        s_signalInfos.emplace(subscribedSignal.signalNumber(), subscribedSignal.signalId());
        std::cout << __FUNCTION__ << " signal\n\t" << subscribedSignal.signalId() << " (signal number " << subscribedSignal.signalNumber() << ") got subscribed" << std::endl;
        return;
    } else if (method == daq::streaming_protocol::META_METHOD_UNSUBSCRIBE) {
        s_signalInfos.at(subscribedSignal.signalNumber()).print();
        s_signalInfos.erase(subscribedSignal.signalNumber());
    }

    std::cout << __FUNCTION__ << " signal: '" << subscribedSignal.signalId() << "', method: '" << method << "':\n" << params.dump(2) << std::endl;
}

static void streamMetaInformationCb(daq::streaming_protocol::ProtocolHandler& protocolHandler, const std::string& method, const nlohmann::json& params)
{
    if (method == daq::streaming_protocol::META_METHOD_APIVERSION) {
        std::cout << __FUNCTION__ << " " << method << ": " << params[daq::streaming_protocol::VERSION] << std::endl;
    } else if (method == daq::streaming_protocol::META_METHOD_AVAILABLE) {
        // Simply subscribe all signals that become available. When there is a signal filter, only macthing signals are to be used

        std::cout << __FUNCTION__ << ": Some signals became available: ==============" << std::endl;
        daq::streaming_protocol::SignalIds signalIds;
        for (const auto& element: params[daq::streaming_protocol::META_SIGNALIDS]) {
            std::string signalId = element.get<std::string>();
            if (s_signalFilter.empty()) {
                signalIds.emplace_back(signalId);
            } else {
                if (s_signalFilter.find(signalId) != s_signalFilter.end()) {
                    signalIds.emplace_back(signalId);
                }
            }
            std::cout << "\t" << signalId << std::endl;
        }
        std::cout << __FUNCTION__ << ": =============================================" << std::endl;
        protocolHandler.subscribe(signalIds);
        std::cout << std::endl;
    } else if(method == daq::streaming_protocol::META_METHOD_INIT) {
        // this was handled already internally
    } else if(method == daq::streaming_protocol::META_METHOD_UNAVAILABLE) {
        std::cout << __FUNCTION__ << ": The following signal(s) are not available anymore: ";
        for (const auto& element: params[daq::streaming_protocol::META_SIGNALIDS]) {
            std::cout << element << ", ";
        }
        std::cout << std::endl;
    } else if(method == daq::streaming_protocol::META_METHOD_ALIVE) {
        nlohmann::json::const_iterator valueIter = params.find(daq::streaming_protocol::META_FILLLEVEL);
        if (valueIter != params.end()) {
            unsigned int fill;
            fill = valueIter.value();
            if(fill > 25) {
                std::cout << "Ring buffer fill level is " << fill << "%" << std::endl;
            }
        }
    } else {
        std::cout << __FUNCTION__ << " " << method << " " << params.dump(2) << std::endl;
    }
}

static void sigHandler(int)
{
    s_ioc.stop();
}


int main(int argc, char** argv)
{
    // Some signals should lead to a normal shutdown of the daq stream client. Afterwards the program exists.
    // Values that are left in buffers won't be read!
    signal( SIGTERM, &sigHandler);
    signal( SIGINT, &sigHandler);

    boost::program_options::options_description optionsDescription("Options");

    optionsDescription.add_options()
            ("help", "help message")
            ("host", boost::program_options::value < std::string >(), "Address/name of streaming server")
            ("file", boost::program_options::value < std::string >(), "File name of dumped stream")
            ("port", boost::program_options::value < std::string >(), "Listening port of streaming server")
            ("target", boost::program_options::value < std::string >(), "Service path for websocket")
            ("quiet", "Do not print measured values")
            // signalFilter is a list of signal ids. Only available signal ids mentioned in the list are to be subscribed
            ("signalFilter", boost::program_options::value<SignalFilterParameter>()->multitoken(), "Signal filter, if given only those are to be subscribed");
            ;

    boost::program_options::variables_map optionMap;
    try {
        boost::program_options::store(boost::program_options::parse_command_line(argc, argv, optionsDescription), optionMap);
    } catch(...) {
        std::cout << optionsDescription << "\n";
        return EXIT_FAILURE;
    }

    boost::program_options::notify(optionMap);

    if (optionMap.empty() || optionMap.count("help")) {
        std::cout << optionsDescription << "\n";
        return EXIT_SUCCESS;
    }

    std::string fileName;
    std::string host;
    std::string port;
    std::string target;
    bool quiet;

    if (optionMap.count("host")) {
        host = optionMap["host"].as < std::string >();
    }
    if (optionMap.count("port")) {
        port = optionMap["port"].as < std::string >();
    }
    if (optionMap.count("target")) {
        target = optionMap["target"].as < std::string >();
    }
    if (optionMap.count("quiet")) {
        // In this case we use the default data callback that does nothing...
        std::cout << "Quiet mode. Measured values will not be printed." << std::endl;
        quiet = true;
    } else {
        std::cout << "Printing measured values" << std::endl;
        quiet = false;
    }
    if (optionMap.count("signalFilter")) {
        SignalFilterParameter signalFilterParameter = optionMap["signalFilter"].as <SignalFilterParameter>();
        for (const auto &iter : signalFilterParameter) {
            s_signalFilter.insert(iter);
        }
    }

    daq::streaming_protocol::SignalContainer signalContainer(logCallback);
    signalContainer.setSignalMetaCb(signalMetaCb);
    if (!quiet) {
        signalContainer.setDataAsRawCb(rawDataCb);
        // signalContainer.setDataAsValueCb(valueDataCb);
    }

    std::unique_ptr < daq::stream::Stream > stream;
    auto protocolHandler = std::make_shared < daq::streaming_protocol::ProtocolHandler > (s_ioc, signalContainer, streamMetaInformationCb, logCallback);
    // Prepare the asynchronous operation depending on the protocol to use
    if (!target.empty()) {
        std::cout << "Using websocket..." << std::endl;
        stream = std::make_unique < daq::stream::WebsocketClientStream > (s_ioc, host, port, target);
    } else if (!host.empty()){
        std::cout << "Using raw tcp..." << std::endl;
        stream = std::make_unique < daq::stream::TcpClientStream > (s_ioc, host, port);
    } else {
        std::cout << optionsDescription << "\n";
        return EXIT_SUCCESS;
    }

    auto completionCb = [](const boost::system::error_code& ec) {
        STREAMING_PROTOCOL_LOG_E("{}", ec.message());
    };

    protocolHandler->start(std::move(stream), completionCb);
    // Run the I/O service. This will process all prepared asynchronous operations and data received until the session stops.
    // The call will return when the session is closed or an error occurs.
    s_ioc.run();
    for (const auto& iter : s_signalInfos) {
        iter.second.print();
    }

    return EXIT_SUCCESS;
}
