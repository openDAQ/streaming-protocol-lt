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


#include <string>

#include <gtest/gtest.h>

#include <nlohmann/json.hpp>

#include "../signaldefinition.h"
#include "../signalgenerator.h"
#include "../stdoutwriter.h"


TEST(siggen_test, test_addsignals_from_config)
{
	nlohmann::json signalDefinition = 
R"(
	{	"signals": 
		{
			"thesignal" :
			{
				"function" : "rectangle",
				"amplitude" : 1.0,
				"offset" : 1.0,
				"frequency" : 100,
				"dutyCycle" : 0.5,
				"samplePeriod" : "100µs",
				"delay": "0"
			}
		}
	}
)"_json;

        daq::siggen::StdoutWriter writer;

        daq::siggen::SignalGenerator signalGenerator(std::chrono::system_clock::now(), writer);
	ASSERT_EQ(signalGenerator.signalCount(), 0);
	signalGenerator.configure(signalDefinition);
	ASSERT_EQ(signalGenerator.signalCount(), 1);
}

TEST(siggen_test, test_durationFromString)
{
	std::chrono::nanoseconds duration;
        duration = daq::siggen::durationFromString("1009 s", std::chrono::seconds::zero());
	ASSERT_EQ(std::chrono::seconds(1009), duration);
        duration = daq::siggen::durationFromString("107 ms", std::chrono::seconds::zero());
	ASSERT_EQ(std::chrono::milliseconds(107), duration);
        duration = daq::siggen::durationFromString("1µs", std::chrono::seconds::zero());
	ASSERT_EQ(std::chrono::microseconds(1), duration);
        duration = daq::siggen::durationFromString("1000 ns", std::chrono::seconds::zero());
	ASSERT_EQ(std::chrono::microseconds(1), duration);
}

