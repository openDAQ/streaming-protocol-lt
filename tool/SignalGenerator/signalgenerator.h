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

#pragma once

#include <map>
#include <memory>

#include "stream/Stream.hpp"

#include "streaming_protocol/StreamWriter.h"
#include "generatedtimesignal.h"
#include "generatedasynchronoussignal.h"
#include "generatedsynchronoussignal.h"
#include "signaldefinition.h"

namespace daq::streaming_protocol::siggen {
    static const char AMPLITUDE[] = "amplitude";
    static const char OFFSET[] = "offset";
    static const char FREQUENCY[] = "frequency";
    static const char DUTYCYCLE[] = "dutyCycle";
    static const char EXPLICITTIME[] = "explicitTime";
    static const char DELAY[] = "delay";
    static const char SAMPLEPERIOD[] = "samplePeriod";

    static const char EXECUTIONTIME[] = "executionTime";
    static const char PROCESSPERIOD[] = "processPeriod";

    class StdoutWriter;
    class SignalGenerator {
    public:
        SignalGenerator(std::chrono::system_clock::time_point startTime, std::shared_ptr<daq::stream::Stream> stream);
        SignalGenerator(const SignalGenerator&) = delete;
        ~SignalGenerator();

        /// create a new signal with the given properties
        /// template type determines data type of the signal and the paramters amplitude and offset. The internal handling of the signal will be done with this data type.
        /// \param name Signal name. Should be unique on this server.
        /// \param signalParameters Signal parameters
        /// \param samplePeriod Samples are taken with this rate. Not to be confused with the signal frequency!
        /// \param startTime The abolute start time can be used to enable a signal later than others. Usefull for implementing a phase shift between signals
        /// \return > -1 on error, 0 on success
        template < typename T >
        int addSynchronousSignal(const std::string &signalId, const std::string &tableId, const std::string &timeSignalId,
                                    FunctionParameters<T> signalParameters,
                                    std::chrono::nanoseconds samplePeriod, uint64_t valueIndex)
        {

            if (m_generatedSignals.find(signalId)!=m_generatedSignals.end()) {
                STREAMING_PROTOCOL_LOG_E("signal id is already ins use!");
                return -1;
            }

            std::unique_ptr < iGeneratedSignal > pGeneratedSignal(std::make_unique<GeneratedSynchronousSignal<T>> (signalId, tableId, timeSignalId, signalParameters, samplePeriod, m_time, valueIndex, m_writer));
            // make sure signal exists before telling it is available
            m_generatedSignals.insert(std::make_pair(signalId, std::move(pGeneratedSignal)));
            return 0;
        }
        template < typename T >
        int addAsynchronousSignal(const std::string &signalId, const std::string &tableId, const std::string &timeSignalId,
                                    FunctionParameters<T> signalParameters,
                                    std::chrono::nanoseconds samplePeriod)
        {
            if (m_generatedSignals.find(signalId)!=m_generatedSignals.end()) {
                STREAMING_PROTOCOL_LOG_E("signal id is already ins use!");
                return -1;
            }

            std::unique_ptr < iGeneratedSignal > pGeneratedSignal(std::make_unique<GeneratedAsynchronousSignal<T>> (signalId, tableId, timeSignalId, signalParameters, samplePeriod, m_time, m_writer));
            // make sure signal exists before telling it is available
            m_generatedSignals.insert(std::make_pair(signalId, std::move(pGeneratedSignal)));
            return 0;
        }

       int addLinearTimeSignal(const std::string &signalId, const std::string &tableId, std::chrono::nanoseconds samplePeriod)
       {
            if (m_generatedSignals.find(signalId)!=m_generatedSignals.end()) {
                STREAMING_PROTOCOL_LOG_E("signal id is already ins use!");
                return -1;
            }

            std::unique_ptr < iGeneratedSignal > pGeneratedSignal(
                std::make_unique<GeneratedTimeSignal<streaming_protocol::LinearTimeSignal>> (signalId, tableId, samplePeriod, 1000000000, m_time, m_writer));
            // make sure signal exists before telling it is available
            m_generatedSignals.insert(std::make_pair(signalId, std::move(pGeneratedSignal)));
            return 0;
       }

       int addExplicitTimeSignal(const std::string &signalId, const std::string &tableId, std::chrono::nanoseconds samplePeriod)
       {
            if (m_generatedSignals.find(signalId)!=m_generatedSignals.end()) {
                STREAMING_PROTOCOL_LOG_E("signal id is already ins use!");
                return -1;
            }

            std::unique_ptr < iGeneratedSignal > pGeneratedSignal(
                std::make_unique<GeneratedTimeSignal<streaming_protocol::ExplicitTimeSignal>> (signalId, tableId, samplePeriod, 1000000000, m_time, m_writer));
            // make sure signal exists before telling it is available
            m_generatedSignals.insert(std::make_pair(signalId, std::move(pGeneratedSignal)));
            return 0;
       }

        /// replaces complete existing configuration
        int configureFromFile(const std::string& configFileName);


