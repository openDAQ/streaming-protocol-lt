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

#include <memory>
#include <thread>

#include "stream/Stream.hpp"

#include "signalgenerator.h"

#pragma once

/// Is constructed as a shared pointer to control lifecycle.
class Worker : public std::enable_shared_from_this<Worker>
{
public:
    using CompletionCb = std::function < void () >;
    Worker(std::shared_ptr < daq::stream::Stream > stream);
    ~Worker();
    Worker (Worker&&) = delete;
    Worker operator= (Worker&) = delete;

    void start(const std::string& configFile);
private:
    void threadFunction(const std::string& configFileName);

    daq::streaming_protocol::siggen::SignalGenerator m_signalGenerator;
    std::thread m_thread;
    daq::streaming_protocol::LogCallback logCallback;
};
