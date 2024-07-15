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
#include <cstring>
#include <iostream>

#include <boost/asio/io_context.hpp>

#include "stream/TcpServer.hpp"

#include "signalgenerator.h"
#include "worker.h"
#include "defines.h"

void AcceptCb(const std::string& configFile, std::shared_ptr < daq::stream::Stream > newStream)
{
    std::cout << "accepted client..." << std::endl;
    auto newWorker = std::make_shared < Worker > (newStream);
    newWorker->start(configFile);
}

int main(int argc, char* argv[])
{
	uint16_t port;
	std::string configFile;

	if (argc > 1) {
		port = static_cast < uint16_t > (std::strtoul(argv[1], nullptr, 10));
	} else {
        port = daq::streaming_protocol::siggen::StreamingPort;
	}

	if (argc > 2) {
		configFile = argv[2];
		std::cout << "using configuration file '" << configFile << "'" << std::endl;
	} else {
		std::cout << "using default configuration" << std::endl;
	}

    boost::asio::io_context ioContext;
    daq::stream::TcpServer newTcpServer(ioContext, std::bind(AcceptCb, configFile, std::placeholders::_1), port);
    newTcpServer.start();
    std::cout << "Listening to port " << port << std::endl;

    ioContext.run();
	return EXIT_SUCCESS;
}	
