#pragma once
#include "cards/Tabletop.h"
namespace fl {
enum class CardOp : std::uint8_t {Tap=1,Flip,Rotate,Draw,Shuffle,Group,Split,Hand,Return,Begin,Move,Finish,Cancel,Center};
struct TableCommand {
    std::uint32_t generation=0;
    CardOp op=CardOp::Cancel;
    std::vector<CardId> cards;
    PileId pile=0,target=0;
    Vec2 point{};
    float rotation=0;
    std::uint8_t count=1;
    bool bottom=false;
};
}