        /// replaces complete existing configuration
        /// \param config Configuration provided by json document
        ///
        /// duration and process period may be read from json of following form:
        /// \code
        ///{
        ///  "duration": "<string time duration>"
        ///  "processPeriod": "<string time duration>"
        ///}
        /// \endcode
        /// duration defaults to 10s if not provided
        /// process period defaults to 10µs if not provided
        ///
        /// signal definition is read from json of following form:
        /// omitted signal parameters will be set 0
        /// \code
        ///{
        ///	"signals" : {
        ///		"<signal name>" :
        ///		{
        ///			"dataType" : "real32" | "real64" | "int32",
        ///			"function" : "sine" | "rectangle" | "impulse" | "constant" | "sawtooth",
        ///			"amplitude" : <double>,
        ///			"offset" : <double>,
        ///			"frequency" : <double>,
        ///			"dutyCycle" : <double>,
        ///			"samplePeriod" : "<string time duration>",
        ///			"delay": "<string time duration>",
        ///			"explicitTime": <bool>
        ///		},
        ///		"<another signal name>" :
        ///		{
        ///			...
        ///		}
        /// }
        ///}
        /// \endcode
        int configure(const nlohmann::json& configJson);

        /// \param processPeriod processPeriod Processing of accumulated signal data takes place with this rate.
        /// \param executionTime time to run 0 for indefinitely
        void configureTimes(std::chrono::nanoseconds processPeriod, std::chrono::nanoseconds executionTime);

        size_t removeSignal(const std::string& signalId);
        size_t signalCount() const;

        /// All signals will be removed!
        void clear();

        /// Start processing as configured
        /// \param realTime Wait for processPeriod after each cycle if true, otherwise process as fast as possible
        void start(bool realTime);
        void stop();

    private:

        template < typename T >
        int configureSignal(const std::string& id, const nlohmann::json &configSignal)
        {
            daq::streaming_protocol::siggen::FunctionType functionType;
            std::string function = configSignal.value<std::string>("function", std::string());
            if (function=="sine") {
                functionType = FUNCTION_TYPE_SINE;
            } else if (function=="rectangle") {
                functionType = FUNCTION_TYPE_RECTANGLE;
            } else if (function=="impulse") {
                functionType = FUNCTION_TYPE_IMPULSE;
            } else if (function=="constant") {
                functionType = FUNCTION_TYPE_CONSTANT;
            } else if (function=="sawtooth") {
                functionType = FUNCTION_TYPE_SAWTOOTH;
            } else {
                STREAMING_PROTOCOL_LOG_E("invalid function type");
                return -1;
            }

            std::chrono::nanoseconds samplePeriod;
            std::chrono::nanoseconds delay;
            bool explicitTime = configSignal.value<bool>(EXPLICITTIME, false);
            std::string delayString = configSignal.value<std::string>(DELAY, std::string());
            std::string samplePeriodString = configSignal.value<std::string>(SAMPLEPERIOD, std::string());
            delay = durationFromString(delayString, std::chrono::seconds::zero());
            samplePeriod = durationFromString(samplePeriodString, std::chrono::seconds::zero());

            daq::streaming_protocol::Range range;
            auto rangeConfig = configSignal.value(META_RANGE, R"({})"_json);
            range.low = rangeConfig.value<double>(META_LOW, range.low);
            range.high = rangeConfig.value<double>(META_HIGH, range.high);

            daq::streaming_protocol::PostScaling postScaling;
            auto postScalingConfig = configSignal.value(META_POSTSCALING, R"({})"_json);
            postScaling.scale = postScalingConfig.value<double>(META_SCALE, postScaling.scale);
            postScaling.offset = postScalingConfig.value<double>(META_POFFSET, postScaling.offset);

            FunctionParameters <T> signalParameters;
            signalParameters.amplitude = configSignal.value<T>(AMPLITUDE, 0);
            signalParameters.offset = configSignal.value<T>(OFFSET, 0);
            signalParameters.frequency = configSignal.value<double>(FREQUENCY, 0.0);
            signalParameters.dutyCycle = configSignal.value<double>(DUTYCYCLE, 0.0);
            signalParameters.functionType = functionType;
            signalParameters.range = range;
            signalParameters.postScaling = postScaling;

            std::string tableId = configSignal.value<std::string>(META_TABLEID, std::string());
            if (tableId.empty())
                tableId = id + "_table";

            std::string timeSignalId = "A_" + id + "_time";
            if (explicitTime) {
                // asynchronous signal
                addExplicitTimeSignal(timeSignalId, tableId, samplePeriod);
                addAsynchronousSignal<T>(id, tableId, timeSignalId, signalParameters, samplePeriod);
            } else {
                // synchronous signal
                addLinearTimeSignal(timeSignalId, tableId, samplePeriod);
                addSynchronousSignal<T>(id, tableId, timeSignalId, signalParameters, samplePeriod, 0);
            }
            return 0;
        }

        /// signal id is key
        using GeneratedSignals = std::map < std::string, std::unique_ptr < iGeneratedSignal > >;

        std::shared_ptr<daq::stream::Stream> m_stream;
        streaming_protocol::StreamWriter m_writer;
        GeneratedSignals m_generatedSignals;
        std::chrono::system_clock::time_point m_time;
        std::chrono::system_clock::time_point m_endTime;
        std::chrono::nanoseconds m_processPeriod;
        std::chrono::nanoseconds m_executionTime;
        streaming_protocol::LogCallback logCallback;
    };
}
