#include "cards/TabletopController.h"
#include <algorithm>
#include <cmath>

namespace fl {
bool TabletopController::selected(CardId id) const {return std::find(selected_.begin(),selected_.end(),id)!=selected_.end();}
void TabletopController::clearSelection() {selected_.clear();selectedPile_=0;}
void TabletopController::setViewer(int viewer) {
    if(viewer_==viewer) return;
    cancel();clearSelection();handOffset_=0;viewer_=viewer;
}
CardId TabletopController::handHit(float x,float y) const {
    const TableUiLayout ui{width_,height_};const auto hand=table_.hand(viewer_);
    for(int i=0;i<ui.handSlots();++i) {
        const auto at=static_cast<std::size_t>(std::max(0,handOffset_)+i);
        if(at<hand.size() && ui.handCard(i).contains(x,y)) return hand[at];
    }
    return 0;
}
void TabletopController::pointerMove(float x,float y,std::optional<Ray> ray) {
    if(!pressed_ || viewer_<0 || !ray) return;
    const TableUiLayout ui{width_,height_};
    if(ui.handArea().contains(x,y)) return;
    if(!startedDrag_ && std::hypot(x-pressX_,y-pressY_)>5.0F) {
        startedDrag_=table_.beginDrag(selected_,selectedPile_,pressAnchor_);
    }
    if(startedDrag_) if(auto point=onPlane(*ray,layout::cardCenterY)) table_.moveDrag(*point);
}
void TabletopController::leftDown(float x,float y,std::optional<Ray> ray,bool control,bool doubleClick) {
    if(viewer_<0) return;
    if(inspectionOpen()) {inspectCard_=0;inspectPile_=0;return;}
    if(menuOpen_) {
        const auto rect=menuRect();menuOpen_=false;
        if(rect.contains(x,y)) invokeMenu(static_cast<int>((y-rect.y)/(28*TableUiLayout{width_,height_}.scale)));
        return;
    }
    const TableUiLayout ui{width_,height_};
    const int maxOffset=std::max(0,static_cast<int>(table_.hand(viewer_).size())-ui.handSlots());
    if(ui.previous().contains(x,y)) {handOffset_=std::max(0,handOffset_-ui.handSlots());return;}
    if(ui.next().contains(x,y)) {handOffset_=std::min(maxOffset,handOffset_+ui.handSlots());return;}
    if(y<108*ui.scale) return;
    const CardId inHand=handHit(x,y);
    TableHit hit=ray ? table_.pick(*ray):TableHit{};
    if(inHand) hit={inHand,0,{}};
    else if(ui.handArea().contains(x,y)) {clearSelection();return;}
    if(!hit.card && !hit.pile) {clearSelection();return;}
    if(hit.pile) {
        clearSelection();selectedPile_=hit.pile;
        if(doubleClick) {action(TableAction::Draw);return;}
    } else if(control) {
        selectedPile_=0;
        // Keep hand and tabletop selections separate for predictable group drags.
        if(!selected_.empty() && table_.card(selected_.front())->zone!=table_.card(hit.card)->zone) selected_.clear();
        if(selected(hit.card)) selected_.erase(std::remove(selected_.begin(),selected_.end(),hit.card),selected_.end());
        else selected_.push_back(hit.card);
        return;
    } else if(!selected(hit.card)) {clearSelection();selected_.push_back(hit.card);}
    pressed_=true;startedDrag_=false;pressX_=x;pressY_=y;
    pressAnchor_=ray ? onPlane(*ray,layout::cardCenterY).value_or(Vec2{}):Vec2{};
}
void TabletopController::leftUp(float x,float y,std::optional<Ray> ray,bool merge) {
    if(startedDrag_) {
        pointerMove(x,y,ray);
        if(TableUiLayout{width_,height_}.handArea().contains(x,y)) {
            // Dropping above the hand bar cancels the motion, then explicitly
            // moves loose selections into the active viewer's hidden hand.
            table_.cancelDrag();
            if(!selected_.empty()) table_.toHand(selected_,viewer_);
        } else {
            const auto target=(merge && ray) ? table_.pick(*ray,true).pile:0;
            table_.finishDrag(target);
        }
    }
    pressed_=false;startedDrag_=false;
}
void TabletopController::rightDown(float x,float y,std::optional<Ray> ray,bool menu) {
    if(inspectionOpen()) {inspectCard_=0;inspectPile_=0;return;}
    if(table_.dragging()) {table_.cancelDrag();pressed_=false;startedDrag_=false;return;}
    const CardId ownHand=handHit(x,y);
    if(!ownHand && TableUiLayout{width_,height_}.handArea().contains(x,y)) return;
    const auto hit=ownHand ? TableHit{ownHand,0,{}} : (ray ? table_.pick(*ray):TableHit{});
    if(!hit.card && !hit.pile) {menuOpen_=false;return;}
    if(hit.pile) {clearSelection();selectedPile_=hit.pile;}
    else if(!selected(hit.card)) {clearSelection();selected_.push_back(hit.card);}
    if(hit.card && table_.visibleDefinition(hit.card,viewer_) && !menu) {inspect();return;}
    menuX_=x;menuY_=y;menuOpen_=true;
}
UiRect TabletopController::menuRect() const {
    const float s=TableUiLayout{width_,height_}.scale,w=260*s,h=28*s*menuRows;
    return {std::clamp(menuX_,8*s,std::max(8*s,width_-w-8*s)),
            std::clamp(menuY_,8*s,std::max(8*s,height_-h-8*s)),w,h};
}
std::string_view TabletopController::menuLabel(int row) {
    constexpr std::string_view labels[]={"INSPECT / VIEW PUBLIC PILE","DRAW ONE","DRAW SEVEN","SHUFFLE PILE","MAKE PILE FROM SELECTION","SPLIT PILE","REVEAL / HIDE PILE OR CARD","MOVE SELECTION TO HAND","RETURN TO LIBRARY TOP","RETURN TO LIBRARY BOTTOM"};
    return row>=0 && row<menuRows ? labels[row]:"";
}
bool TabletopController::menuEnabled(int row) const {
    const auto* p=table_.pile(selectedPile_);
    switch(row) {
    case 0: return p ? !p->hidden && !p->cards.empty() : (!selected_.empty() && table_.visibleDefinition(selected_.front(),viewer_).has_value());
    case 1: case 2: return p && !p->cards.empty() && (!p->hidden || p->owner==viewer_);
    case 3: case 5: return p && p->cards.size()>=2;
    case 4: return !selected_.empty() && std::all_of(selected_.begin(),selected_.end(),[&](CardId id){const auto* c=table_.card(id);return c && c->zone==CardZone::Table;});
    case 6: return p || !selected_.empty();
    case 7: case 8: case 9: return !selected_.empty();
    default: return false;
    }
}
void TabletopController::invokeMenu(int row) {
    if(!menuEnabled(row)) return;
    if(row==0) {inspect();return;}
    constexpr TableAction actions[]={TableAction::Draw,TableAction::DrawSeven,TableAction::Shuffle,TableAction::Group,TableAction::Split,TableAction::Flip,TableAction::Hand,TableAction::Top,TableAction::Bottom};
    if(row>=1 && row<menuRows) action(actions[row-1]);
}
void TabletopController::inspect() {
    inspectCard_=0;inspectPile_=0;inspectIndex_=0;
    if(selectedPile_) {
        const auto* p=table_.pile(selectedPile_);
        if(p && !p->hidden && !p->cards.empty()) {inspectPile_=p->id;inspectIndex_=static_cast<int>(p->cards.size())-1;}
    } else if(!selected_.empty() && table_.visibleDefinition(selected_.front(),viewer_)) inspectCard_=selected_.front();
    if(!inspectionOpen()) status_="FACE-DOWN CARD OR HIDDEN PILE - REVEAL WITH F FIRST";
}
std::optional<CardId> TabletopController::inspected() const {
    CardId id=inspectCard_;
    if(inspectPile_) {const auto* p=table_.pile(inspectPile_);if(!p || p->hidden || p->cards.empty()) return {};id=p->cards[static_cast<std::size_t>(std::clamp(inspectIndex_,0,static_cast<int>(p->cards.size())-1))];}
    return id && table_.visibleDefinition(id,viewer_) ? std::optional{id}:std::nullopt;
}
std::string TabletopController::inspectionCaption() const {
    if(inspectPile_) {const auto* p=table_.pile(inspectPile_);return p ? "PILE CARD "+std::to_string(inspectIndex_+1)+" / "+std::to_string(p->cards.size())+" - WHEEL TO BROWSE":"";}
    return "INSPECTION - CLICK OR ESC TO CLOSE";
}
bool TabletopController::wheel(float amount,float x,float y) {
    if(inspectionOpen()) {
        if(inspectPile_) if(const auto* p=table_.pile(inspectPile_);p && !p->cards.empty())
            inspectIndex_=std::clamp(inspectIndex_+(amount>0 ? 1:-1),0,static_cast<int>(p->cards.size())-1);
        return true;
    }
    const TableUiLayout ui{width_,height_};
    if(ui.handArea().contains(x,y)) {
        const int maxOffset=std::max(0,static_cast<int>(table_.hand(viewer_).size())-ui.handSlots());
        handOffset_=std::clamp(handOffset_+(amount>0 ? -1:1),0,maxOffset);return true;
    }
    return menuOpen_;
}
void TabletopController::action(TableAction action) {
    if(viewer_<0 || inspectionOpen() || table_.dragging()) return;
    menuOpen_=false;pressed_=false;
    const auto target=selectedPile_ ? selectedPile_:table_.library(viewer_);
    switch(action) {
    case TableAction::Tap: table_.tap(selected_,selectedPile_);break;
    case TableAction::Flip: table_.flip(selected_,selectedPile_);break;
    case TableAction::RotateLeft: table_.rotate(selected_,selectedPile_,-0.261799388F);break;
    case TableAction::RotateRight: table_.rotate(selected_,selectedPile_,0.261799388F);break;
    case TableAction::Draw: case TableAction::DrawSeven: {
        const auto n=table_.draw(target,viewer_,action==TableAction::Draw ? 1:7);
        status_="DREW "+std::to_string(n)+" CARD[S]";break;
    }
    case TableAction::Shuffle: status_=table_.shuffle(target)?"PILE SHUFFLED":"NO PILE";break;
    case TableAction::Group: {const auto p=table_.group(selected_);if(p) {clearSelection();selectedPile_=p;status_="SELECTION GROUPED INTO ORDERED PILE";}break;}
    case TableAction::Split: {const auto p=table_.split(selectedPile_);if(p) {selectedPile_=p;status_="TOP HALF SPLIT INTO NEW PILE";}break;}
    case TableAction::Hand: table_.toHand(selected_,viewer_);break;
    case TableAction::Top: case TableAction::Bottom: table_.toPile(selected_,table_.library(viewer_),action==TableAction::Bottom);clearSelection();break;
    case TableAction::Center:
        if(selectedPile_) {if(const auto* p=table_.pile(selectedPile_);p && table_.beginDrag({},p->id,p->position)) {table_.moveDrag({0,0});table_.finishDrag();}}
        else if(!selected_.empty()) {if(const auto* c=table_.card(selected_.front());c && table_.beginDrag(selected_,0,c->position)) {table_.moveDrag({0,0});table_.finishDrag();}}
        break;
    }
    handOffset_=std::clamp(handOffset_,0,std::max(0,static_cast<int>(table_.hand(viewer_).size())-TableUiLayout{width_,height_}.handSlots()));
}
bool TabletopController::escape() {
    if(table_.dragging() || pressed_) {table_.cancelDrag();pressed_=false;startedDrag_=false;return true;}
    if(inspectionOpen()) {inspectCard_=0;inspectPile_=0;return true;}
    if(menuOpen_) {menuOpen_=false;return true;}
    return false;
}
void TabletopController::cancel() {table_.cancelDrag();pressed_=false;startedDrag_=false;menuOpen_=false;inspectCard_=0;inspectPile_=0;}
}
