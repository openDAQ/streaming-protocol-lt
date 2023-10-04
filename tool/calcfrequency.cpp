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

#include <cstdlib>
#include <ctime>
#include <iomanip>
#include <iostream>
#include <string>

#include "streaming_protocol/Timefamily.hpp"


/* Calculates the frequency from given prime factor exponents */
int main(int argc, char* argv[])
{
	if (argc==1) {
		std::cout << "expects prime factor exponents as a parameters" << std::endl;
		return EXIT_SUCCESS;
	}

	TimeFamily::PrimeFactorExponents primeFactorExponents;
	unsigned int primeFactorExponentCount = argc-1;
	for (unsigned int primeFactorExponentIndex=0; primeFactorExponentIndex<primeFactorExponentCount; ++primeFactorExponentIndex) {
		primeFactorExponents.push_back(std::stoul(argv[primeFactorExponentIndex+1]));
	}

	TimeFamily tf;
	tf.set(primeFactorExponents);
	uint64_t baseFrequency = tf.getBaseFrequency();
	uint64_t range = tf.getRange();
	std::cout << "base frequency: f = " << baseFrequency << "Hz" << std::endl;
	std::cout << "resolution: t_res = " << 1/baseFrequency << "s" << std::endl;
	std::cout << "range is " << range << "s" << std::endl;
	
	if (range > std::numeric_limits < uint64_t >::max()) {
		std::cout << "range exceeds range of std::time_t" << std::endl;
	} else {
		// possible absolute time when using UNIX epoch
		timespec ts;
		ts.tv_sec = range;
		ts.tv_nsec = 0;
		
		std::cout << "When using the UNIX EPOCH (1.1.1970) the abolute time ranges up to (UTC):" << std::endl;
		
		struct tm *gmtime = std::gmtime(&ts.tv_sec);
				
		std::cout << std::put_time(gmtime, "%c %Z") << '\n';
	}
		
	return EXIT_SUCCESS;
}
