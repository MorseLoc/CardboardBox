#pragma once
#include "cards/Tabletop.h"
#include <cstdint>

namespace fl {
struct CardImage { int width=512,height=720; std::vector<std::uint8_t> rgba; };
CardImage makeCardImage(const CardDefinition* definition); // nullptr = shared back
}
