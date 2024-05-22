/*
 * Copyright 2022-2024 Blueberry d.o.o.
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

#include <limits>

#include "streaming_protocol/Defines.h"
#include "streaming_protocol/Types.h"

namespace daq::streaming_protocol {

Range::Range()
{
    clear();
}

bool Range::operator==(const Range &other) const
{
    return (
            (std::abs(low - other.low) < epsilon) &&
            (std::abs(high - other.high) < epsilon)
           );
}

void Range::clear()
{
    low = -std::numeric_limits<double>::max();
    high = std::numeric_limits<double>::max();
}

bool Range::isUnlimited() const
{
    return (
            (low == -std::numeric_limits<double>::max())&&
            (high == std::numeric_limits<double>::max())
           );
}

void Range::compose(nlohmann::json &composition) const
{
    if (low!=-std::numeric_limits<double>::max()) {
        composition[META_RANGE][META_LOW] = low;
    }
    if (high!=std::numeric_limits<double>::max()) {
        composition[META_RANGE][META_HIGH] = high;
    }
}

void Range::parse(const nlohmann::json &composition)
{
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

PostScaling::PostScaling()
{
    clear();
}

bool PostScaling::operator==(const PostScaling &other) const
{
    return (
            (std::abs(offset - other.offset) < epsilon) &&
            (std::abs(scale - other.scale) < epsilon)
           );
}

void PostScaling::clear()
{
    offset = 0.0;
    scale = 1.0;
}

bool PostScaling::isOneToOne() const
{
    return ((offset==0.0)&&(scale==1.0));
}

void PostScaling::compose(nlohmann::json &composition) const
{
    if (isOneToOne()) {
        return;
    }
    composition[META_POSTSCALING][META_POFFSET] = offset;
    composition[META_POSTSCALING][META_SCALE] = scale;
}

void PostScaling::parse(const nlohmann::json &composition)
{
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
};
