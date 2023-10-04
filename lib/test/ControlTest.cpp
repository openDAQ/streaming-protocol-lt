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

#include "boost/asio/io_context.hpp"

#include "streaming_protocol/Types.h"

#include "streaming_protocol/Logging.hpp"

#include "../src/Controller.hpp"

namespace daq::streaming_protocol {
    static const std::string streamId = "theId";;
    static const std::string signalId = "a signal id";
    static const std::string address = "localhost";
    static const unsigned int port = 6000;
    static const std::string target = "/";
    static const unsigned int httpVersion = 11;
    static boost::asio::io_context ioc;
    static LogCallback logCallback = daq::streaming_protocol::Logging::logCallback();

    TEST(ControlTest, empty_stream_id)
    {
        // excepction because of empty stream id
        EXPECT_THROW(Controller controller(ioc, "", address, std::to_string(port), target, httpVersion, logCallback), std::runtime_error);
    }

//    TEST(ControlTest, empty_signal_id_list_subscribe_unsubscribe)
//    {
//        Controller controller(ioc, streamId, address, std::to_string(port), target, httpVersion);
//        SignalIds signalIds;

//        boost::system::error_code ec;
//        unsigned int count = 0;
//        auto resultCb = [&](const boost::system::error_code& cbEc) {
//            ec = cbEc;
//            ++count;
//        };
//        controller.asyncSubscribe(signalIds, resultCb);
//        controller.asyncUnsubscribe(signalIds, resultCb);

//        ioc.run();
//        ASSERT_EQ(count, 2);
//    }

    TEST(ControlTest, signal_id_list_subscribe_unsubscribe)
    {
        Controller controller(ioc, streamId, address, std::to_string(port), target, httpVersion, logCallback);
        SignalIds signalIds;

        boost::system::error_code ec;
        unsigned int count = 0;
        auto resultCb = [&](const boost::system::error_code& cbEc) {
            ec = cbEc;
            ++count;
        };

        signalIds.push_back(signalId);
        controller.asyncSubscribe(signalIds, resultCb);
        controller.asyncUnsubscribe(signalIds, resultCb);

        ioc.run();
        ASSERT_EQ(count, 2);
    }


}
