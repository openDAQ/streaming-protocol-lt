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

#include "signalgenerator.h"

#include "signaldefinition.h"



std::chrono::nanoseconds daq::streaming_protocol::siggen::durationFromString(const std::string& durationString, std::chrono::nanoseconds defaultValue)
{
	unsigned int uintValue;
	char unit[128];
	sscanf(durationString.c_str(), "%u%s", &uintValue, unit);
	if (strcmp(unit, "s")==0) {
		return std::chrono::seconds(uintValue);
	} else if (strcmp(unit, "ms")==0) {
		return std::chrono::milliseconds(uintValue);
	} else if (strcmp(unit, "µs")==0) {
		return std::chrono::microseconds(uintValue);
	} else if (strcmp(unit, "ns")==0) {
		return std::chrono::nanoseconds(uintValue);
	} else {
		return defaultValue;
	}
}

void daq::streaming_protocol::siggen::addSignals(daq::streaming_protocol::siggen::SignalGenerator& signalGenerator)
{
    std::chrono::nanoseconds samplePeriod;
    FunctionParameters <double> dblSignalParameters;

    //samplePeriod = std::chrono::milliseconds(50);
    //dblSignalParameters.amplitude = 2.5;
    //dblSignalParameters.offset = 1.5;
    //dblSignalParameters.frequency = 1.0;
    //dblSignalParameters.dutyCycle = 0.5;
    //dblSignalParameters.functionType = daq::streaming_protocol::siggen::FUNCTION_TYPE_RECTANGLE;
    //signalGenerator.addAsynchronousSignal<double>("async_square", dblSignalParameters, samplePeriod, timeTicksPerSecond);

    samplePeriod = std::chrono::milliseconds(10);
    dblSignalParameters.amplitude = 10;
    dblSignalParameters.offset = 0;
    dblSignalParameters.frequency = 0.1;
    dblSignalParameters.functionType = daq::streaming_protocol::siggen::FUNCTION_TYPE_SINE;
    signalGenerator.addLinearTimeSignal("sine_time", "table_10ms",samplePeriod);
    signalGenerator.addSynchronousSignal<double>("sine", "table_10ms", dblSignalParameters, samplePeriod, 0);

    samplePeriod = std::chrono::milliseconds(100);
    dblSignalParameters.amplitude = 2.5;
    dblSignalParameters.offset = 0;
    dblSignalParameters.frequency = 0.2;
    dblSignalParameters.functionType = daq::streaming_protocol::siggen::FUNCTION_TYPE_SAWTOOTH;
    signalGenerator.addLinearTimeSignal("saw_tooth_time", "table_100ms", samplePeriod);
    signalGenerator.addSynchronousSignal<double>("saw_tooth", "table_100ms", dblSignalParameters, samplePeriod, 0);
    // ToDo: This should work for test purposes - but the handling of the value index is not in sync to the correct time.
    signalGenerator.addSynchronousSignal<double>("saw_tooth_2", "table_100ms", dblSignalParameters, samplePeriod, 100);

    samplePeriod = std::chrono::milliseconds(100);
    dblSignalParameters.amplitude = 2.5;
    dblSignalParameters.offset = 0;
    dblSignalParameters.frequency = 0.5;
    dblSignalParameters.dutyCycle = 0.0;
    dblSignalParameters.functionType = FUNCTION_TYPE_IMPULSE;
    signalGenerator.addSynchronousSignal("impulse", "table_100ms", dblSignalParameters, samplePeriod, 0);

    // we want whole numbers here!
    samplePeriod = std::chrono::seconds(1);
    FunctionParameters <int32_t> int32SignalParameters;
    int32SignalParameters.amplitude = 160000;
    int32SignalParameters.offset = 0;
    int32SignalParameters.frequency = 1.0;
    int32SignalParameters.dutyCycle = 0.0;
    int32SignalParameters.functionType = FUNCTION_TYPE_SAWTOOTH;
    signalGenerator.addLinearTimeSignal("slow_counter_time", "table_1s", samplePeriod);
    signalGenerator.addSynchronousSignal<int32_t>("slow_counter", "table_1s", int32SignalParameters, samplePeriod, 0);

}
