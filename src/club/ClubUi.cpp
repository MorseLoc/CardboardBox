#include "club/ClubUi.h"
#include <algorithm>
#include <cctype>
#include <cmath>
namespace fl {
namespace {std::string lower(std::string s){for(auto& c:s)c=static_cast<char>(std::tolower(static_cast<unsigned char>(c)));return s;}}
void ClubUi::viewport(float w,float h){scale_=std::min(w/1280,h/720);left_=(w-1280*scale_)/2;top_=(h-720*scale_)/2;}
Vec2 ClubUi::point(float x,float y)const{return{(x-left_)/scale_,(y-top_)/scale_};}
void ClubUi::loadDeck(const ClubView& v){if(v.profile.decks.empty()){draft.clear();name="My Deck";return;}deckIndex=std::clamp(deckIndex,0,static_cast<int>(v.profile.decks.size())-1);name=v.profile.decks[deckIndex].name;draft=v.profile.decks[deckIndex].cards;}
void ClubUi::show(ClubPage p,const ClubView& v){open=true;page=p;painting=false;notice.clear();query.clear();offset=0;naming=false;if(page==ClubPage::Decks)loadDeck(v);if(selected<0||selected>=static_cast<int>(v.profile.owned.size())||!v.profile.owned[selected]){selected=0;for(std::size_t i=0;i<v.profile.owned.size();++i)if(v.profile.owned[i]){selected=static_cast<int>(i);break;}}if(page==ClubPage::Shop&&!v.opened.empty())selected=v.opened.front();if(page==ClubPage::Face){canvas=v.faces[0];undo.clear();redo.clear();}}
void ClubUi::close(){up();open=false;}
std::vector<int> ClubUi::filtered(const ClubView& v,const std::vector<CardDefinition>& defs)const {
    std::vector<int> out;const auto search=lower(query);const std::string colors="WUBRG";const char* rarities[]={"","common","uncommon","rare"};
    for(std::size_t i=0;i<defs.size()&&i<v.profile.owned.size();++i){const auto& d=defs[i];if(!v.profile.owned[i])continue;if(!search.empty()&&lower(d.name+" "+d.type+" "+d.rarity).find(search)==std::string::npos)continue;if(colorFilter&&d.colorIdentity.find(colors[static_cast<std::size_t>(colorFilter-1)])==std::string::npos)continue;if(rarityFilter&&d.rarity!=rarities[rarityFilter])continue;out.push_back(static_cast<int>(i));}return out;
}
std::vector<ClubButton> ClubUi::buttons(const ClubView& v)const {
    std::vector<ClubButton> b{{{24,68,220,42},"SHOP",1},{{258,68,220,42},"DECK BUILDER",2},{{492,68,220,42},"PAINT FACE",3},{{726,68,220,42},"MATCH",4},{{1100,68,150,42},"CLOSE",5}};
    const auto add=[&](float x,float y,float w,const std::string& label,int action){b.push_back({{x,y,w,38},label,action});};
    if(page==ClubPage::Shop){add(36,145,270,"BUY PACK / 100 COINS",10);const char* names[]={"PLAINS","ISLAND","SWAMP","MOUNTAIN","FOREST"};for(int i=0;i<5;++i)add(36,215+55.0F*i,270,std::string(names[i])+" / 0.01",20+i);}
    if(page==ClubPage::Decks){add(580,118,160,std::string{"COLOR: "}+std::array<const char*,6>{"ANY","WHITE","BLUE","BLACK","RED","GREEN"}[colorFilter],31);add(750,118,200,std::string{"RARITY: "}+std::array<const char*,4>{"ANY","COMMON","UNCOMMON","RARE"}[rarityFilter],32);add(36,584,120,"PREV",33);add(166,584,120,"NEXT",34);add(620,220,260,"ADD SELECTED +",35);add(620,268,260,"REMOVE SELECTED -",36);add(620,330,120,"< DECK",37);add(760,330,120,"DECK >",38);add(620,386,120,"NEW",39);add(760,386,120,"DELETE",40);add(620,440,260,"SAVE DECK",41);add(620,488,260,"USE SAVED DECK",42);}
    if(page==ClubPage::Face){add(500,300,120,"BRUSH -",50);add(640,300,120,"BRUSH +",51);add(500,350,260,eraser?"ERASER ON":"PENCIL / ERASER",52);add(500,400,120,"UNDO",53);add(640,400,120,"REDO",54);add(500,450,260,"CLEAR CANVAS",55);add(500,510,260,"SAVE AND SHARE FACE",56);add(500,565,260,bucket?"BUCKET ON / PENCIL":"BUCKET FILL",57);}
    if(page==ClubPage::Match){add(50,220,330,v.match.ready[0]||v.match.ready[1]?"TOGGLE MY READY":"READY WITH SELECTED DECK",60);add(50,300,150,"LIFE -",61);add(230,300,150,"LIFE +",62);add(50,380,330,"VOTE: I WON",63);add(50,430,330,"VOTE: OTHER PLAYER WON",64);add(50,520,330,"CONFIRM MATCH RESET",65);}
    return b;
}
void ClubUi::click(float x,float y,const ClubView& v,const std::vector<CardDefinition>& defs,int side){const auto p=point(x,y);for(const auto& b:buttons(v))if(b.rect.contains(p.x,p.z)){action(b.action,v,defs,side);return;}
    if(page==ClubPage::Decks){naming=UiRect{620,165,610,38}.contains(p.x,p.z);if(UiRect{36,165,530,408}.contains(p.x,p.z)){auto list=filtered(v,defs);offset=std::clamp(offset,0,std::max(0,static_cast<int>(list.size())-12));const int i=offset+static_cast<int>((p.z-165)/34);if(i>=0&&i<static_cast<int>(list.size()))selected=list[i];}}
    if(page==ClubPage::Shop && UiRect{350,165,510,450}.contains(p.x,p.z)){const int row=static_cast<int>((p.z-165)/28);if(row<static_cast<int>(v.opened.size()))selected=v.opened[row];}
    if(page==ClubPage::Face){for(int i=0;i<16;++i)if(UiRect{500+static_cast<float>(i%8)*35,170+static_cast<float>(i/8)*45,30,35}.contains(p.x,p.z)){ink=i;eraser=false;return;}if(UiRect{36,160,400,400}.contains(p.x,p.z)){if(bucket){const int x=static_cast<int>((p.x-36)*256/400),y=static_cast<int>((p.z-160)*256/400);const auto old=canvas[static_cast<std::size_t>(y*256+x)],color=static_cast<std::uint8_t>(eraser?0:ink);if(old!=color){history();std::vector<int> todo{y*256+x};canvas[static_cast<std::size_t>(todo[0])]=color;while(!todo.empty()){const int at=todo.back();todo.pop_back();for(int next:{at-1,at+1,at-256,at+256})if(next>=0&&next<facePixels&&((next/256==at/256)||next==at-256||next==at+256)&&canvas[static_cast<std::size_t>(next)]==old){canvas[static_cast<std::size_t>(next)]=color;todo.push_back(next);}}}return;}before_=canvas;painting=true;last_={(p.x-36)*256/400,(p.z-160)*256/400};stamp(static_cast<int>(last_.x),static_cast<int>(last_.z));}}
}
void ClubUi::text(const std::string& t){if(page!=ClubPage::Decks)return;auto& s=naming?name:query;for(unsigned char c:t)if(c>=32&&c<127&&s.size()<(naming?32U:80U))s+=static_cast<char>(c);offset=0;}
void ClubUi::backspace(){if(page!=ClubPage::Decks)return;auto& s=naming?name:query;if(!s.empty())s.pop_back();offset=0;}
void ClubUi::wheel(float amount){if(page==ClubPage::Decks)offset=std::max(0,offset+(amount>0?-3:3));}
void ClubUi::stamp(int x,int y){for(int dy=-brush;dy<=brush;++dy)for(int dx=-brush;dx<=brush;++dx){const int px=x+dx,py=y+dy;if(dx*dx+dy*dy<=brush*brush&&px>=0&&py>=0&&px<256&&py<256)canvas[static_cast<std::size_t>(py*256+px)]=static_cast<std::uint8_t>(eraser?0:ink);}}
void ClubUi::stroke(Vec2 to){const int steps=std::max(1,static_cast<int>(std::hypot(to.x-last_.x,to.z-last_.z)));for(int i=0;i<=steps;++i){const float t=static_cast<float>(i)/steps;stamp(static_cast<int>(last_.x+(to.x-last_.x)*t),static_cast<int>(last_.z+(to.z-last_.z)*t));}last_=to;}
void ClubUi::move(float x,float y){if(!painting)return;const auto p=point(x,y);stroke({std::clamp((p.x-36)*256/400,0.0F,255.0F),std::clamp((p.z-160)*256/400,0.0F,255.0F)});}
void ClubUi::history(){undo.push_back(canvas);if(undo.size()>32)undo.erase(undo.begin());redo.clear();}
void ClubUi::up(){if(painting){if(canvas!=before_){undo.push_back(std::move(before_));if(undo.size()>32)undo.erase(undo.begin());redo.clear();}painting=false;}}
void ClubUi::undoPaint(bool forward){up();auto& from=forward?redo:undo;auto& to=forward?undo:redo;if(from.empty())return;to.push_back(canvas);canvas=std::move(from.back());from.pop_back();}
void ClubUi::action(int a,const ClubView& v,const std::vector<CardDefinition>& defs,int side){if(a>=1&&a<=4){show(static_cast<ClubPage>(a-1),v);if(page==ClubPage::Face)canvas=v.faces[side];return;}if(a==5){close();return;}ClubCommand c;bool dispatch=false;
    if(a==10){selected=-1;c.op=ClubOp::Pack;dispatch=true;}if(a>=20&&a<25){const std::string names[]={"Plains","Island","Swamp","Mountain","Forest"};for(std::size_t i=0;i<defs.size();++i)if(defs[i].basicLand&&defs[i].name==names[a-20]){selected=-1;c.op=ClubOp::Land;c.value=static_cast<int>(i);dispatch=true;break;}}
    if(a==31){colorFilter=(colorFilter+1)%6;offset=0;}if(a==32){rarityFilter=(rarityFilter+1)%4;offset=0;}if(a==33)offset=std::max(0,offset-12);if(a==34)offset+=12;
    if(a==35&&selected>=0&&selected<static_cast<int>(v.profile.owned.size())){if(draft.size()<120&&static_cast<unsigned>(std::count(draft.begin(),draft.end(),selected))<v.profile.owned[selected])draft.push_back(static_cast<std::uint16_t>(selected));else notice="NO AVAILABLE COPY OR DECK FULL";}
    if(a==36){auto it=std::find(draft.begin(),draft.end(),selected);if(it!=draft.end())draft.erase(it);}
    if(a==37||a==38){deckIndex=std::clamp(deckIndex+(a==37?-1:1),0,std::max(0,static_cast<int>(v.profile.decks.size())-1));loadDeck(v);}
    if(a==39){name="My Deck";draft.clear();naming=true;}if(a==40){c.op=ClubOp::DeleteDeck;c.name=name;dispatch=true;}if(a==41){c.op=ClubOp::SaveDeck;c.name=name;c.cards=draft;dispatch=true;}if(a==42){c.op=ClubOp::SelectDeck;c.name=name;dispatch=true;}
    if(a==50){brush=std::max(1,brush-1);}
    if(a==51){brush=std::min(20,brush+1);}
    if(a==52){eraser=!eraser;bucket=false;}
    if(a==53){undoPaint(false);}
    if(a==54){undoPaint(true);}
    if(a==55){up();history();canvas.assign(facePixels,0);}if(a==56){up();c.op=ClubOp::Face;c.face=canvas;dispatch=true;}
    if(a==57){bucket=!bucket;}
    if(a==60){c.op=ClubOp::Ready;dispatch=true;}if(a==61||a==62){c.op=ClubOp::Life;c.value=a==61?-1:1;dispatch=true;}if(a==63||a==64){c.op=ClubOp::Winner;c.value=a==63?side:1-side;dispatch=true;}if(a==65){c.op=ClubOp::Reset;dispatch=true;}
    if(dispatch&&send){notice.clear();send(std::move(c));}
}
}
