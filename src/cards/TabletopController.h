#pragma once
#include "cards/Tabletop.h"
#include <string_view>

namespace fl {
struct UiRect { float x,y,w,h; bool contains(float px,float py) const {return px>=x && px<=x+w && py>=y && py<=y+h;} };
struct TableUiLayout {
    float width,height,scale;
    explicit TableUiLayout(float w,float h):width(w),height(h),scale(std::max(0.65F,std::min(w/1280.0F,h/720.0F))) {}
    UiRect handArea() const {return {0,height-190*scale,width,190*scale};}
    UiRect handCard(int index) const {return {60*scale+static_cast<float>(index)*84*scale,height-150*scale,70*scale,100*scale};}
    int handSlots() const {return std::max(1,static_cast<int>((width-120*scale)/(84*scale)));}
    UiRect previous() const {return {16*scale,height-125*scale,30*scale,45*scale};}
    UiRect next() const {return {width-46*scale,height-125*scale,30*scale,45*scale};}
};
enum class TableAction { Tap,Flip,RotateLeft,RotateRight,Draw,DrawSeven,Shuffle,Group,Split,Hand,Top,Bottom,Center };
class TabletopController {
public:
    explicit TabletopController(Tabletop& table):table_(table) {}
    void setViewport(float width,float height) {width_=width;height_=height;}
    void setViewer(int viewer);
    void pointerMove(float x,float y,std::optional<Ray> ray);
    void leftDown(float x,float y,std::optional<Ray> ray,bool control,bool doubleClick);
    void leftUp(float x,float y,std::optional<Ray> ray,bool merge);
    void rightDown(float x,float y,std::optional<Ray> ray,bool menu);
    bool wheel(float amount,float x,float y);
    void action(TableAction action);
    bool escape();
    void cancel();
    int viewer() const {return viewer_;}
    const std::vector<CardId>& selected() const {return selected_;}
    PileId selectedPile() const {const auto* p=table_.pile(selectedPile_);return p && !p->cards.empty() ? selectedPile_:0;}
    bool selected(CardId id) const;
    bool menuOpen() const {return menuOpen_;}
    UiRect menuRect() const;
    static constexpr int menuRows=10;
    static std::string_view menuLabel(int row);
    bool menuEnabled(int row) const;
    std::optional<CardId> inspected() const;
    bool inspectionOpen() const {return inspectCard_!=0 || inspectPile_!=0;}
    std::string inspectionCaption() const;
    int handOffset() const {return handOffset_;}
    std::string_view status() const {return status_;}
private:
    CardId handHit(float x,float y) const;
    void inspect();
    void clearSelection();
    void invokeMenu(int row);
    Tabletop& table_;
    int viewer_=-1,handOffset_=0,inspectIndex_=0;
    std::vector<CardId> selected_;
    PileId selectedPile_=0,inspectPile_=0;
    CardId inspectCard_=0;
    bool menuOpen_=false,pressed_=false,startedDrag_=false;
    float width_=1280,height_=720,pressX_=0,pressY_=0,menuX_=0,menuY_=0;
    Vec2 pressAnchor_{};
    std::string status_="DOUBLE-CLICK YOUR LIBRARY TO DRAW";
};
}
