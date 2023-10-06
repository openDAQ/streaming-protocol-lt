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

#include <boost/asio/io_context.hpp>

#include <gtest/gtest.h>

#include "nlohmann/json.hpp"

#include "streaming_protocol/Defines.h"
#include "streaming_protocol/StreamMeta.hpp"

#include "streaming_protocol/Logging.hpp"

namespace daq::streaming_protocol {
    static LogCallback logCallback = daq::streaming_protocol::Logging::logCallback();

    static MetaInformation creataMetaInformation(const std::vector < uint8_t >& payload)
    {
        std::vector < uint8_t > package;
        package.resize(sizeof(METAINFORMATION_MSGPACK) + payload.size());
        memcpy(&package[0], &METAINFORMATION_MSGPACK, sizeof(METAINFORMATION_MSGPACK));
        memcpy(&package[sizeof(METAINFORMATION_MSGPACK)], &payload[0], payload.size());
        MetaInformation metaInformation(logCallback);
        metaInformation.interpret(&package[0], package.size());
        return metaInformation;
    }

    TEST(StreamMetaTest, empty_test)
    {
        int result;
        StreamMeta streamMeta(logCallback);
        MetaInformation metaInformation(logCallback);

        result = streamMeta.processMetaInformation(metaInformation, "testStream");
        // empty is ignored
        ASSERT_EQ(result, 0);
    }

    TEST(StreamMetaTest, broken_msgpack_test)
    {
        int result;
        StreamMeta streamMeta(logCallback);

        std::vector < uint8_t > payload(10);
        MetaInformation metaInformation = creataMetaInformation(payload);


        result = streamMeta.processMetaInformation(metaInformation, "testStream");
        // broken meta information is ignored
        ASSERT_EQ(result, 0);
    }

    TEST(StreamMetaTest, invalid_type_test)
    {
        int result;
        static const uint32_t unknownType = 42;
        StreamMeta streamMeta(logCallback);

        std::vector < uint8_t > payload(10);
        std::vector < uint8_t > package;
        package.resize(sizeof(unknownType) + payload.size());
        memcpy(&package[0], &unknownType, sizeof(unknownType));
        memcpy(&package[sizeof(unknownType)], &payload[0], payload.size());
        MetaInformation metaInformation(logCallback);
        metaInformation.interpret(&package[0], package.size());
        result = streamMeta.processMetaInformation(metaInformation, "testStream");
        // meta information of unknown type is ignored
        ASSERT_EQ(result, 0);
    }


    TEST(StreamMetaTest, unknown_meta_test)
    {
        nlohmann::json metaInformationDoc = R"(
        {
            "method" : "unknown",
            "params" : {}
        }
        )"_json;

        int result;
        StreamMeta streamMeta(logCallback);
        std::vector < uint8_t > payload = nlohmann::json::to_msgpack(metaInformationDoc);
        MetaInformation metaInformation = creataMetaInformation(payload);

