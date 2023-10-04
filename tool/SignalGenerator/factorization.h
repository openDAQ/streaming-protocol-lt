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

#pragma once

#include <map>
#include <nlohmann/json.hpp>

using PrimeFactorExponents = std::map < unsigned int, int >;

/// Factorization works for numbers greater 1 only. Furthermore only integers are allowed. Create a valid value.
/// Return the necessary factor to make number valid for factization.
/// \return factor number needs to expand with to make it factorizable. 0 if number is 0 which can not be expanded
unsigned int getFactorForFactorization(double number);

/// \param input 0 and 1 are invlid values
/// \return prime factor exponents. Empty on invalid input
PrimeFactorExponents getprimeFactorExponents(unsigned int input);

nlohmann::json composePrimeFactorExponents(const PrimeFactorExponents &primeFactorExponents);

/// \return 0 if primeFactorExponents empty
double getProduct(PrimeFactorExponents primeFactorExponents);

/// changes Sign of all prime factor exponents.
/// prime factor exponents of frequencies become prime factor exponents of the period and vice versa
PrimeFactorExponents changeSigns(PrimeFactorExponents input);
