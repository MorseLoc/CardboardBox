#pragma once
#include "cards/TableCommand.h"
#include <array>
namespace fl::net {
enum class CardResult : std::uint8_t {Applied=0,NotSeated,Busy,Invalid,Private,NoChange};
const char* cardResultText(CardResult result);
// Independent per-player leases lock only the cards/pile being manipulated.
// A drag is canceled on stand, disconnect, focus loss, or lease expiry.
class TableSession {
public:
    Tabletop table;
    CardResult apply(int side,const TableCommand& command,bool seated,double now);
    bool release(int side);
    bool expire(double now);
    bool holding(int side) const {return side>=0 && side<2 && leases_[static_cast<std::size_t>(side)].drag.active();}
private:
    bool allowedCard(CardId id,int side) const;
    bool allowedPile(PileId id,int side) const;
    std::vector<CardId> heldCards(int side) const;
    bool lockedPile(PileId id,int other) const;
    struct Lease {CardDrag drag;double expires=0;};
    std::array<Lease,2> leases_{};
};
}
