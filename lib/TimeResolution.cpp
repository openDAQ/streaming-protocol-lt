#include <stdint.h>

#include "streaming_protocol/TimeResolution.hpp"

namespace daq::streaming_protocol {
    const Resolution TIME_RESOLUTION_1HZ(1, 1);
    const Resolution TIME_FAMILY_1GHZ(1, 1000000000);
    const Resolution TIME_FAMILY_NTP (1, (uint64_t(1) << 32));
}
