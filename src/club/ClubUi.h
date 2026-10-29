#pragma once
#include "club/Club.h"
#include "cards/TabletopController.h"
#include <functional>
namespace fl {
enum class ClubPage {Shop,Decks,Face,Match};
struct ClubButton {UiRect rect;std::string label;int action;};
class ClubUi {
public:
    bool open=false;ClubPage page=ClubPage::Shop;
    std::string query,name="My Deck",notice;bool naming=false;
    std::vector<std::uint16_t> draft;
    int offset=0,selected=0,deckIndex=0,colorFilter=0,rarityFilter=0;
    Face canvas=defaultFace();int ink=1,brush=4;bool eraser=false,painting=false,bucket=false;
    std::vector<Face> undo,redo;
    std::function<void(ClubCommand)> send;
    void show(ClubPage p,const ClubView& view);
    void close();
    void viewport(float w,float h);
    Vec2 point(float x,float y)const;
    void click(float x,float y,const ClubView& view,const std::vector<CardDefinition>& defs,int side);
    void move(float x,float y);void up();void wheel(float amount);
    void text(const std::string& text);void backspace();void undoPaint(bool forward);
    std::vector<ClubButton> buttons(const ClubView& view)const;
    std::vector<int> filtered(const ClubView& view,const std::vector<CardDefinition>& defs)const;
    float scale()const{return scale_;}float left()const{return left_;}float top()const{return top_;}
    void action(int action,const ClubView& view,const std::vector<CardDefinition>& defs,int side);
private:
    void loadDeck(const ClubView& view);void stroke(Vec2 to);void stamp(int x,int y);
    void history();
    float scale_=1,left_=0,top_=0;Vec2 last_{};Face before_;
};
}
