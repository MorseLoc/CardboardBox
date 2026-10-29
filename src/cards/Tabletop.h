#pragma once
#include "world/Geometry.h"
#include "world/TableLayout.h"
#include <cstdint>
#include <map>
#include <optional>
#include <random>
#include <span>
#include <string>
#include <vector>

namespace fl {
using CardId=std::uint32_t;
using PileId=std::uint32_t;
struct Ray { Vec3 origin,direction; };
std::optional<Vec2> onPlane(const Ray& ray,float y);
struct CardDefinition {
    std::string id,name,type,cost,rules,power;
    Vec3 color;
    std::string setId{},rarity{},collectorNumber{},scryfallId{},oracleText{},colorIdentity{};
    bool basicLand=false;
};
enum class CardZone { Table,Hand,Pile };
struct CardInstance {
    CardId id;
    std::size_t definition;
    int owner; // Original copy owner; movement never changes it.
    CardZone zone=CardZone::Pile;
    int handOwner=-1;
    PileId pile=0;
    Vec2 position{};
    float angle=0,height=layout::cardCenterY;
    bool faceUp=false,tapped=false;
    std::uint64_t order=0;
};
struct CardPile {
    PileId id;
    int owner;
    Vec2 position;
    std::vector<CardId> cards; // bottom -> top; back() is drawn first
    float angle=0;
    bool hidden=true,tapped=false;
};
struct TableHit { CardId card=0; PileId pile=0; Vec2 point{}; };
class Tabletop {
public:
    explicit Tabletop(std::uint32_t seed=std::random_device{}());
    const std::vector<CardDefinition>& definitions() const { return definitions_; }
    const std::map<CardId,CardInstance>& cards() const { return cards_; }
    const std::map<PileId,CardPile>& piles() const { return piles_; }
    const CardInstance* card(CardId id) const;
    const CardPile* pile(PileId id) const;
    std::vector<CardId> hand(int viewer) const;
    PileId library(int viewer) const;
    std::optional<std::size_t> visibleDefinition(CardId id,int viewer) const;
    TableHit pick(const Ray& ray,bool ignoreDragged=false) const;
    std::size_t draw(PileId pile,int handOwner,std::size_t count);
    bool shuffle(PileId id);
    PileId split(PileId id);
    PileId group(std::span<const CardId> ids);
    bool toHand(std::span<const CardId> ids,int viewer);
    bool toPile(std::span<const CardId> ids,PileId pile,bool bottom);
    void tap(std::span<const CardId> ids,PileId pile);
    void flip(std::span<const CardId> ids,PileId pile);
    void rotate(std::span<const CardId> ids,PileId pile,float radians);
    bool beginDrag(std::span<const CardId> ids,PileId pile,Vec2 anchor);
    void moveDrag(Vec2 anchor);
    void finishDrag(PileId mergeTarget=0);
    void cancelDrag();
    bool dragging() const { return !dragCards_.empty() || dragPile_.has_value(); }
    bool dragged(CardId id) const;
    bool draggedPile(PileId id) const { return dragPile_ && dragPile_->id==id; }
    bool valid() const;
private:
    void detach(CardId id);
    void removeEmptyPiles();
    void settle(CardInstance& card);
    std::vector<CardDefinition> definitions_;
    std::map<CardId,CardInstance> cards_;
    std::map<PileId,CardPile> piles_;
    std::vector<CardInstance> dragCards_;
    std::optional<CardPile> dragPile_;
    Vec2 dragAnchor_{};
    std::mt19937 rng_;
    CardId nextCard_=1;
    PileId nextPile_=1;
    std::uint64_t nextOrder_=1;
};
}
