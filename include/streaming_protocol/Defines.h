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

#include <stdint.h>

namespace daq::streaming_protocol {

static const uint32_t SIGNAL_NUMBER_MASK = 0x000fffff;
static const uint32_t TYPE_MASK = 0x30000000;
static const uint32_t TYPE_SHIFT = 28;
static const uint32_t SIZE_MASK = 0x0ff00000;
static const uint32_t SIZE_SHIFT = 20;

// presentation layer
static const uint32_t METAINFORMATION_MSGPACK = 2; /// Used in openDAQ streaming protocol

static const char PARAMS[] = "params";
static const char METHOD[] = "method";

// stream related meta information
/// This one always comes first!
static const char META_METHOD_APIVERSION[] = "apiVersion";
static const char VERSION[] = "version";
static const char META_METHOD_INIT[] = "init";
static const char COMMANDINTERFACES[] = "commandInterfaces";

/// Will carry an array with signal ids of signals that just got available.
/// Only changes are being told here if for example one signal was already available and two others are becoming available later,
/// There will be one msg telling about the first one and another one telling about the two latter ones.
static const char META_METHOD_AVAILABLE[] = "available";
/// Will carry an array with signal ids of signals that just became unavailable.
/// As for META_METHOD_AVAILABLE, only changes are transmitted!
static const char META_METHOD_UNAVAILABLE[] = "unavailable";

// signal related meta information
static const char META_STREAMID[] = "streamId";
static const char META_METHOD_SIGNAL[] = "signal";
static const char META_SIGNALID[] = "signalId";
static const char META_SIGNALIDS[] = "signalIds";

static const char META_TABLEID[] = "tableId";
static const char META_VALUEINDEX[] = "valueIndex";
static const char META_RELATEDSIGNALS[] = "relatedSignals";

/// Is send to acknowldege the sunscription of a signal. It carries the signal id togehter with the signal number.
static const char META_METHOD_SUBSCRIBE[] = "subscribe";
/// Is send to acknowldege that sunscription a signal got unsubscribed.
static const char META_METHOD_UNSUBSCRIBE[] = "unsubscribe";

static const char META_DATATYPE[] = "dataType";
static const char META_INTERPRETATION[] = "interpretation";

static const char DATA_TYPE_INT8[] = "int8";
static const char DATA_TYPE_UINT8[] = "uint8";
static const char DATA_TYPE_INT16[] = "int16";
static const char DATA_TYPE_UINT16[] = "uint16";
static const char DATA_TYPE_INT32[] = "int32";
static const char DATA_TYPE_UINT32[] = "uint32";
static const char DATA_TYPE_INT64[] = "int64";
static const char DATA_TYPE_UINT64[] = "uint64";
static const char DATA_TYPE_REAL32[] = "real32";
static const char DATA_TYPE_REAL64[] = "real64";

/// contains one float value with the real part and one float with imaginary part
static const char DATA_TYPE_COMPLEX32[] = "complex32";
/// contains one double value with the real part and one double with imaginary part
static const char DATA_TYPE_COMPLEX64[] = "complex64";

static const char DATA_TYPE_ARRAY[] = "array";
static const char DATA_TYPE_DYNAMIC_ARRAY[] = "dynamicArray";
static const char DATA_TYPE_STRUCT[] = "struct";
static const char DATA_TYPE_BITFIELD[] = "bitField";

static const char META_COUNT[] = "count";
static const char META_DEFINITION[] = "definition";
static const char META_RULE[] = "rule";
static const char META_RULETYPE_EXPLICIT[] = "explicit";
static const char META_RULETYPE_LINEAR[] = "linear";
static const char META_RULETYPE_CONSTANT[] = "constant";
static const char META_NAME[] = "name";
static const char META_TIME[] = "time";

/// openDAQ
static const char META_RESOLUTION[] = "resolution";
static const char META_NUMERATOR[] = "num";
static const char META_DENOMINATOR[] = "denom";
static const char META_ABSOLUTE_REFERENCE[] = "absoluteReference";

/// ISO 8601:2004 date format: YYYY-MM-DD
static const char UNIX_EPOCH[] = "1970-01-01";

/// ISO 8601:2004 UTC date time format: YYYY-MM-DDThh:mm:ssZ
static const char UNIX_EPOCH_DATE_UTC_TIME[] = "1970-01-01T00:00:00Z";


/// If enabled, the producer writes this periodically. The client has to read all the datat coming in between before this one is read!
/// It carries the fill level of the device ringbuffer at the time of writing
static const char META_METHOD_ALIVE[] = "alive";
static const char META_FILLLEVEL[] = "fillLevel";
static const char META_START[] = "start";
static const char META_DELTA[] = "delta";
static const char META_UNIT[] = "unit";
static const char META_DISPLAY_NAME[] = "displayName";
static const char META_UNIT_ID[] = "unitId";
static const char META_QUANTITY[] = "quantity";



/**
 * @todo set the decided version!
 */
static const char OPENDAQ_LT_STREAM_VERSION[] = "0.7.0";

}
