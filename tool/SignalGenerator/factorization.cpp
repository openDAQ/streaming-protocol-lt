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

#include <cmath>
#include <cstdlib>
#include <iostream>

#include <map>

#include <nlohmann/json.hpp>

#include "factorization.h"

static const double epsilon = 0.000001;

unsigned int getFactorForFactorization(double number)
{
	if (fabs(number)<epsilon) {
		// 0 is not valid at all!
		return 0;
	}
	
	// shift left until there is no fractional part left
	double whole;
	double fraction;
	unsigned int multiplier = 1;
	while (fabs(fraction = modf(number, &whole))>epsilon) {
		multiplier *= 10;
		number *= 10;
	}

	if ((fabs(number)-1)<epsilon) {
		// 1 is forbidden double once more
		multiplier *= 2;
	}
	return multiplier;
}


PrimeFactorExponents getprimeFactorExponents(unsigned int input)
{
	PrimeFactorExponents primeFactorExponents;
	unsigned int i;

	for (i=2; i<=input; i++) {
		while (input%i == 0)	{
			input/=i;
			primeFactorExponents[i]++;
		}
	}
	return primeFactorExponents;
}

nlohmann::json composePrimeFactorExponents(const PrimeFactorExponents &primeFactorExponents)
{
	nlohmann::json json;
	for (const auto& iter : primeFactorExponents) {
		json[std::to_string(iter.first)] = iter.second;
	}
	return json;
}


double getProduct(PrimeFactorExponents primeFactorExponents)
{
	double product = 0.0;
	
	if (!primeFactorExponents.empty()) {
		product = 1.0;
		for (const auto& iter: primeFactorExponents) {
			unsigned int prime = iter.first;
			int exponent = iter.second;
			product *= pow(prime, exponent);
		}
	}
	return product;
}

PrimeFactorExponents changeSigns(PrimeFactorExponents input) {
	PrimeFactorExponents inversePrimeFactorExponents;
	for (const auto& iter: input) {
		unsigned int prime = iter.first;
		int inverseExponent = -(iter.second);
		inversePrimeFactorExponents[prime] = inverseExponent;
	}
	return inversePrimeFactorExponents;
}
