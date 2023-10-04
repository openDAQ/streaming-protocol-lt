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



#include <gtest/gtest.h>

#include <cmath>

#include "../function.h"


TEST(siggen_test, test_constant)
{
        daq::siggen::FunctionParameters<double> functionParameters;
	functionParameters.amplitude = 1.0;
	functionParameters.offset = 1.0;
	functionParameters.frequency = 0.0;
	functionParameters.dutyCycle = 0.0;
        functionParameters.functionType = daq::siggen::FUNCTION_TYPE_CONSTANT;
	double value;
        daq::siggen::Function < double > function(functionParameters);
	value = function.calculate(100);
	ASSERT_NEAR(value, functionParameters.offset, 0.0001);
}

TEST(siggen_test, test_sine)
{
	static const double PI = 3.141592653589793;
	static const size_t stepCount = 100000;
        daq::siggen::FunctionParameters<double> functionParameters;
        functionParameters.functionType = daq::siggen::FUNCTION_TYPE_SINE;
	functionParameters.amplitude = 1.0;
	functionParameters.offset = 1.0;
	functionParameters.frequency = 1.0;
	functionParameters.dutyCycle = 0.0;

	double period = 1 / functionParameters.frequency;
	double timeStep = period / stepCount;
	double value;
	double valueExpected;
        daq::siggen::Function < double > function(functionParameters);
	for (double periodTime = 0.0; periodTime<period; periodTime+=timeStep) {
		value = function.calculate(periodTime);
		valueExpected = (sin(2*PI*functionParameters.frequency*periodTime)) * functionParameters.amplitude + functionParameters.offset;
		ASSERT_NEAR(value, valueExpected, 0.0001);
	}
}

TEST(siggen_test, test_square_double)
{
	// for the square function we test corner cases only
        daq::siggen::FunctionParameters<double> functionParameters;
        functionParameters.functionType = daq::siggen::FUNCTION_TYPE_RECTANGLE;
	functionParameters.amplitude = 100000.0;
	functionParameters.offset = -100.0;
	functionParameters.frequency = 1.0;
	functionParameters.dutyCycle = 0.5;
	double value;
        daq::siggen::Function < double > function(functionParameters);
	value = function.calculate(0.0);
	ASSERT_NEAR(value, functionParameters.offset+functionParameters.amplitude, 0.0001);
	value = function.calculate(0.5);
	ASSERT_NEAR(value, functionParameters.offset-functionParameters.amplitude, 0.0001);
	value = function.calculate(1.0);
	ASSERT_NEAR(value, functionParameters.offset+functionParameters.amplitude, 0.0001);
}

TEST(siggen_test, test_sawtooth_double)
{
        daq::siggen::FunctionParameters<double> functionParameters;
        functionParameters.functionType = daq::siggen::FUNCTION_TYPE_SAWTOOTH;
	functionParameters.amplitude = 100000.0;
	functionParameters.offset = -100.0;
	functionParameters.frequency = 1.0;
	functionParameters.dutyCycle = 0.0;

	double period = 1 / functionParameters.frequency;
	double timeStep = period / functionParameters.amplitude*10;
	double value;
        daq::siggen::Function < double > function(functionParameters);
	for (double periodTime = 0.0; periodTime<period; periodTime+=timeStep) {
		value = function.calculate(periodTime);
		ASSERT_NEAR(value, functionParameters.offset+periodTime*functionParameters.amplitude, 0.0001);
	}
}

TEST(siggen_test, test_sawtooth_int32)
{
        daq::siggen::FunctionParameters<int32_t> functionParameters;
        functionParameters.functionType = daq::siggen::FUNCTION_TYPE_SAWTOOTH;
	functionParameters.amplitude = 100000;
	functionParameters.offset = -100;
	functionParameters.frequency = 1.0;
	functionParameters.dutyCycle = 0.0;

	double period = 1 / functionParameters.frequency;
	double timeStep = period / functionParameters.amplitude;
	int32_t value;
	int32_t valueExpected;
        daq::siggen::Function < int32_t > function(functionParameters);
	for (double periodTime = 0.0; periodTime<period; periodTime+=timeStep) {
		value = function.calculate(periodTime);
		valueExpected = static_cast < int32_t >(std::round(functionParameters.offset+periodTime*functionParameters.amplitude));
		ASSERT_EQ(value, valueExpected);
	}
}
TEST(siggen_test, test_sawtooth_int64)
{
        daq::siggen::FunctionParameters<int64_t> functionParameters;
        functionParameters.functionType = daq::siggen::FUNCTION_TYPE_SAWTOOTH;
	functionParameters.amplitude = 100000;
	functionParameters.offset = -100;
	functionParameters.frequency = 1.0;
	functionParameters.dutyCycle = 0.0;

	double period = 1 / functionParameters.frequency;
	double timeStep = period / functionParameters.amplitude;
	int64_t value;
	int64_t valueExpected;

        daq::siggen::Function < int64_t > function(functionParameters);
	for (double periodTime = 0.0; periodTime<period; periodTime+=timeStep) {
		value = function.calculate(periodTime);
		valueExpected = static_cast < int64_t >(std::round(functionParameters.offset+periodTime*functionParameters.amplitude));
		ASSERT_EQ(value, valueExpected);
	}
}
