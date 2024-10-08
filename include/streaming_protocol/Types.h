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


#pragma once

#include <vector>
#include <string>

#include "nlohmann/json.hpp"

#include "streaming_protocol/Logging.hpp"

namespace daq::streaming_protocol {
/// Each signal has a id as string. It is unique for each producer.
using SignalIds = std::vector < std::string >;

/// On transport layer, each signal is identified by its signal number
using SignalNumber = unsigned int;

/// transport layer
enum TransportType {
    TYPE_SIGNALDATA = 1,
    TYPE_METAINFORMATION = 2,
};

enum SampleType {
    SAMPLETYPE_UNKNOWN,

    SAMPLETYPE_U8,
    SAMPLETYPE_S8,
    SAMPLETYPE_U16,
    SAMPLETYPE_S16,
    SAMPLETYPE_U32,
    SAMPLETYPE_S32,
    SAMPLETYPE_U64,
    SAMPLETYPE_S64,
    SAMPLETYPE_REAL32,
    SAMPLETYPE_REAL64,

    SAMPLETYPE_BITFIELD32,
    SAMPLETYPE_BITFIELD64,

    SAMPLETYPE_COMPLEX32,
    SAMPLETYPE_COMPLEX64,

    /// HBK streaming only!
    SAMPLETYPE_ARRAY,
    SAMPLETYPE_STRUCT
};

struct Complex32Type {
    float real;
    float imag;
};

struct Complex64Type {
    double real;
    double imag;
};

enum RuleType {
    RULETYPE_UNKNOWN,
    RULETYPE_EXPLICIT, /// Time rule of asynchronous signals.
    RULETYPE_CONSTANT,
    RULETYPE_LINEAR, /// Time rule of synchronous signals delivering values that are equidistant in time.
};

struct PostScaling
{
    /// default to being "one to one"
    PostScaling();

    bool operator==(const PostScaling& other) const;

    /// set to "one to one"
    void clear();

    bool isOneToOne() const;

    void compose(nlohmann::json& composition) const;

    /// missing parameters are kept as is
    void parse(const nlohmann::json& composition);

    double offset;
    double scale;
};

struct Range
{
    /// On construction limits are unlimited
    Range();

    bool operator==(const Range& other) const;

    /// remove limits
    void clear();

    bool isUnlimited() const;

    /// unlimited limits are not composed
    void compose(nlohmann::json& composition) const;

    /// missing limits are kept as is
    void parse(const nlohmann::json& composition);

    double low;
    double high;
};

class Resolution {
public:
    Resolution();
    Resolution(uint64_t numerator, uint64_t denominator);
    uint64_t numerator;
    uint64_t denominator;

    void compose(nlohmann::json& composition) const;

    /// \return -1 on error 1 on change of value
    int parse(const nlohmann::json& composition, LogCallback logCallback);

    std::string toString() const;
};

/// signals following an implicit rule produce indexed values that consist of the signal value and a value index
template<class Type>
struct IndexedValue
{
    uint64_t index;
    Type value;
};

/// Signal type is the key signal id is the value.
/// Currently types "time" and "status" are specified
using RelatedSignals = std::map < std::string, std::string>;


}
