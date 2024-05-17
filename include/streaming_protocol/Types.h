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

#include <limits>
#include <vector>
#include <string>

#include "nlohmann/json.hpp"

#include "Defines.h"

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
    PostScaling()
    {
        clear();
    }

    /// set to "one to one"
    void clear()
    {
        offset = 0.0;
        scale = 1.0;
    }

    bool isOneToOne() const
    {
        return ((offset==0.0)&&(scale==1.0));
    }

    void compose(nlohmann::json& composition) const
    {
        if (isOneToOne()) {
            return;
        }
        composition[META_POSTSCALING][META_POFFSET] = offset;
        composition[META_POSTSCALING][META_SCALE] = scale;
    }

    void parse(const nlohmann::json& composition)
    {
        clear();
        auto postScaling = composition.find(META_POSTSCALING);
        if (postScaling!=composition.end()) {
            auto offsetIter = postScaling->find(META_POFFSET);
            if (offsetIter!=postScaling->end()) {
                offset = *offsetIter;
            }
            auto scaleIter = postScaling->find(META_SCALE);
            if (scaleIter!=postScaling->end()) {
                scale = *scaleIter;
            }
        }
    }

    double offset;
    double scale;

};

struct Range
{
    Range()
    {
        clear();
    }

    /// remove limits
    void clear()
    {
        low = -std::numeric_limits<double>::max();
        high = std::numeric_limits<double>::max();
    }

    bool isUnlimited() const
    {
        return (
                (low == -std::numeric_limits<double>::max())&&
                (high == std::numeric_limits<double>::max())
               );
    }

    void compose(nlohmann::json& composition) const
    {
        if (low!=-std::numeric_limits<double>::max()) {
            composition[META_RANGE][META_LOW] = low;
        }
        if (high!=std::numeric_limits<double>::max()) {
            composition[META_RANGE][META_HIGH] = high;
        }
    }

    void parse(const nlohmann::json& composition)
    {
        clear();
        auto rangeIter = composition.find(META_RANGE);
        if (rangeIter!=composition.end()) {
            auto lowIter = rangeIter->find(META_LOW);
            if (lowIter!=rangeIter->end()) {
                low = *lowIter;
            }
            auto highIter = rangeIter->find(META_HIGH);
            if (highIter!=rangeIter->end()) {
                high = *highIter;
            }
        }
    }

    double low;
    double high;
};

}
