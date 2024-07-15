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

#include <gtest/gtest.h>

#include <nlohmann/json.hpp>
#include "../factorization.h"


static const double epsilon = 0.000001;

TEST(siggen_test, test_Factorization)
{
	unsigned int inputFrequency;
	double resultFrequency;
	PrimeFactorExponents primeFactorExponents;

	// decimal rates, only exponent factors of primes 2 and primes 5 are to be set
	// 10,100..1,000,000
	for (unsigned int i=1; i<7; ++i) {
		inputFrequency = static_cast < unsigned int > (pow(10, i));
		primeFactorExponents = getprimeFactorExponents(inputFrequency);
		ASSERT_EQ(primeFactorExponents.size(), 2);
		ASSERT_GT(primeFactorExponents[2], 0);
		ASSERT_GT(primeFactorExponents[5], 0);
		resultFrequency = getProduct(primeFactorExponents);
		ASSERT_EQ(inputFrequency, resultFrequency);
	}

	// very intersting case!
	inputFrequency = 	44100;
	primeFactorExponents = getprimeFactorExponents(inputFrequency);
	ASSERT_EQ(primeFactorExponents.size(), 4);
	ASSERT_EQ(primeFactorExponents[2], 2);
	ASSERT_EQ(primeFactorExponents[3], 2);
	ASSERT_EQ(primeFactorExponents[5], 2);
	ASSERT_EQ(primeFactorExponents[7], 2);
	resultFrequency = getProduct(primeFactorExponents);
	ASSERT_EQ(inputFrequency, resultFrequency);

	// for binary rates, only exponent factors of prime 2 are to be set
	for (unsigned int i=1; i<32; ++i) {
		inputFrequency = static_cast < unsigned int > (pow(2, i));
		primeFactorExponents = getprimeFactorExponents(inputFrequency);
		ASSERT_EQ(primeFactorExponents.size(), 1);
		ASSERT_GT(primeFactorExponents[2], 0);
		resultFrequency = getProduct(primeFactorExponents);
		ASSERT_EQ(inputFrequency, resultFrequency);
	}

	/// 0 is not valid for factorization
	inputFrequency = 0;
	primeFactorExponents = getprimeFactorExponents(inputFrequency);
	ASSERT_EQ(primeFactorExponents.size(), 0);
	
	/// 1 is not valid for factorization
	inputFrequency = 1;
	primeFactorExponents = getprimeFactorExponents(inputFrequency);
	ASSERT_EQ(primeFactorExponents.size(), 0);
	
}

TEST(siggen_test, test_freq_to_period)
{
	unsigned int inputFrequency;
	double resultPeriod;
	PrimeFactorExponents primeFactorExponents;
	PrimeFactorExponents invertedPrimeFactorExponents;

	inputFrequency = 2;
	primeFactorExponents = getprimeFactorExponents(inputFrequency);
	invertedPrimeFactorExponents = changeSigns(primeFactorExponents);
	resultPeriod = getProduct(invertedPrimeFactorExponents);
	ASSERT_NEAR(inputFrequency, 1/resultPeriod, epsilon);

	inputFrequency = 1000;
	primeFactorExponents = getprimeFactorExponents(inputFrequency);
	invertedPrimeFactorExponents = changeSigns(primeFactorExponents);
	resultPeriod = getProduct(invertedPrimeFactorExponents);
	ASSERT_NEAR(inputFrequency, 1/resultPeriod, epsilon);

}

TEST(siggen_test, test_composePrimeFactorExponents)
{
	unsigned int inputFrequency;
	int exponent;
	int exponentFromJson;
	PrimeFactorExponents primeFactorExponents;
	nlohmann::json json;
	json = composePrimeFactorExponents(primeFactorExponents);
	ASSERT_TRUE(json.is_null());
	inputFrequency = 1000;
	primeFactorExponents = getprimeFactorExponents(inputFrequency);
	json = composePrimeFactorExponents(primeFactorExponents);
	ASSERT_TRUE(json.is_object());
	ASSERT_TRUE(json.size()==primeFactorExponents.size());
	static const int zero = 0;
	for (const auto& iter : primeFactorExponents) {
		exponentFromJson = json.value(std::to_string(iter.first), zero);
		exponent = iter.second;
		ASSERT_TRUE(exponent == exponentFromJson);
	}
}


TEST(siggen_test, test_expand)
{
	double number;
	unsigned int factor;

	// number is alreay suitable => factor = 1
	number = 10;
	factor = getFactorForFactorization(number);
	ASSERT_EQ(factor, 1);

	// 1 can not be factorized smallest possible number is 2
	number = 1;
	factor = getFactorForFactorization(number);
	ASSERT_EQ(factor, 2);

	// whole numbers only => factor = 100
	number = 10.01;
	factor = getFactorForFactorization(number);
	ASSERT_EQ(factor, 100);

	// 0 is not valid at all. I can not be expanded
	number = 0;
	factor = getFactorForFactorization(number);
	ASSERT_EQ(factor, 0);

	// make it a whole number
	number = 1.5;
	factor = getFactorForFactorization(number);
	ASSERT_EQ(factor, 10);

	// make it a whole number > 1
	number = 0.5;
	factor = getFactorForFactorization(number);
	ASSERT_EQ(factor, 10);
	
	PrimeFactorExponents primeFactorExponents;
	unsigned int toBeFactorized;
	double product;
	double numberResult;
	// make it a whole number > 1
	for (number = 0.01; number<1; number+=0.01) {
		factor = getFactorForFactorization(number);
		toBeFactorized = static_cast < unsigned int > (std::round(number *factor));
		primeFactorExponents = getprimeFactorExponents(toBeFactorized);
		product = getProduct(primeFactorExponents);
		numberResult = product / factor;
		ASSERT_NEAR(number, numberResult, epsilon);
	}
}
