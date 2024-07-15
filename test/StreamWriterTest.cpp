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

#include <boost/asio/io_context.hpp>

#include <gtest/gtest.h>

#include "nlohmann/json.hpp"

#include "stream/FileStream.hpp"
#include "streaming_protocol/StreamWriter.h"
#include "streaming_protocol/Defines.h"

namespace daq::streaming_protocol {

    struct HeaderInfo {
        uint32_t header;
        uint32_t length;
        SignalNumber signalNumber;
        TransportType type;
    };

    static void readHeader(stream::Stream& inputStream, HeaderInfo &headerInfo) {
        boost::system::error_code ec;
        ec = inputStream.read( sizeof(headerInfo.header));
        inputStream.copyDataAndConsume(&headerInfo.header, sizeof(headerInfo.header));
        headerInfo.signalNumber = headerInfo.header & SIGNAL_NUMBER_MASK;
        headerInfo.type = static_cast < TransportType > ((headerInfo.header & TYPE_MASK) >> TYPE_SHIFT);
        headerInfo.length = (headerInfo.header & SIZE_MASK) >> SIZE_SHIFT;
        if (headerInfo.length == 0) {
            // length is to be found in additional length field
            ec = inputStream.read( sizeof(headerInfo.length));
            inputStream.copyDataAndConsume(&headerInfo.length, sizeof(headerInfo.length));
        }
    }

    TEST(StreamWriterTest, test)
    {
        static const std::string fileName = "theFile";
        static const unsigned int signalNumber = 7;
        nlohmann::json smallMetaInfo;
        nlohmann::json bigMetaInfo;

        smallMetaInfo["bla"]["blub"] = 12;
        for (size_t index=0; index<256; ++index) {
            bigMetaInfo["bla"].push_back(index);
        }

        double smallData[] = { 1.1, 2.2, 3.3};
        std::vector <double> bigData;
        for (size_t index=0; index<256; ++index) {
            bigData.push_back(index*0.5);
        }

        boost::asio::io_context ioc;
        {
            auto fileStream = std::make_shared<daq::stream::FileStream>(ioc, fileName, true);
            fileStream->init();
            StreamWriter streamWriter(fileStream);
            ASSERT_EQ(streamWriter.id(), fileName);
            streamWriter.writeMetaInformation(signalNumber, smallMetaInfo);
            streamWriter.writeMetaInformation(signalNumber, bigMetaInfo);

            streamWriter.writeSignalData(signalNumber, smallData, sizeof(smallData));
            streamWriter.writeSignalData(signalNumber, bigData.data(), bigData.size()*sizeof(bigData[0]));
        }

        {
            // now read produced stuff and check...
            HeaderInfo headerInfo;
            nlohmann::json jsonContent;

            daq::stream::FileStream fileStream(ioc, fileName);
            fileStream.init();

            readHeader(fileStream, headerInfo);
            ASSERT_EQ(headerInfo.signalNumber, signalNumber);
            ASSERT_EQ(headerInfo.type, TYPE_METAINFORMATION);
            fileStream.consume(headerInfo.length); // consume payload
            readHeader(fileStream, headerInfo);
            ASSERT_EQ(headerInfo.signalNumber, signalNumber);
            ASSERT_EQ(headerInfo.type, TYPE_METAINFORMATION);
            fileStream.consume(headerInfo.length); // consume payload
            readHeader(fileStream, headerInfo);
            ASSERT_EQ(headerInfo.length, sizeof(smallData));
            ASSERT_EQ(headerInfo.signalNumber, signalNumber);
            ASSERT_EQ(headerInfo.type, TYPE_SIGNALDATA);
            fileStream.consume(headerInfo.length); // consume payload
            readHeader(fileStream, headerInfo);
            ASSERT_EQ(headerInfo.length, bigData.size()*sizeof(bigData[0]));
            ASSERT_EQ(headerInfo.signalNumber, signalNumber);
            ASSERT_EQ(headerInfo.type, TYPE_SIGNALDATA);
            fileStream.consume(headerInfo.length); // consume payload
        }
    }

    TEST(StreamWriterTest, size_limit)
    {
        static const std::string fileName = "theFile";
        unsigned int signalNumber = 7;


        std::vector < uint8_t > onLimitData(255); // the maximum being expresse in compact header size
        std::vector < uint8_t > overLimitData(256); // has to be put into the additional length
        for(unsigned int index=0; index<onLimitData.size(); ++index) {
            onLimitData[index] = index;
        }

        for(unsigned int index=0; index<overLimitData.size(); ++index) {
            overLimitData[index] = index;
        }

        boost::asio::io_context ioc;
        {
            auto fileStream = std::make_shared<daq::stream::FileStream>(ioc, fileName, true);
            fileStream->init();
            StreamWriter streamWriter(fileStream);
            ASSERT_EQ(streamWriter.id(), fileName);
            streamWriter.writeSignalData(signalNumber, onLimitData.data(), onLimitData.size());
            streamWriter.writeSignalData(signalNumber, overLimitData.data(), overLimitData.size());
        }

        {
            // now read produced stuff and check...
            HeaderInfo headerInfo;
            daq::stream::FileStream fileStream(ioc, fileName);
            fileStream.init();

            readHeader(fileStream, headerInfo);
            ASSERT_EQ(headerInfo.length, onLimitData.size());
            ASSERT_EQ(headerInfo.signalNumber, signalNumber);
            ASSERT_EQ(headerInfo.type, TYPE_SIGNALDATA);
            fileStream.consume(headerInfo.length); // consume payload
            readHeader(fileStream, headerInfo);
            ASSERT_EQ(headerInfo.length, overLimitData.size());
            ASSERT_EQ(headerInfo.signalNumber, signalNumber);
            ASSERT_EQ(headerInfo.type, TYPE_SIGNALDATA);
            fileStream.consume(headerInfo.length); // consume payload
        }
    }
}
