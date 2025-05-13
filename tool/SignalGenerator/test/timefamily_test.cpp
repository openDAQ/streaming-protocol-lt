/*
 * Copyright 2022-2025 openDAQ d.o.o.
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


#include <nlohmann/json.hpp>
#include "../timefamily.h"


static const double epsilon = 0.000001;

TEST(siggen_test, test_Factorization)
{
	unsigned int inputFrequency;
	double resultFrequency;
	
	TimeFamily timeFamily;
	TimeFamily::PrimeFactorExponents primeFactorExponents;

	// decimal rates >=10Hz, only exponent factors of primes 2 and primes 5 are to be set
	// 1,10,100..1,000,000
	for (unsigned int i=1; i<7; ++i) {
		inputFrequency = static_cast < unsigned int > (pow(10, i));
		timeFamily.setFrequency(inputFrequency);
		primeFactorExponents = timeFamily.getPrimeFactorExponents();
		ASSERT_EQ(primeFactorExponents.size(), 3);
		ASSERT_GT(primeFactorExponents[0], 0);
		ASSERT_EQ(primeFactorExponents[1], 0);
		ASSERT_GT(primeFactorExponents[2], 0);
		resultFrequency = timeFamily.getFrequency();
		ASSERT_EQ(inputFrequency, resultFrequency);
	}

	// very intersting case!
	inputFrequency = 44100;
	timeFamily.setFrequency(inputFrequency);
	primeFactorExponents = timeFamily.getPrimeFactorExponents();
	ASSERT_EQ(primeFactorExponents, TimeFamily::TIME_FAMILY_44_1HZ);
	resultFrequency = timeFamily.getFrequency();
	ASSERT_EQ(inputFrequency, resultFrequency);

	// for binary rates, only exponent factors of prime 2 are to be set
	for (unsigned int i=1; i<32; ++i) {
		inputFrequency = static_cast < unsigned int > (pow(2, i));
		timeFamily.setFrequency(inputFrequency);
		primeFactorExponents = timeFamily.getPrimeFactorExponents();
		ASSERT_EQ(primeFactorExponents.size(), 1);
		ASSERT_GT(primeFactorExponents[0], 0);
		resultFrequency = timeFamily.getFrequency();
		ASSERT_EQ(inputFrequency, resultFrequency);
	}

	/// 0 is not valid for factorization
	inputFrequency = 0;
	timeFamily.setFrequency(inputFrequency);
	primeFactorExponents = timeFamily.getPrimeFactorExponents();
	ASSERT_EQ(primeFactorExponents.size(), 0);
}

TEST(siggen_test, test_freq_to_period)
{
	double inputFrequency;
	TimeFamily::PrimeFactorExponents primeFactorExponents;
	TimeFamily::PrimeFactorExponents invertedPrimeFactorExponents;
	TimeFamily timeFamilyFromFrequency;
	TimeFamily timeFamilyFromPeriod;
	double productFromFrequency;
	double productFromPeriod;

	inputFrequency = 2;
	timeFamilyFromFrequency.setFrequency(inputFrequency);
	timeFamilyFromPeriod.setPeriod(1/inputFrequency);
	productFromFrequency = timeFamilyFromFrequency.getFrequency();
	productFromPeriod = timeFamilyFromPeriod.getFrequency();
	ASSERT_NEAR(productFromFrequency, productFromPeriod, epsilon);


	inputFrequency = 1000;
	timeFamilyFromFrequency.setFrequency(inputFrequency);
	timeFamilyFromPeriod.setPeriod(1/inputFrequency);
	productFromFrequency = timeFamilyFromFrequency.getFrequency();
	productFromPeriod = timeFamilyFromPeriod.getFrequency();
	ASSERT_NEAR(productFromFrequency, productFromPeriod, epsilon);
}

TEST(siggen_test, test_composePrimeFactorExponents)
{
	double frequency;
	TimeFamily timeFamily;
	TimeFamily::PrimeFactorExponents primeFactorExponents;
	nlohmann::json json;
	int exponent;
	int exponentFromJson;


	json = timeFamily.composePrimeFactorExponentArray();
	ASSERT_TRUE(json.is_null());

	frequency = 1000;
	timeFamily.setFrequency(frequency);
	primeFactorExponents = timeFamily.getPrimeFactorExponents();
	json = timeFamily.composePrimeFactorExponentArray();
	ASSERT_TRUE(json.is_array());
	ASSERT_TRUE(json.size()==primeFactorExponents.size());
	for (unsigned int arrayIndex=0; arrayIndex<primeFactorExponents.size(); ++arrayIndex) {
		exponentFromJson = json[arrayIndex];
		exponent = primeFactorExponents[arrayIndex];
		ASSERT_TRUE(exponent == exponentFromJson);
	}
}

TEST(siggen_test, test_fromjson)
{
	nlohmann::json timeFamilyNode;
	TimeFamily timeFamily;
	double baseFrequency;
	double frequency;

	timeFamilyNode = R"([1])"_json;
	timeFamily.set(timeFamilyNode);
	baseFrequency = timeFamily.getBaseFrequency();
	frequency = timeFamily.getFrequency();
	ASSERT_NEAR(baseFrequency, 2.0, 0.0001);
	ASSERT_NEAR(baseFrequency, frequency, 0.0001);

	timeFamilyNode = R"([1, 0, 1])"_json;
	timeFamily.set(timeFamilyNode);
	baseFrequency = timeFamily.getBaseFrequency();
	frequency = timeFamily.getFrequency();
	ASSERT_NEAR(baseFrequency, 10.0, 0.0001);
	ASSERT_NEAR(baseFrequency, frequency, 0.0001);

	timeFamilyNode = R"([2, 2, 2, 2])"_json;
	timeFamily.set(timeFamilyNode);
	baseFrequency = timeFamily.getBaseFrequency();
	frequency = timeFamily.getFrequency();
	ASSERT_NEAR(baseFrequency, 44100.0, 0.0001);
	ASSERT_NEAR(baseFrequency, frequency, 0.0001);
}


TEST(siggen_test, test_expand)
{
	double frequency;
	double resultFrequency;

	TimeFamily timeFamily;
	// number is alreay suitable => factor = 1
	frequency = 10;
	timeFamily.setFrequency(frequency);
	ASSERT_EQ(timeFamily.getMultiplier(), 1);

	frequency = 1;
	timeFamily.setFrequency(frequency);
	ASSERT_EQ(timeFamily.getMultiplier(), 1);


	// whole numbers only => factor = 100
	frequency = 10.01;
	timeFamily.setFrequency(frequency);
	ASSERT_EQ(timeFamily.getMultiplier(), 100);

	// 0 is not valid at all. I can not be expanded
	frequency = 0;
	timeFamily.setFrequency(frequency);
	ASSERT_EQ(timeFamily.getMultiplier(), 0);

	// make it a whole number
	frequency = 1.5;
	timeFamily.setFrequency(frequency);
	ASSERT_EQ(timeFamily.getMultiplier(), 10);

	// make it a whole number > 1
	frequency = 0.5;
	timeFamily.setFrequency(frequency);
	ASSERT_EQ(timeFamily.getMultiplier(), 10);

	for (frequency = 0.02; frequency<1.0; frequency+=0.02) {
		timeFamily.setFrequency(frequency);
		resultFrequency = timeFamily.getFrequency();
		ASSERT_NEAR(resultFrequency, frequency, epsilon);
	}
}
