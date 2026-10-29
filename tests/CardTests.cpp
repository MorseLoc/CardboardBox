#include "cards/TabletopController.h"
#include "cards/CardArt.h"
#include "cards/CardImages.h"
#include "cards/CardCatalog.h"
#include <filesystem>
#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <set>
#include <stdexcept>

namespace {
void check(bool ok,const char* message) {if(!ok) throw std::runtime_error(message);}
fl::Ray ray(float x,float z) {return {{x,10,z},{0,-1,0}};}
void stateTests() {
    fl::Tabletop t{1234};check(t.valid() && t.cards().size()==120,"Initial cards invalid");
    check(t.pile(1)->cards.size()==60 && t.pile(2)->cards.size()==60,"Both complete decks required");
    const auto first=t.pile(1)->cards;const auto top=first.back();
    check(!t.visibleDefinition(top,0) && !t.visibleDefinition(top,1),"Hidden library identity exposed");
    check(t.draw(2,0,7)==0,"Cannot privately draw another side's hidden library");
    check(t.draw(1,0,7)==7 && t.hand(0).size()==7 && t.pile(1)->cards.size()==53,"Opening hand count wrong");
    check(t.card(top)->zone==fl::CardZone::Hand && t.visibleDefinition(top,0) && !t.visibleDefinition(top,1),"Hand visibility wrong");
    const auto hand=t.hand(0);const auto owner=t.card(hand.front())->owner;
    check(t.beginDrag(hand,0,{0,0}),"Hand drag failed");t.moveDrag({1,0});t.cancelDrag();
    check(t.valid() && t.hand(0).size()==7,"Canceled drag lost hand cards");
    check(t.beginDrag(hand,0,{0,0}),"Second hand drag failed");t.moveDrag({0.5F,0.5F});t.finishDrag();
    check(t.valid() && t.hand(0).empty(),"Playing cards invalidated state");
    check(t.card(hand.front())->owner==owner && t.visibleDefinition(hand.front(),1),"Playing changed owner or failed public reveal");
    const auto picked=t.pick(ray(t.card(hand.back())->position.x,t.card(hand.back())->position.z));
    check(picked.card!=0,"Ray did not select a card");
    t.tap(hand,0);check(t.card(hand.front())->tapped,"Tap failed");t.tap(hand,0);check(!t.card(hand.front())->tapped,"Untap failed");
    t.flip(hand,0);check(!t.visibleDefinition(hand.front(),0),"Face-down table card inspectable");t.flip(hand,0);
    t.rotate(hand,0,0.3F);check(t.card(hand.front())->angle>0.29F,"Rotation failed");
    const auto group=t.group(hand);check(group && t.pile(group)->cards==hand && t.valid(),"Group ordering/membership failed");
    const auto split=t.split(group);check(split && t.valid() && t.pile(group)->cards.size()+t.pile(split)->cards.size()==7,"Split duplicated/lost cards");
    check(t.beginDrag({},split,t.pile(split)->position),"Pile drag failed");t.moveDrag(t.pile(group)->position);t.finishDrag(group);
    check(t.valid() && t.pile(group)->cards.size()==7 && !t.pile(split),"Pile merge invalid");
    auto sorted=t.pile(group)->cards;std::sort(sorted.begin(),sorted.end());
    t.shuffle(group);auto shuffled=t.pile(group)->cards;std::sort(shuffled.begin(),shuffled.end());check(sorted==shuffled,"Shuffle changed membership");
    const auto order=t.pile(group)->cards;
    check(t.draw(group,0,2)==2 && t.hand(0).size()==2,"Public pile draw failed");
    const auto drawn=t.hand(0);check(t.toPile(drawn,1,true) && t.valid(),"Return to bottom failed");
    check(t.pile(1)->cards.front()==drawn.front(),"Bottom placement order wrong");
    check(t.toPile(drawn,1,false) && t.valid(),"Moving within a pile duplicated cards");
    check(t.pile(1)->cards.back()==drawn.back(),"Top placement order wrong");
    check(t.draw(1,0,1000)==55,"Draw past end should clamp to library size");
    check(t.draw(1,0,1)==0 && t.pile(1)->cards.empty(),"Empty library draw failed");
    check(t.toPile(t.hand(0),1,false) && t.valid(),"Empty library could not accept returned cards");
    check(t.cards().size()==120,"Card operations changed total instance count");
    (void)order;
    // Partial edge overlap must also produce a physical stack, not z-fighting.
    auto single=t.pile(1)->cards.back();t.draw(1,0,1);t.beginDrag({&single,1},0,{0,0});t.moveDrag({0,0});t.finishDrag();
    auto second=t.pile(1)->cards.back();t.draw(1,0,1);t.beginDrag({&second,1},0,{0,0});t.moveDrag({0.25F,0});t.finishDrag();
    check(t.card(second)->height>t.card(single)->height,"Edge-overlap stacking failed");
    check(t.pick(ray(0.13F,0)).card==second,"Stack picking failed to select the upper card");
    check(t.valid(),"Stacking invalidated membership");
}
void pickingAndControllerTests() {
    fl::Tabletop t{42};fl::TabletopController ui{t};ui.setViewer(0);ui.setViewport(1280,720);
    check(!fl::onPlane({{0,3,0},{1,0,0}},1),"Parallel ray should not hit");
    check(!fl::onPlane({{0,3,0},{0,1,0}},1),"Backwards plane intersection accepted");
    const auto p=t.pile(1)->position;
    ui.leftDown(300,300,ray(p.x,p.z),false,true);
    check(t.hand(0).size()==1,"Double click did not draw");
    const auto cid=t.hand(0).front();
    ui.rightDown(70,580,ray(0,0),false);check(ui.inspected()==cid,"Own hand inspection failed");
    check(ui.escape() && !ui.inspected(),"Inspection close failed");
    ui.leftDown(70,580,ray(0,2),false,false);ui.pointerMove(640,300,ray(0,0));ui.leftUp(640,300,ray(0,0),false);
    check(t.card(cid)->zone==fl::CardZone::Table && t.valid(),"Hand-to-table mouse drag failed");
    ui.action(fl::TableAction::Flip);ui.rightDown(640,300,ray(0,0),false);
    check(!ui.inspected() && ui.menuOpen(),"Hidden card opened identity inspection");ui.escape();
    ui.action(fl::TableAction::Flip);ui.rightDown(640,300,ray(0,0),false);check(ui.inspected()==cid,"Public inspection failed");
    ui.setViewer(1);check(!ui.inspected() && ui.selected().empty(),"Seat switch retained private UI state");
    check(t.beginDrag({&cid,1},0,{0,0}),"Table drag start failed");t.moveDrag({0.1F,0});t.cancelDrag();
    t.rotate({&cid,1},0,1.57079633F);
    check(t.pick(ray(0.18F,0)).card==cid,"Rotated rectangle picking failed");
    check(t.pick(ray(0,0.18F)).card!=cid,"Rotated picking used unrotated bounds");
    check(t.valid(),"Controller operations broke invariants");
}
void oppositeSeatTest() {
    fl::Tabletop t{99};t.draw(2,1,1);const auto id=t.hand(1).front();
    check(t.beginDrag({&id,1},0,{0,0}),"Opposite hand drag failed");t.finishDrag();
    check(std::abs(t.card(id)->angle-3.14159265F)<0.001F,"Opposite side plays cards upside-down");
    check(t.card(id)->owner==1 && t.visibleDefinition(id,0) && t.valid(),"Opposite-side play changed owner/visibility");
}
void emptyPileTests() {
    fl::Tabletop t{7};
    const auto makePile=[&](std::size_t count) {
        check(t.draw(1,0,count)==count,"Could not prepare pile regression");
        const auto ids=t.hand(0);check(t.beginDrag(ids,0,{0,0}),"Could not play regression cards");
        t.moveDrag({0,0});t.finishDrag();
        const auto pid=t.group(ids);check(pid!=0,"Could not group regression cards");return pid;
    };
    auto pid=makePile(1);
    check(t.pick(ray(0,0)).pile==pid,"Nonempty pile must remain selectable");
    fl::TabletopController ui{t};ui.setViewer(0);
    ui.leftDown(640,300,ray(0,0),false,false);ui.leftUp(640,300,ray(0,0),false);
    ui.action(fl::TableAction::Draw);
    check(!t.pile(pid) && !ui.selectedPile(),"Last draw left an empty pile or stale selection");
    check(!t.pick(ray(0,0)).pile,"Deleted pile is still selectable");
    auto ids=t.hand(0);check(t.toPile(ids,1,false),"Could not reset regression hand");
    pid=makePile(3);ids=t.pile(pid)->cards;
    check(t.toHand(ids,0) && !t.pile(pid),"Moving all cards to hand left a ghost pile");
    ids=t.hand(0);t.toPile(ids,1,false);
    pid=makePile(2);ids=t.pile(pid)->cards;
    check(t.toPile(ids,pid,false) && t.pile(pid)->cards.size()==2,"Same-pile return deleted its destination midway");
    check(t.toPile(ids,1,false) && !t.pile(pid),"Returning all cards to library left a ghost pile");
    const auto position=t.pile(1)->position;
    check(t.draw(1,0,60)==60 && t.library(0)==1,"Empty library lost its return address");
    check(!t.pick(ray(position.x,position.z)).pile,"Empty library still has a clickable placeholder");
    check(!t.beginDrag({},1,position),"Invisible empty library can still be dragged");
    const auto first=t.hand(0).front();check(t.toPile({&first,1},1,false),"Could not return a card to empty library");
    check(t.pick(ray(position.x,position.z)).pile==1,"Refilled library did not reappear for picking");
    check(t.cards().size()==120 && t.valid(),"Empty-pile cleanup lost card instances");
}
void importedCatalogTests() {
    fl::Tabletop t{11};const auto& definitions=t.definitions();
    check(definitions.size()==302,"Unlimited import lost printings");
    std::set<std::string> ids,printingIds;int basics=0,commons=0,uncommons=0,rares=0;
    bool foundChaosOrb=false,foundLotus=false;
    for(const auto& d:definitions) {
        check(d.setId=="2ed" && !d.name.empty(),"Wrong/missing set metadata");
        check(ids.insert(d.id).second && printingIds.insert(d.scryfallId).second,"Duplicate imported IDs");
        basics+=d.basicLand ? 1:0;commons+=d.rarity=="common" ? 1:0;uncommons+=d.rarity=="uncommon" ? 1:0;rares+=d.rarity=="rare" ? 1:0;
        foundChaosOrb|=d.name=="Chaos Orb";foundLotus|=d.name=="Black Lotus";
    }
    check(basics==15 && commons==90 && uncommons==95 && rares==117,"Imported rarity/basic-land counts differ from source");
    check(foundChaosOrb && foundLotus,"Required Unlimited cards missing");
    for(int side:{0,1}) {
        const auto deck=fl::unlimitedDemoDeck(definitions,side);check(deck.size()==60,"Imported test deck has wrong size");
        int lands=0;for(auto index:deck) {check(index<definitions.size(),"Invalid deck definition index");lands+=definitions[index].basicLand?1:0;}
        check(lands==24,"Imported test deck lost its basic lands");
    }
}
void imageFileTests() {
    const std::filesystem::path root{FL_TEST_DATA_DIR};
    for(const auto* name:{"card-test.jpg","card-test.png"}) {
        const auto image=fl::loadCardImage(root/name);
        check(image && image->width==32 && image->height==48,"JPEG/PNG decode failed");
        check(image->rgba[0]>240 && image->rgba[2]<20,"Image top row was flipped or recolored");
        const auto last=image->rgba.size()-4;check(image->rgba[last]<20 && image->rgba[last+2]>240,"Image bottom row wrong");
        const auto small=fl::resizeCardImage(*image,16,24);
        check(small.rgba.size()==16*24*4 && small.rgba[0]>240,"Small texture generation failed");
    }
    check(!fl::loadCardImage(root/"bad-image.jpg"),"Corrupt image accepted");
    check(!fl::loadCardImage(root/"not-present.png"),"Missing image accepted");
}
void images() {
    fl::Tabletop t{1};
    for(const auto& definition:t.definitions()) {
        const auto image=fl::makeCardImage(&definition);
        check(image.rgba.size()==static_cast<std::size_t>(image.width*image.height*4),"Invalid texture size");
        check(std::all_of(image.rgba.begin()+3,image.rgba.begin()+4,[](auto value){return value==255;}),"Invalid alpha");
    }
    const auto back=fl::makeCardImage(nullptr);check(!back.rgba.empty(),"Card back missing");
}
}
int main() {
    try {stateTests();pickingAndControllerTests();oppositeSeatTest();emptyPileTests();importedCatalogTests();imageFileTests();images();std::cout<<"PASS: 120-card conservation, ordered draws/returns, visibility, tap/flip/rotate, drag cancel, split/merge/shuffle, rotated ray picking, hand/controller input and generated images\n";return EXIT_SUCCESS;}
    catch(const std::exception& e) {std::cerr<<"FAIL: "<<e.what()<<'\n';return EXIT_FAILURE;}
}
