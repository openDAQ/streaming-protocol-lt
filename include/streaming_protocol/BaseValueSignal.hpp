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

#pragma once

#include <chrono>
#include <string>
#include <mutex>

#include "streaming_protocol/BaseSignal.hpp"
#include "streaming_protocol/Defines.h"
#include "streaming_protocol/iWriter.hpp"
#include "streaming_protocol/Types.h"
#include "streaming_protocol/Logging.hpp"
#include "streaming_protocol/Unit.hpp"

/// \warning Since only C++ libraries under linux allow resolution down to nanoseconds, resolution is limited to microseconds
//#define TIME_GRANULARITY_NS

namespace daq::streaming_protocol{
    /// \addtogroup producer
    /// Abstrace base class for producing signal data
    class BaseValueSignal : public BaseSignal {
    public:
        /// \param writer The communication stream or session used to write the data to. Several signals can be added to it.
        BaseValueSignal(const std::string& signalId, const std::string& tableId, iWriter& writer, LogCallback logCb);
        /// not to be copied!
        BaseValueSignal(const BaseValueSignal&) = delete;
        
        virtual ~BaseValueSignal() = default;

        virtual bool isDataSignal() const final
        {
            return true;
        }

        virtual SampleType getSampleType() const = 0;

        /// \param name Name of the root data member
        void setMemberName(const std::string& name);
        /// \return Name of the root data member
        std::string getMemberName() const;

        Unit getUnit() const;
        int32_t getUnitId() const;
        std::string getUnitDisplayName() const;

        //// \param unitId Unit::UNIT_ID_NONE for no unit
        void setUnit(int32_t unitId, const std::string& displayName);
        void setUnit(const Unit& value);

        void setPostScaling(const PostScaling postScaling)
        {
            m_postScaling = postScaling;
        }

        void clearPostScaling()
        {
            m_postScaling.clear();
        }

        void setRange(const Range& range)
        {
            m_range = range;
        }

        void clearRange()
        {
            m_range.clear();
        }

        void setRelatedSignals(const RelatedSignals& relatedSignals)
        {
            m_relatedSignals = relatedSignals;
        }

    protected:

        void composeRelatedSignals(nlohmann::json& composition) const
        {
            for(const auto& iter : m_relatedSignals) {
                 nlohmann::json relatedSignal;
                 relatedSignal[META_TYPE] = iter.first;
                 relatedSignal[META_SIGNALID] = iter.second;
                 composition[META_RELATEDSIGNALS].push_back(relatedSignal);
             }
        }

        std::string m_valueName;
        Unit m_unit;
        PostScaling m_postScaling;
        Range m_range;
        RelatedSignals m_relatedSignals;
    };
}
