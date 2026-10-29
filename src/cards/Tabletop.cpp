#include "cards/Tabletop.h"
#include "cards/CardCatalog.h"
#include <algorithm>
#include <cmath>
#include <limits>
#include <set>
#include <array>

namespace fl {
namespace {
constexpr float pi=3.14159265359F;
float orientation(const CardInstance& c) { return c.angle+(c.tapped ? pi/2 : 0); }
float orientation(const CardPile& p) { return p.angle+(p.tapped ? pi/2 : 0); }
bool inside(Vec2 p,Vec2 center,float angle) {
    const float x=p.x-center.x,z=p.z-center.z;
    const float localX=x*std::cos(angle)-z*std::sin(angle);
    const float localZ=x*std::sin(angle)+z*std::cos(angle);
    return std::abs(localX)<=layout::cardWidth/2 && std::abs(localZ)<=layout::cardDepth/2;
}
bool overlapping(Vec2 a,float angleA,Vec2 b,float angleB) {
    const Vec2 ax{std::cos(angleA),-std::sin(angleA)},az{std::sin(angleA),std::cos(angleA)};
    const Vec2 bx{std::cos(angleB),-std::sin(angleB)},bz{std::sin(angleB),std::cos(angleB)};
    const auto dot=[](Vec2 p,Vec2 q){return p.x*q.x+p.z*q.z;};
    for(auto axis:{ax,az,bx,bz}) {
        const float aRadius=layout::cardWidth/2*std::abs(dot(ax,axis))+layout::cardDepth/2*std::abs(dot(az,axis));
        const float bRadius=layout::cardWidth/2*std::abs(dot(bx,axis))+layout::cardDepth/2*std::abs(dot(bz,axis));
        if(std::abs(dot({b.x-a.x,b.z-a.z},axis))>=aRadius+bRadius) return false;
    }
    return true;
}
}
std::optional<Vec2> onPlane(const Ray& ray,float y) {
    if (std::abs(ray.direction.y)<0.000001F) return {};
    const float t=(y-ray.origin.y)/ray.direction.y;
    if (t<0) return {};
    return Vec2{ray.origin.x+t*ray.direction.x,ray.origin.z+t*ray.direction.z};
}
Tabletop::Tabletop(std::uint32_t seed):rng_(seed) {
    definitions_=unlimitedDefinitions();
    for (int owner=0;owner<2;++owner) {
        const PileId id=nextPile_++;
        CardPile p{id,owner,{-2.8F,owner==0 ? 1.7F : -1.7F},{}};
        p.angle=owner==1 ? pi:0;
        for(const auto def:unlimitedDemoDeck(definitions_,owner)) {
            const CardId cid=nextCard_++;
            CardInstance c{cid,def,owner}; c.pile=id;c.order=nextOrder_++;
            cards_.emplace(cid,c);p.cards.push_back(cid);
        }
        piles_.emplace(id,std::move(p));shuffle(id);
    }
}
const CardInstance* Tabletop::card(CardId id) const { const auto it=cards_.find(id);return it==cards_.end()?nullptr:&it->second; }
const CardPile* Tabletop::pile(PileId id) const { const auto it=piles_.find(id);return it==piles_.end()?nullptr:&it->second; }
std::vector<CardId> Tabletop::hand(int viewer) const {
    std::vector<CardId> result;
    for (const auto& [id,c]:cards_) if (c.zone==CardZone::Hand && c.handOwner==viewer) result.push_back(id);
    std::sort(result.begin(),result.end(),[&](CardId a,CardId b){return cards_.at(a).order<cards_.at(b).order;});
    return result;
}
PileId Tabletop::library(int viewer) const {
    // Keep invisible original-library anchors so top/bottom returns still work
    // after drawing the whole deck. Empty player-made piles are deleted.
    const PileId id=viewer==0 ? 1U : (viewer==1 ? 2U : 0U);
    return pile(id) ? id : 0;
}
std::optional<std::size_t> Tabletop::visibleDefinition(CardId id,int viewer) const {
    const auto* c=card(id);if (!c || c->definition>=definitions_.size()) return {};
    if (c->zone==CardZone::Hand) return viewer==c->handOwner ? std::optional{c->definition}:std::nullopt;
    if (c->zone==CardZone::Pile) { const auto* p=pile(c->pile);if (!p || p->hidden) return {}; }
    return c->faceUp ? std::optional{c->definition}:std::nullopt;
}
TableHit Tabletop::pick(const Ray& ray,bool ignoreDragged) const {
    TableHit hit{};float nearest=std::numeric_limits<float>::max();std::uint64_t order=0;
    const auto consider=[&](float height,Vec2 position,float angle,CardId cid,PileId pid,std::uint64_t serial) {
        if (auto p=onPlane(ray,height);p && inside(*p,position,angle)) {
            const float t=(height-ray.origin.y)/ray.direction.y;
            if (t<nearest-0.00001F || (std::abs(t-nearest)<0.00001F && serial>order)) { nearest=t;order=serial;hit={cid,pid,*p}; }
        }
    };
    for (const auto& [id,c]:cards_) if (c.zone==CardZone::Table && !(ignoreDragged && dragged(id)))
        consider(c.height+layout::cardThickness/2,c.position,orientation(c),id,0,c.order);
    for (const auto& [id,p]:piles_) if (!p.cards.empty() && !(ignoreDragged && draggedPile(id))) {
        const float height=1.35F+layout::cardThickness*static_cast<float>(std::max<std::size_t>(1,p.cards.size()));
        consider(height,p.position,orientation(p),0,id,0);
    }
    return hit;
}
void Tabletop::detach(CardId id) {
    auto& c=cards_.at(id);
    if (c.zone==CardZone::Pile) {
        auto& list=piles_.at(c.pile).cards;
        list.erase(std::remove(list.begin(),list.end(),id),list.end());
    }
    c.pile=0;c.handOwner=-1;
}
void Tabletop::removeEmptyPiles() {
    // Run only after a complete move, never midway through detach/add. A return
    // to the same pile may temporarily empty its vector before filling it again.
    std::erase_if(piles_,[](const auto& entry){return entry.first>2 && entry.second.cards.empty();});
}
std::size_t Tabletop::draw(PileId id,int viewer,std::size_t count) {
    if (dragging() || viewer<0 || viewer>1 || !pile(id)) return 0;
    auto& p=piles_.at(id);
    if (p.hidden && p.owner!=viewer) return 0;
    const auto n=std::min(count,p.cards.size());
    for (std::size_t i=0;i<n;++i) {
        auto& c=cards_.at(p.cards.back());p.cards.pop_back();
        c.zone=CardZone::Hand;c.handOwner=viewer;c.pile=0;c.faceUp=true;c.tapped=false;c.angle=0;c.order=nextOrder_++;
    }
    removeEmptyPiles();
    return n;
}
bool Tabletop::shuffle(PileId id) {
    if (dragging() || !pile(id)) return false;
    auto& p=piles_.at(id);std::shuffle(p.cards.begin(),p.cards.end(),rng_);return true;
}
PileId Tabletop::split(PileId id) {
    if (dragging() || !pile(id) || pile(id)->cards.size()<2) return 0;
    auto& p=piles_.at(id);CardPile copy=p;copy.id=nextPile_++;copy.position.x+=0.5F;
    const auto middle=p.cards.begin()+static_cast<std::ptrdiff_t>(p.cards.size()/2);
    copy.cards.assign(middle,p.cards.end());p.cards.erase(middle,p.cards.end());
    for (auto cid:copy.cards) cards_.at(cid).pile=copy.id;
    const auto result=copy.id;piles_.emplace(result,std::move(copy));return result;
}
PileId Tabletop::group(std::span<const CardId> ids) {
    if (dragging() || ids.empty()) return 0;
    std::set<CardId> unique(ids.begin(),ids.end());
    for (auto id:unique) if (!card(id) || card(id)->zone!=CardZone::Table) return 0;
    const auto first=*card(ids.front());CardPile p{nextPile_++,first.owner,first.position,{}};p.hidden=false;p.angle=first.angle;p.tapped=first.tapped;
    // Selection order becomes bottom-to-top order, with duplicates rejected.
    if (unique.size()!=ids.size()) return 0;
    for (auto id:ids) { auto& c=cards_.at(id);detach(id);c.zone=CardZone::Pile;c.pile=p.id;p.cards.push_back(id); }
    const auto result=p.id;piles_.emplace(result,std::move(p));return result;
}
bool Tabletop::toHand(std::span<const CardId> ids,int viewer) {
    if (dragging() || viewer<0 || viewer>1 || ids.empty()) return false;
    for (auto id:ids) if (!card(id)) return false;
    for (auto id:ids) {detach(id);auto& c=cards_.at(id);c.zone=CardZone::Hand;c.handOwner=viewer;c.faceUp=true;c.tapped=false;c.order=nextOrder_++;}
    removeEmptyPiles();
    return true;
}
bool Tabletop::toPile(std::span<const CardId> ids,PileId target,bool bottom) {
    if (dragging() || ids.empty() || !pile(target)) return false;
    std::set<CardId> unique(ids.begin(),ids.end());if (unique.size()!=ids.size()) return false;
    for (auto id:ids) if (!card(id)) return false;
    for (auto id:ids) detach(id);
    auto& p=piles_.at(target);
    if (bottom) p.cards.insert(p.cards.begin(),ids.begin(),ids.end());
    else p.cards.insert(p.cards.end(),ids.begin(),ids.end());
    for (auto id:ids) {auto& c=cards_.at(id);c.zone=CardZone::Pile;c.pile=target;c.tapped=false;c.faceUp=!p.hidden;}
    removeEmptyPiles();
    return true;
}
void Tabletop::tap(std::span<const CardId> ids,PileId pid) {
    if (pid && pile(pid)) {auto& p=piles_.at(pid);p.tapped=!p.tapped;}
    for (auto id:ids) if (card(id)) {auto& c=cards_.at(id);c.tapped=!c.tapped;}
}
void Tabletop::flip(std::span<const CardId> ids,PileId pid) {
    if (pid && pile(pid)) {auto& p=piles_.at(pid);p.hidden=!p.hidden;for(auto id:p.cards) cards_.at(id).faceUp=!p.hidden;}
    for (auto id:ids) if (card(id)) {auto& c=cards_.at(id);c.faceUp=!c.faceUp;}
}
void Tabletop::rotate(std::span<const CardId> ids,PileId pid,float radians) {
    if (pid && pile(pid)) {auto& p=piles_.at(pid);p.angle=std::remainder(p.angle+radians,2*pi);}
    for (auto id:ids) if (card(id)) {auto& c=cards_.at(id);c.angle=std::remainder(c.angle+radians,2*pi);}
}
bool Tabletop::beginDrag(std::span<const CardId> ids,PileId pid,Vec2 anchor) {
    if (dragging()) return false;
    if (pid) {
        const auto* p=pile(pid);
        if(!p || p->cards.empty()) return false;
        drag_.pile=*p;
    }
    else {
        if (ids.empty()) return false;
        std::set<CardId> unique(ids.begin(),ids.end());if (unique.size()!=ids.size()) return false;
        for(auto id:ids) if (!card(id) || card(id)->zone==CardZone::Pile) return false;
        for(auto id:ids) drag_.cards.push_back(*card(id));
        for (std::size_t i=0;i<drag_.cards.size();++i) {
            auto& c=cards_.at(drag_.cards[i].id);
            if(c.zone==CardZone::Hand) {c.position={anchor.x+static_cast<float>(i)*0.04F,anchor.z};c.faceUp=true;c.angle=c.handOwner==1 ? pi:0;}
            c.zone=CardZone::Table;c.handOwner=-1;c.height+=0.15F;c.order=nextOrder_++;
        }
    }
    drag_.anchor=anchor;return true;
}
void Tabletop::moveDrag(Vec2 anchor) {
    if (!dragging()) return;
    const Vec2 delta{anchor.x-drag_.anchor.x,anchor.z-drag_.anchor.z};
    if (drag_.pile) {auto& p=piles_.at(drag_.pile->id);p.position.x+=delta.x;p.position.z+=delta.z;}
    for(const auto& original:drag_.cards) {auto& c=cards_.at(original.id);c.position.x+=delta.x;c.position.z+=delta.z;}
    drag_.anchor=anchor;
}
bool Tabletop::dragged(CardId id) const {return std::any_of(drag_.cards.begin(),drag_.cards.end(),[id](const auto& c){return c.id==id;});}
void Tabletop::settle(CardInstance& c,std::span<const CardId> otherHeld) {
    const auto held=[&](CardId id){return std::find(otherHeld.begin(),otherHeld.end(),id)!=otherHeld.end();};
    c.height=layout::cardCenterY;
    for(const auto& [id,other]:cards_) if(id!=c.id && other.zone==CardZone::Table && !dragged(id) && !held(id) && overlapping(c.position,orientation(c),other.position,orientation(other)))
        c.height=std::max(c.height,other.height+layout::cardThickness);
    for(const auto& [id,p]:piles_) if(!p.cards.empty() && !draggedPile(id) && !held(p.cards.front()) && overlapping(c.position,orientation(c),p.position,orientation(p)))
        c.height=std::max(c.height,1.35F+layout::cardThickness*static_cast<float>(std::max<std::size_t>(1,p.cards.size()))+layout::cardThickness/2);
}
void Tabletop::finishDrag(PileId target,std::span<const CardId> otherHeld) {
    if (!dragging()) return;
    std::vector<CardId> moving;
    if (drag_.pile) {
        if(target && target!=drag_.pile->id && pile(target)) moving=piles_.at(drag_.pile->id).cards;
    } else for(const auto& c:drag_.cards) moving.push_back(c.id);
    if(!target || !pile(target) || (drag_.pile && target==drag_.pile->id)) target=0;
    if(!target) {
        std::stable_sort(drag_.cards.begin(),drag_.cards.end(),[](const auto& a,const auto& b){return a.height<b.height;});
        while(!drag_.cards.empty()) {
            const auto id=drag_.cards.front().id;drag_.cards.erase(drag_.cards.begin());
            settle(cards_.at(id),otherHeld); // Already settled group members can support the next card.
        }
    }
    drag_.cards.clear();drag_.pile.reset();
    if(target && !moving.empty()) toPile(moving,target,false);
}
void Tabletop::cancelDrag() {
    for(const auto& c:drag_.cards) cards_.at(c.id)=c;
    if(drag_.pile) piles_.at(drag_.pile->id)=*drag_.pile;
    drag_.cards.clear();drag_.pile.reset();
}
CardDrag Tabletop::suspendDrag() {
    auto saved=std::move(drag_);drag_={};return saved;
}
void Tabletop::resumeDrag(CardDrag drag) {drag_=std::move(drag);}
bool Tabletop::valid() const {
    std::set<CardId> contained;
    for(const auto& [id,p]:piles_) if(id>2 && p.cards.empty()) return false;
    for(const auto& [id,p]:piles_) for(auto cid:p.cards) {
        const auto* c=card(cid);
        if(!c || c->zone!=CardZone::Pile || c->pile!=id || !contained.insert(cid).second) return false;
    }
    for(const auto& [id,c]:cards_) {
        if(c.id!=id || (c.definition!=hiddenDefinition && c.definition>=definitions_.size())) return false;
        if(c.zone==CardZone::Pile && !contained.contains(id)) return false;
        if(c.zone==CardZone::Hand && (c.handOwner<0 || c.handOwner>1)) return false;
    }
    return true;
}
TableView Tabletop::viewFor(int viewer) const {
    TableView out;
    // A hidden pile exposes only a count and anonymous slots, never real IDs,
    // definition indices, original copy owners, or shuffled order.
    for(const auto& [id,p]:piles_) {
        auto visible=p;
        if(p.hidden) {
            visible.cards.clear();
            for(std::size_t i=0;i<p.cards.size();++i) {
                const CardId alias=0x80000000U+id*256U+static_cast<CardId>(i);
                CardInstance c{alias,hiddenDefinition,p.owner};c.pile=id;
                out.cards.push_back(c);visible.cards.push_back(alias);
            }
        }
        out.piles.push_back(std::move(visible));
    }
    std::array<unsigned,2> handSlots{};
    for(const auto& [id,c]:cards_) {
        if(c.zone==CardZone::Pile && pile(c.pile)->hidden) continue;
        if(c.zone==CardZone::Hand && c.handOwner!=viewer) {
            const CardId alias=0xf0000000U+static_cast<unsigned>(c.handOwner)*256U+handSlots[static_cast<std::size_t>(c.handOwner)]++;
            CardInstance anonymous{alias,hiddenDefinition,c.handOwner};
            anonymous.zone=CardZone::Hand;anonymous.handOwner=c.handOwner;
            out.cards.push_back(anonymous);
        } else {
            auto visible=c;
            if(!visibleDefinition(id,viewer)) visible.definition=hiddenDefinition;
            out.cards.push_back(visible);
        }
    }
    return out;
}
bool Tabletop::applyView(const TableView& view) {
    if(view.cards.size()!=120 || view.piles.size()>122) return false;
    std::map<CardId,CardInstance> cards;
    std::map<PileId,CardPile> piles;
    for(const auto& c:view.cards) {
        if(!c.id || c.owner<0 || c.owner>1 ||
           (c.definition!=hiddenDefinition && c.definition>=definitions_.size()) ||
           !std::isfinite(c.position.x) || !std::isfinite(c.position.z) ||
           !std::isfinite(c.angle) || !std::isfinite(c.height) ||
           std::abs(c.position.x)>20 || std::abs(c.position.z)>20 || c.height<1 || c.height>3 ||
           (c.zone!=CardZone::Table && c.zone!=CardZone::Hand && c.zone!=CardZone::Pile) ||
           (c.zone==CardZone::Hand && (c.handOwner<0 || c.handOwner>1)) ||
           !cards.emplace(c.id,c).second) return false;
    }
    for(const auto& p:view.piles) {
        if(!p.id || p.id>1000000 || p.owner<0 || p.owner>1 ||
           !std::isfinite(p.position.x) || !std::isfinite(p.position.z) || !std::isfinite(p.angle) ||
           std::abs(p.position.x)>20 || std::abs(p.position.z)>20 || (p.id>2 && p.cards.empty()) ||
           !piles.emplace(p.id,p).second) return false;
    }
    if(!piles.contains(1) || !piles.contains(2)) return false;
    std::set<CardId> contained;
    for(const auto& [id,p]:piles) for(auto cid:p.cards) {
        const auto c=cards.find(cid);
        if(c==cards.end() || c->second.zone!=CardZone::Pile || c->second.pile!=id || !contained.insert(cid).second) return false;
    }
    for(const auto& [id,c]:cards) if(c.zone==CardZone::Pile && !contained.contains(id)) return false;
    cards_=std::move(cards);piles_=std::move(piles);drag_.cards.clear();drag_.pile.reset();return true;
}

}
