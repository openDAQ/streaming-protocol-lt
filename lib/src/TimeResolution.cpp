#include "streaming_protocol/TimeResolution.hpp"

static const double epsilon = 0.000001;

const TimeResolution::Resolution TIME_RESOLUTION_1HZ = { 1, 1};
const TimeResolution::Resolution TIME_FAMILY_1GHZ = { 1, 1000000000 };
const TimeResolution::Resolution TIME_FAMILY_NTP = { 1, (uint64_t(1) << 32) };
