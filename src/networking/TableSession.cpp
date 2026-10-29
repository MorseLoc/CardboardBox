#include "networking/TableSession.h"
#include <algorithm>
#include <cmath>
#include <set>
namespace fl::net {
const char* cardResultText(CardResult r) {
    switch(r) {
    case CardResult::Applied:return "SHARED TABLE / HOST CONFIRMED";
    case CardResult::NotSeated:return "SIT IN YOUR ASSIGNED CHAIR TO PLAY";
    case CardResult::Busy:return "OBJECT BUSY - THAT CARD OR PILE IS ALREADY HELD";
    case CardResult::Private:return "THAT HAND OR HIDDEN PILE BELONGS TO THE OTHER PLAYER";
    case CardResult::NoChange:return "NO CHANGE - SELECT A VALID CARD OR PILE";
    default:return "TABLE REQUEST REJECTED - INVALID OR STALE SELECTION";
    }
}
bool TableSession::allowedCard(CardId id,int side) const {
    const auto* c=table.card(id);
    return c && (c->zone==CardZone::Table || (c->zone==CardZone::Hand && c->handOwner==side));
}
bool TableSession::allowedPile(PileId id,int side) const {
    const auto* p=table.pile(id);return p && (!p->hidden || p->owner==side);
}
std::vector<CardId> TableSession::heldCards(int side) const {
    const auto& drag=leases_[static_cast<std::size_t>(side)].drag;
    if(drag.pile) return drag.pile->cards;
    std::vector<CardId> ids;for(const auto& c:drag.cards) ids.push_back(c.id);return ids;
}
bool TableSession::lockedPile(PileId id,int other) const {
    if(!id) return false;
    const auto& drag=leases_[static_cast<std::size_t>(other)].drag;
    if(drag.pile && drag.pile->id==id) return true;
    const auto* p=table.pile(id);if(!p) return false;
    const auto held=heldCards(other);
    return std::any_of(p->cards.begin(),p->cards.end(),[&](CardId cid){return std::find(held.begin(),held.end(),cid)!=held.end();});
}
bool TableSession::release(int side) {
    if(!holding(side)) return false;
    auto& lease=leases_[static_cast<std::size_t>(side)];
    table.resumeDrag(std::move(lease.drag));table.cancelDrag();lease={};return true;
}
bool TableSession::expire(double now) {
    bool changed=false;
    for(int side=0;side<2;++side) if(holding(side) && now>leases_[static_cast<std::size_t>(side)].expires) changed=release(side)||changed;
    return changed;
}
CardResult TableSession::apply(int side,const TableCommand& c,bool seated,double now) {
    if(side<0 || side>1) return CardResult::Invalid;
    if(c.op==CardOp::Cancel) {release(side);return CardResult::Applied;}
    if(!seated) return CardResult::NotSeated;

    if(c.cards.size()>120 || std::set<CardId>(c.cards.begin(),c.cards.end()).size()!=c.cards.size() ||
       !std::isfinite(c.point.x) || !std::isfinite(c.point.z) || !std::isfinite(c.rotation) ||
       std::abs(c.point.x)>layout::feltWidth/2 || std::abs(c.point.z)>layout::feltDepth/2 ||
       std::abs(c.rotation)>0.27F || (c.count!=1 && c.count!=7)) return CardResult::Invalid;
    auto& lease=leases_[static_cast<std::size_t>(side)];
    const int other=1-side;
    const auto held=heldCards(other);
    if(c.op==CardOp::Move || c.op==CardOp::Finish) {
        if(!holding(side)) return CardResult::Invalid;
        table.resumeDrag(std::move(lease.drag));
        if(c.op==CardOp::Move) {
            float dx=c.point.x-table.dragAnchor().x,dz=c.point.z-table.dragAnchor().z;
            float lowX=-100,highX=100,lowZ=-100,highZ=100;
            const auto bound=[&](Vec2 p) {
                constexpr float margin=layout::cardDepth*0.6F;
                lowX=std::max(lowX,-layout::feltWidth/2+margin-p.x);highX=std::min(highX,layout::feltWidth/2-margin-p.x);
                lowZ=std::max(lowZ,-layout::feltDepth/2+margin-p.z);highZ=std::min(highZ,layout::feltDepth/2-margin-p.z);
            };
            for(const auto& [id,card]:table.cards()) if(table.dragged(id)) bound(card.position);
            for(const auto& [id,pile]:table.piles()) if(table.draggedPile(id)) bound(pile.position);
            if(lowX<=highX) dx=std::clamp(dx,lowX,highX);else dx=0;
            if(lowZ<=highZ) dz=std::clamp(dz,lowZ,highZ);else dz=0;
            const auto point=table.dragAnchor();table.moveDrag({point.x+dx,point.z+dz});lease.drag=table.suspendDrag();lease.expires=now+3;return CardResult::Applied;
        }
        // Releasing over a held/private/stale target drops freely instead of
        // merging into it. The target stays untouched and this lease ends.
        const auto target=c.target && !lockedPile(c.target,other) && allowedPile(c.target,side)?c.target:0;
        table.finishDrag(target,held);lease={};return CardResult::Applied;
    }
    if(holding(side)) return CardResult::Busy;
    for(auto id:c.cards) if(std::find(held.begin(),held.end(),id)!=held.end()) return CardResult::Busy;
    PileId touched=c.pile;
    if((c.op==CardOp::Draw || c.op==CardOp::Shuffle) && !touched) touched=table.library(side);
    if(c.op==CardOp::Return) touched=table.library(side);
    if(lockedPile(touched,other)) return CardResult::Busy;
    for(auto id:c.cards) if(!allowedCard(id,side)) return CardResult::Private;
    if(c.pile && !allowedPile(c.pile,side)) return CardResult::Private;
    if(c.pile && !c.cards.empty()) return CardResult::Invalid;
    switch(c.op) {
    case CardOp::Draw:
        return table.draw(c.pile?c.pile:table.library(side),side,c.count) ? CardResult::Applied:CardResult::NoChange;
    case CardOp::Shuffle:
        return table.shuffle(c.pile?c.pile:table.library(side)) ? CardResult::Applied:CardResult::NoChange;
    case CardOp::Group: return table.group(c.cards)?CardResult::Applied:CardResult::NoChange;
    case CardOp::Split: return table.split(c.pile)?CardResult::Applied:CardResult::NoChange;
    case CardOp::Hand: return table.toHand(c.cards,side)?CardResult::Applied:CardResult::NoChange;
    case CardOp::Return: return table.toPile(c.cards,table.library(side),c.bottom)?CardResult::Applied:CardResult::NoChange;
    case CardOp::Begin:
        if(table.beginDrag(c.cards,c.pile,c.point)) {lease.drag=table.suspendDrag();lease.expires=now+3;return CardResult::Applied;}
        return CardResult::NoChange;
    case CardOp::Center: {
        Vec2 anchor{};
        if(c.pile) anchor=table.pile(c.pile)->position;
        else if(!c.cards.empty()) anchor=table.card(c.cards.front())->position;
        if(table.beginDrag(c.cards,c.pile,anchor)) {table.moveDrag({0,0});table.finishDrag(0,held);return CardResult::Applied;}
        return CardResult::NoChange;
    }
    case CardOp::Tap:case CardOp::Flip:case CardOp::Rotate:
        if(!c.pile && c.cards.empty()) return CardResult::NoChange;
        if(c.op==CardOp::Tap) table.tap(c.cards,c.pile);
        if(c.op==CardOp::Flip) table.flip(c.cards,c.pile);
        if(c.op==CardOp::Rotate) table.rotate(c.cards,c.pile,c.rotation);
        return CardResult::Applied;
    default:return CardResult::Invalid;
    }
}
}
