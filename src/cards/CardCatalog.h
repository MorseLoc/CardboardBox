#pragma once
#include "cards/Tabletop.h"

namespace fl {
std::vector<CardDefinition> unlimitedDefinitions();
std::vector<std::size_t> unlimitedDemoDeck(const std::vector<CardDefinition>& definitions,int side);
}
