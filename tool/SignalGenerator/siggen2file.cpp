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

#include <chrono>
#include <cstdlib>

#include "boost/asio/io_context.hpp"

#include "stream/FileStream.hpp"

#include "signaldefinition.h"
#include "signalgenerator.h"


int main(int argc, char* argv[])
{
    static const char targetFile[] = "output.stream";
    std::chrono::system_clock::time_point startTime = std::chrono::system_clock::now();
    boost::asio::io_context ioc;
    auto fileStream = std::make_shared<daq::stream::FileStream>(ioc, targetFile, true);
    boost::system::error_code ec = fileStream->init();
    if (ec) {
        std::cerr << "Could not opne file for writing: " << ec.message() << std::endl;
        return EXIT_FAILURE;
    }
    daq::streaming_protocol::siggen::SignalGenerator signalGenerator(startTime, fileStream);

    if (argc>=2) {
        if (signalGenerator.configureFromFile(argv[1])) {
            std::cerr << "could not configure signals from file!" << std::endl;
            return EXIT_FAILURE;
        }
    } else {
        static const std::chrono::seconds duration(10);
        static const std::chrono::microseconds processPeriod(10);
        daq::streaming_protocol::siggen::addSignals(signalGenerator);
        signalGenerator.configureTimes(processPeriod, duration);
    }
    signalGenerator.start(false);
    return EXIT_SUCCESS;
}	
