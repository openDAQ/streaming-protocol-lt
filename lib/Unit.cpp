#include "streaming_protocol/Unit.hpp"

namespace daq::streaming_protocol {
    const int32_t Unit::UNIT_ID_USER = 0;
    const int32_t Unit::UNIT_ID_NONE = -1;

    const int32_t Unit::UNIT_ID_SECONDS = 5457219;
    const int32_t Unit::UNIT_ID_MILLI_SECONDS = 4403766;

    Unit::Unit()
        : unitId(UNIT_ID_NONE)
    {}
};
