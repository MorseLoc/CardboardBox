#pragma once
#include "cards/TableCommand.h"
namespace fl::net {
enum class CardResult : std::uint8_t {Applied=0,NotSeated,Busy,Invalid,Private,NoChange};
const char* cardResultText(CardResult result);
// One short-lived table grab at a time. Atomic actions are serialized by host.
// A drag is canceled on stand, disconnect, focus loss, or lease expiry.
class TableSession {
public:
    Tabletop table;
    CardResult apply(int side,const TableCommand& command,bool seated,double now);
    bool release(int side);
    bool expire(double now);
    int holder() const {return holder_;}
private:
    bool allowedCard(CardId id,int side) const;
    bool allowedPile(PileId id,int side) const;
    int holder_=-1;
    double lease_=0;
    Vec2 dragPoint_{};
};
}