        result = streamMeta.processMetaInformation(metaInformation, "testStream");
        // unknown meta information is ignored
        ASSERT_EQ(result, 0);
    }



    TEST(StreamMetaTest, version_test)
    {
        nlohmann::json metaInformationDoc = R"(
        {
            "method" : "apiVersion",
            "params" : {}
        }
        )"_json;

        int result;


        {
            // missing version informaion is fatal!

            std::vector < uint8_t > payload = nlohmann::json::to_msgpack(metaInformationDoc);
            MetaInformation metaInformation = creataMetaInformation(payload);

            StreamMeta streamMeta(logCallback);
            result = streamMeta.processMetaInformation(metaInformation, "testStream");
            ASSERT_EQ(result, -1);
        }

        {
            // valid version
            std::string requestedVersion = "0.6.0";

            metaInformationDoc[PARAMS][VERSION] = requestedVersion;

            std::vector < uint8_t > payload = nlohmann::json::to_msgpack(metaInformationDoc);
            MetaInformation metaInformation = creataMetaInformation(payload);

            StreamMeta streamMeta(logCallback);
            result = streamMeta.processMetaInformation(metaInformation, "testStream");
            ASSERT_EQ(result, 0);
            ASSERT_EQ(streamMeta.apiVersion(), requestedVersion);
        }

        {
            // invalid format
            std::string requestedVersion = "0.6";

            metaInformationDoc[PARAMS][VERSION] = requestedVersion;

            std::vector < uint8_t > payload = nlohmann::json::to_msgpack(metaInformationDoc);
            MetaInformation metaInformation = creataMetaInformation(payload);

            StreamMeta streamMeta(logCallback);
            result = streamMeta.processMetaInformation(metaInformation, "testStream");
            ASSERT_EQ(result, -1);
            ASSERT_EQ(streamMeta.apiVersion(), requestedVersion);
        }

        {
            // version too small
            std::string requestedVersion = "0.5.0";

            metaInformationDoc[PARAMS][VERSION] = requestedVersion;

            std::vector < uint8_t > payload = nlohmann::json::to_msgpack(metaInformationDoc);
            MetaInformation metaInformation = creataMetaInformation(payload);

            StreamMeta streamMeta(logCallback);
            result = streamMeta.processMetaInformation(metaInformation, "testStream");
            ASSERT_EQ(result, -1);
            ASSERT_EQ(streamMeta.apiVersion(), requestedVersion);
        }
    }

    TEST(StreamMetaTest, init_test)
    {
        nlohmann::json metaInformationDoc = R"(
        {
            "method" : "init",
            "params" : {}
        }
        )"_json;

        int result;
        {
            // stream id
            std::string streamId = "theId";
            metaInformationDoc[PARAMS][META_STREAMID] = streamId;

            std::vector < uint8_t > payload = nlohmann::json::to_msgpack(metaInformationDoc);
            MetaInformation metaInformation = creataMetaInformation(payload);

            StreamMeta streamMeta(logCallback);
            result = streamMeta.processMetaInformation(metaInformation, "testStream");
            ASSERT_EQ(result, 0);
            ASSERT_EQ(streamMeta.streamId(), streamId);
        }

        {
            // command interfaces
            std::string httpVersion = "1.1";

            std::string httpControlPort = "8080"; // might also be by name like "http"
            std::string httpControlPath = "/path";

            metaInformationDoc[PARAMS]["commandInterfaces"]["jsonrpc-http"]["httpMethod"] = "POST";
            metaInformationDoc[PARAMS]["commandInterfaces"]["jsonrpc-http"]["httpVersion"] = httpVersion;
            metaInformationDoc[PARAMS]["commandInterfaces"]["jsonrpc-http"]["port"] = httpControlPort;
            metaInformationDoc[PARAMS]["commandInterfaces"]["jsonrpc-http"]["httpPath"] = httpControlPath;
            std::vector < uint8_t > payload = nlohmann::json::to_msgpack(metaInformationDoc);
            MetaInformation metaInformation = creataMetaInformation(payload);

            StreamMeta streamMeta(logCallback);
            result = streamMeta.processMetaInformation(metaInformation, "testStream");
            ASSERT_EQ(result, 0);

            httpVersion.erase(std::remove(httpVersion.begin(), httpVersion.end(), '.'), httpVersion.end());
            unsigned int httpVersionAsNumber = std::stoi(httpVersion);


            ASSERT_EQ(streamMeta.httpVersion(), httpVersionAsNumber);
            ASSERT_EQ(streamMeta.httpControlPort(), httpControlPort);
            ASSERT_EQ(streamMeta.httpControlPath(), httpControlPath);
        }
    }

    TEST(StreamMetaTest, alive_test)
    {
        nlohmann::json metaInformationDoc = R"(
        {
            "method" : "alive",
            "params" : {}
        }
        )"_json;

        int result;
        {
            // no fill level
            std::vector < uint8_t > payload = nlohmann::json::to_msgpack(metaInformationDoc);
            MetaInformation metaInformation = creataMetaInformation(payload);

            StreamMeta streamMeta(logCallback);
            result = streamMeta.processMetaInformation(metaInformation, "testStream");
            ASSERT_EQ(result, 0);
        }

        {
            // fill level set
            metaInformationDoc[PARAMS][META_FILLLEVEL] = 20;
            std::vector < uint8_t > payload = nlohmann::json::to_msgpack(metaInformationDoc);
            MetaInformation metaInformation = creataMetaInformation(payload);

            StreamMeta streamMeta(logCallback);
            result = streamMeta.processMetaInformation(metaInformation, "testStream");
            ASSERT_EQ(result, 0);
        }
    }
}
