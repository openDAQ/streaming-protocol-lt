#include "streaming_protocol/Defines.h"
#include "streaming_protocol/Unit.hpp"

namespace daq::streaming_protocol {
    const int32_t Unit::UNIT_ID_USER = 0;
    const int32_t Unit::UNIT_ID_NONE = -1;

    const int32_t Unit::UNIT_ID_SECONDS = 5457219;
    const int32_t Unit::UNIT_ID_MILLI_SECONDS = 4403766;

    Unit::Unit()
        : id(UNIT_ID_NONE)
    {}

    void Unit::clear()
    {
        id = Unit::UNIT_ID_NONE;
        displayName.clear();
        quantity.clear();
    }

    void Unit::compose(nlohmann::json &composition) const
    {
        if (id != Unit::UNIT_ID_NONE) {
            composition[META_UNIT][META_UNIT_ID] = id;
            composition[META_UNIT][META_DISPLAY_NAME] = displayName;
            if (!quantity.empty()) {
                // quantity is optional
                composition[META_UNIT][META_QUANTITY] = quantity;
            }
        }
    }

    void Unit::parse(const nlohmann::json& composition)
    {
        auto unitNode = composition.find(META_UNIT);
        if (unitNode!=composition.end()) {
            auto uintIdIter = unitNode->find(META_UNIT_ID);
            if (uintIdIter!=unitNode->end()) {
                id = *uintIdIter;
            }
            auto displayNameIter = unitNode->find(META_DISPLAY_NAME);
            if (displayNameIter!=unitNode->end()) {
                displayName = *displayNameIter;
            }
            auto quantityIter = unitNode->find(META_QUANTITY);
            if (quantityIter!=unitNode->end()) {
                quantity = *quantityIter;
            }
        }

    }
};
