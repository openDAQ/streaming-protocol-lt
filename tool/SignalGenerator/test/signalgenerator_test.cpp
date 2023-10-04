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
#include <string>

#include <gtest/gtest.h>


#include "../signalgenerator.h"
#include "../stdoutwriter.h"


TEST(siggen_test, test_constant)
{
    daq::siggen::StdoutWriter stdoutWriter;
    daq::siggen::SignalGenerator signalGenerator(std::chrono::system_clock::now(), stdoutWriter);

    daq::siggen::FunctionParameters <double> functionParameters;
	functionParameters.amplitude = 1.0;
	functionParameters.offset = 1.0;
	functionParameters.frequency = 0.0;
	functionParameters.dutyCycle = 0.0;
    functionParameters.functionType = daq::siggen::FUNCTION_TYPE_CONSTANT;
	double value;
	std::string signalID = "thesignal";
	int result;


	result = signalGenerator.addSynchronousSignal(signalID, functionParameters, std::chrono::milliseconds(100), std::chrono::nanoseconds::zero());
	ASSERT_GT(result, 0);
	// using the same signal id again is not allowed!
	result = signalGenerator.addSynchronousSignal(signalID, functionParameters, std::chrono::milliseconds(100), std::chrono::nanoseconds::zero());
	ASSERT_EQ(result, -1);

    daq::siggen::Function < double > function(functionParameters);
	value = function.calculate(100);
	ASSERT_NEAR(value, functionParameters.offset, 0.0001);
}

TEST(siggen_test, test_executiontime)
{
	static const std::chrono::milliseconds executionTime(200);
	std::chrono::milliseconds realTimeDiff;
	std::chrono::milliseconds calcTimeDiff;

	std::chrono::high_resolution_clock::time_point t1Real;
	std::chrono::high_resolution_clock::time_point t2Real;
	std::chrono::high_resolution_clock::time_point t1Calculated;
	std::chrono::high_resolution_clock::time_point t2Calculated;


    daq::siggen::StdoutWriter stdoutWriter;
    daq::siggen::SignalGenerator signalGenerator(std::chrono::system_clock::now(), stdoutWriter);
	signalGenerator.configureTimes(std::chrono::milliseconds(1), executionTime);

	t1Calculated = signalGenerator.geTime();
	t1Real = std::chrono::high_resolution_clock::now();
    signalGenerator.start(true);
	t2Real = std::chrono::high_resolution_clock::now();
	t2Calculated = signalGenerator.geTime();
	realTimeDiff = std::chrono::duration_cast<std::chrono::milliseconds>(t2Real - t1Real);
	ASSERT_NEAR(executionTime.count(), realTimeDiff.count(), 40);
	calcTimeDiff = std::chrono::duration_cast<std::chrono::milliseconds>(t2Calculated - t1Calculated);
	ASSERT_NEAR(executionTime.count(), calcTimeDiff.count(), 2);

	t1Calculated = signalGenerator.geTime();
	t1Real = std::chrono::high_resolution_clock::now();
    signalGenerator.start(false);
	t2Real = std::chrono::high_resolution_clock::now();
	t2Calculated = signalGenerator.geTime();


	realTimeDiff = std::chrono::duration_cast<std::chrono::milliseconds>(t2Real - t1Real);
	ASSERT_NEAR(0, realTimeDiff.count(), 40);
	calcTimeDiff = std::chrono::duration_cast<std::chrono::milliseconds>(t2Calculated - t1Calculated);
	ASSERT_NEAR(executionTime.count(), calcTimeDiff.count(), 2);

}

