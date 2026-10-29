#include "networking/NetworkRoom.h"
#include "networking/PawnSmoother.h"
#include "cards/TabletopController.h"
#include <chrono>
#include <cmath>
#include <filesystem>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <thread>
using namespace fl;
using namespace fl::net;
namespace {
void check(bool ok,const char* message) {if(!ok) throw std::runtime_error(message);}
TableCommand cmd(CardOp op,PileId pile=0,std::vector<CardId> cards={}) {TableCommand c;c.op=op;c.pile=pile;c.cards=std::move(cards);return c;}
void protocol() {
    for(int op=1;op<=14;++op) {
        auto c=cmd(static_cast<CardOp>(op),2,{3,4});c.target=1;c.point={1,-2};c.rotation=0.2F;c.count=7;c.bottom=true;
        const auto bytes=encodeCommand(c);const auto read=decodeCommand(bytes);
        check(read && encodeCommand(*read)==bytes,"Command round trip");
        for(std::size_t n=0;n<bytes.size();++n) check(!decodeCommand({bytes.data(),n}),"Truncated command accepted");
        auto extra=bytes;extra.push_back(0);check(!decodeCommand(extra),"Trailing command accepted");
    }
    auto bad=cmd(CardOp::Move);bad.point.x=std::numeric_limits<float>::infinity();
    check(!decodeCommand(encodeCommand(bad)),"Infinite drag accepted");
    Tabletop table{7};table.draw(1,0,7);table.draw(2,1,7);
    for(int side=0;side<2;++side) {
        auto view=table.viewFor(side);const auto bytes=encodeView(view);check(bytes.size()<maxPacketBytes-16,"Snapshot exceeds packet cap");
        const auto read=decodeView(bytes);Tabletop display;
        check(read && display.applyView(*read) && display.valid(),"Filtered view round trip");
        check(display.hand(side).size()==7 && display.hand(1-side).size()==7,"Private hand counts lost");
        for(auto id:display.hand(side)) check(display.visibleDefinition(id,side).has_value(),"Own hand missing identity");
        for(auto id:display.hand(1-side)) {
            check(!table.card(id) && display.card(id)->definition==hiddenDefinition,"Opponent hand identity leaked");
            check(!display.visibleDefinition(id,side),"Opponent inspection allowed");
        }
        for(const auto& [id,p]:display.piles()) for(auto cid:p.cards) {
            check(!table.card(cid) && display.card(cid)->definition==hiddenDefinition,"Hidden pile ID/order leaked");
        }
        for(std::size_t n=0;n<bytes.size();++n) check(!decodeView({bytes.data(),n}),"Truncated view accepted");
        auto corrupt=view;corrupt.cards[1].id=corrupt.cards[0].id;check(!display.applyView(corrupt),"Duplicate view ID accepted");
        corrupt=view;corrupt.cards[0].definition=9999;check(!display.applyView(corrupt),"Bad definition index accepted");
        Message m;m.kind=Kind::TableState;m.sequence=2;m.payload=bytes;
        const auto wire=encode(m);auto decoded=decode(wire);check(decoded && decoded->payload==bytes,"Wire snapshot changed payload");
    }
    // Anonymous slots reveal nothing about a hidden shuffle.
    const auto before=encodeView(table.viewFor(1));table.shuffle(1);
    check(encodeView(table.viewFor(1))==before,"Hidden shuffle transmitted ordered copy IDs");
}
void authority() {
    TableSession session;double now=0;
    auto apply=[&](int side,TableCommand c,bool seated=true){now+=0.01;return session.apply(side,c,seated,now);};
    check(apply(0,cmd(CardOp::Draw,1),false)==CardResult::NotSeated,"Unseated draw accepted");
    check(apply(1,cmd(CardOp::Draw,1))==CardResult::Private,"Opponent library draw accepted");
    auto draw=cmd(CardOp::Draw,1);draw.count=7;check(apply(0,draw)==CardResult::Applied,"Draw seven failed");
    auto cards=session.table.hand(0);check(cards.size()==7,"Wrong draw count");
    check(apply(1,cmd(CardOp::Hand,0,{cards[0]}))==CardResult::Private,"Stole opponent hand card");
    check(apply(0,cmd(CardOp::Begin,0,{cards[0]}))==CardResult::Applied,"Begin drag failed");
    check(apply(1,cmd(CardOp::Draw,2))==CardResult::Applied,"Unrelated library was locked");
    check(apply(1,cmd(CardOp::Begin,0,{cards[0]}))==CardResult::Busy,"Same-card grab was not locked");
    auto move=cmd(CardOp::Move);move.point={1,1};apply(0,move);
    check(session.table.card(cards[0])->zone==CardZone::Table,"Dragging hand card did not reveal");
    check(session.release(0),"Disconnect release failed");
    check(session.table.hand(0).size()==7,"Canceled drag lost hand card");
    apply(0,cmd(CardOp::Begin,0,{cards[0]}));move.point={1,0};apply(0,move);apply(0,cmd(CardOp::Finish));
    check(session.table.hand(0).size()==6,"Play did not remove from hand");
    auto publicView=session.table.viewFor(1);Tabletop display;check(display.applyView(publicView),"Public view invalid");
    check(display.visibleDefinition(cards[0],1).has_value(),"Played card remained hidden");
    apply(1,cmd(CardOp::Tap,0,{cards[0]}));check(session.table.card(cards[0])->tapped,"Tap failed");
    auto rotate=cmd(CardOp::Rotate,0,{cards[0]});rotate.rotation=0.261799388F;apply(0,rotate);
    check(session.table.card(cards[0])->angle>0.26F,"Rotate failed");
    apply(0,cmd(CardOp::Flip,0,{cards[0]}));check(!session.table.visibleDefinition(cards[0],1),"Hidden table front exposed");
    check(display.applyView(session.table.viewFor(1)) && display.card(cards[0])->definition==hiddenDefinition,"Hidden table snapshot leaked identity");
    apply(0,cmd(CardOp::Flip,0,{cards[0]}));
    apply(0,cmd(CardOp::Begin,0,{cards[1]}));apply(0,cmd(CardOp::Finish));
    apply(0,cmd(CardOp::Group,0,{cards[0],cards[1]}));check(session.table.piles().size()==3,"Group failed");
    check(apply(1,cmd(CardOp::Split,3))==CardResult::Applied,"Public pile split failed");
    check(session.table.piles().size()==4,"Split wrong pile count");
    apply(0,cmd(CardOp::Begin,4));auto finish=cmd(CardOp::Finish);finish.target=3;apply(0,finish);
    check(!session.table.pile(4) && session.table.pile(3)->cards.size()==2,"Merge or empty pile cleanup failed");
    apply(1,cmd(CardOp::Draw,3));apply(1,cmd(CardOp::Draw,3));check(!session.table.pile(3),"Empty pile persists");
    apply(0,cmd(CardOp::Return,0,{cards[2]}));check(session.table.pile(1)->cards.size()==54,"Library return failed");
    apply(0,cmd(CardOp::Begin,0,{cards[3]}));check(session.expire(now+4) && session.table.card(cards[3])->zone==CardZone::Hand,"Lease expiry lost card");
    for(int i=0;i<200;++i) {
        auto d=cmd(CardOp::Draw,1);apply(0,d);
        auto hand=session.table.hand(0);
        if(!hand.empty()) {auto ret=cmd(CardOp::Return,0,{hand.back()});ret.bottom=i%2;apply(0,ret);}
        apply(0,cmd(CardOp::Shuffle,1));check(session.table.valid() && session.table.cards().size()==120,"Conservation failed");
    }
}
void simultaneousGrabs() {
    TableSession session;double now=0;
    const auto apply=[&](int side,TableCommand c){return session.apply(side,c,true,now+=0.01);};
    session.table.draw(1,0,4);session.table.draw(2,1,4);
    const auto a=session.table.hand(0),b=session.table.hand(1);
    check(apply(0,cmd(CardOp::Begin,0,{a[0]}))==CardResult::Applied,"Host grab failed");
    check(apply(1,cmd(CardOp::Begin,0,{b[0]}))==CardResult::Applied,"Guest simultaneous grab failed");
    auto move=cmd(CardOp::Move);move.point={-1,0.5F};apply(0,move);move.point={1,-0.5F};apply(1,move);
    check(session.holding(0) && session.holding(1),"Leases were not independent");
    check(session.table.card(a[0])->position.x<0 && session.table.card(b[0])->position.x>0,"Concurrent movement overwrote other card");
    apply(0,cmd(CardOp::Finish));
    check(!session.holding(0) && session.holding(1),"Finishing host released guest grab");
    check(apply(0,cmd(CardOp::Tap,0,{b[0]}))==CardResult::Busy,"Held card tapped by other player");
    check(apply(0,cmd(CardOp::Group,0,{a[0],b[0]}))==CardResult::Busy,"Held card grouped by other player");
    check(apply(0,cmd(CardOp::Tap,0,{a[0]}))==CardResult::Applied,"Unrelated tap blocked");
    apply(1,cmd(CardOp::Finish));
    // Grouped drags lock every member, not just the top card.
    apply(0,cmd(CardOp::Begin,0,{a[0],a[1]}));apply(1,cmd(CardOp::Begin,0,{b[0],b[1]}));
    apply(0,cmd(CardOp::Cancel));check(session.holding(1) && !session.holding(0),"Cancel released both leases");
    check(session.table.card(a[1])->zone==CardZone::Hand,"Canceled group did not restore hand");
    apply(1,cmd(CardOp::Finish));apply(0,cmd(CardOp::Group,0,{a[0]}));apply(1,cmd(CardOp::Group,0,{b[0],b[1]}));
    // Public pile 3 and pile 4 may be moved independently.
    check(apply(0,cmd(CardOp::Begin,3))==CardResult::Applied,"Pile 3 grab failed");
    check(apply(1,cmd(CardOp::Split,3))==CardResult::Busy,"Held pile split accepted");
    check(apply(1,cmd(CardOp::Shuffle,3))==CardResult::Busy,"Held pile shuffled");
    check(apply(1,cmd(CardOp::Draw,3))==CardResult::Busy,"Held pile draw accepted");
    check(apply(1,cmd(CardOp::Begin,4))==CardResult::Applied,"Independent pile 4 blocked");
    auto finish=cmd(CardOp::Finish);finish.target=3;apply(1,finish);
    check(session.table.pile(3)->cards.size()==1 && session.table.pile(4)->cards.size()==2,"Merge mutated a held target");
    check(!session.holding(1) && session.holding(0),"Rejected merge left stale lease");
    session.release(0);
    // A disconnected player must not cancel the other player's drag.
    apply(0,cmd(CardOp::Begin,3));apply(1,cmd(CardOp::Begin,4));session.release(1);
    check(session.holding(0) && !session.holding(1),"Disconnect canceled host grab");
    check(session.expire(now+4) && !session.holding(0),"Independent expiry failed");
    // A floating card cannot become permanent support for the other drop.
    apply(0,cmd(CardOp::Begin,0,{a[2]}));apply(1,cmd(CardOp::Begin,0,{b[2]}));
    apply(0,cmd(CardOp::Finish));check(session.table.card(a[2])->height<layout::cardCenterY+0.02F,"Drop stacked on another held card");
    apply(1,cmd(CardOp::Cancel));
    for(int i=0;i<150;++i) {
        apply(0,cmd(CardOp::Begin,0,{a[2]}));apply(1,cmd(CardOp::Begin,0,{b[2]}));
        move.point={-1.5F,0.7F};apply(0,move);move.point={1.5F,-0.7F};apply(1,move);
        apply(0,cmd(i%2?CardOp::Cancel:CardOp::Finish));apply(1,cmd(i%2?CardOp::Finish:CardOp::Cancel));
        check(session.table.valid() && session.table.cards().size()==120,"Concurrent drags lost cards");
    }
}
void networkControls() {
    TableSession server;Tabletop display;server.table.draw(1,0,2);display.applyView(server.table.viewFor(0));
    TabletopController ui{display};ui.setViewport(1280,720);ui.setViewer(0);double now=0;
    ui.setCommandSink([&](const TableCommand& c){
        const auto result=server.apply(0,c,true,now+=0.01);check(result==CardResult::Applied,"Network mouse request rejected");
        check(display.applyView(server.table.viewFor(0)),"Controller view sync failed");
    });
    const auto hand=display.hand(0);const CardId card=hand.front();
    // Hand-click ray is deliberately outside the felt. Beginning the drag
    // must use the cursor's first tabletop point instead of that old ray.
    ui.leftDown(65,580,Ray{{0,10,8},{0,-1,0}},false,false);
    ui.pointerMove(500,300,Ray{{0,10,0},{0,-1,0}});
    check(ui.dragging(),"Network mouse drag did not start");
    ui.leftUp(500,300,Ray{{0,10,0},{0,-1,0}},false);
    check(server.table.card(card)->zone==CardZone::Table && !ui.dragging(),"Network hand-to-table drag failed");
    ui.action(TableAction::Tap);check(server.table.card(card)->tapped,"Network key action failed");
    ui.action(TableAction::Group);ui.refreshSelection();check(ui.selectedPile()==3,"Network grouping lost selection");
    ui.action(TableAction::Draw);ui.refreshSelection();check(!server.table.pile(3) && !ui.selectedPile(),"Network empty pile persists");
    check(server.table.valid() && server.table.cards().size()==120,"Network controls lost cards");
}
void smoothing() {
    PawnSmoother smoother;PawnState target{{0,0},0,-1,true};float last=0,maxJump=0;int moving=0;
    for(int frame=0;frame<180;++frame) {
        const auto tick=static_cast<std::uint64_t>((frame/3)*3);target.position.x=static_cast<float>(tick)/60;
        const auto shown=smoother.update(target,tick,1.0F/60);
        if(frame>30) {const float delta=shown.position.x-last;maxJump=std::max(maxJump,std::abs(delta));if(delta>0.001F) ++moving;}
        last=shown.position.x;
    }
    check(maxJump<0.025F && moving>140,"20Hz snapshots did not interpolate at 60fps");
    target.seat=1;target.position={0,-layout::seatZ};const auto seated=smoother.update(target,183,1.0F/60);
    check(seated.seat==1 && distanceSquared(seated.position,target.position)==0,"Seat transition interpolated through furniture");
    target.seat=-1;target.position={8,8};check(distanceSquared(smoother.update(target,186,1.0F/60).position,target.position)==0,"Teleport did not snap");
}
void pump(NetworkRoom& host,NetworkRoom& guest,int n,MovementInput a={},MovementInput b={}) {
    for(int i=0;i<n;++i) {host.update(0.01F,a);guest.update(0.01F,b);std::this_thread::sleep_for(std::chrono::milliseconds(1));}
}
std::uint16_t hostAt(NetworkRoom& host) {for(std::uint16_t p=35645;p<35665;++p) if(host.host(p)) return p;throw std::runtime_error("No free test port");}
void retries() {
    NetworkRoom host;const auto port=hostAt(host);
    ENetHost* transport=enet_host_create(nullptr,1,2,0,0);check(transport,"Replay probe create failed");
    ENetAddress address{};enet_address_set_host(&address,"127.0.0.1");address.port=port;
    ENetPeer* peer=enet_host_connect(transport,&address,2,0);check(peer,"Replay probe connect failed");
    const auto send=[&](const Message& m,bool reliable=true) {
        const auto bytes=encode(m);enet_peer_send(peer,reliable?0:1,enet_packet_create(bytes.data(),bytes.size(),reliable?ENET_PACKET_FLAG_RELIABLE:0));
    };
    bool welcomed=false;unsigned inputSequence=0;
    const auto run=[&](int count,MovementInput input={}) {
        for(int i=0;i<count;++i) {
            host.update(0.01F);ENetEvent e{};
            while(enet_host_service(transport,&e,0)>0) {
                if(e.type==ENET_EVENT_TYPE_CONNECT) {Message hello;hello.catalog=catalogFingerprint();send(hello);}
                if(e.type==ENET_EVENT_TYPE_RECEIVE) {const auto m=decode({e.packet->data,e.packet->dataLength});if(m && m->kind==Kind::Welcome) welcomed=true;enet_packet_destroy(e.packet);}
            }
            if(welcomed && i%5==0) {Message m;m.kind=Kind::Input;m.sequence=++inputSequence;m.input=input;send(m,false);}
            enet_host_flush(transport);std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    };
    run(100);check(welcomed,"Replay probe handshake failed");run(20,{-1,0,0,false});run(45,{0,-1,0,false});run(20);
    Message sit;sit.kind=Kind::Interact;send(sit);run(20);check(host.state().pawns[1].seat==1,"Replay probe seat failed");
    Message draw;draw.kind=Kind::CardCommand;draw.sequence=1;draw.payload=encodeCommand(cmd(CardOp::Draw,2));
    send(draw);send(draw);send(draw);run(30);Tabletop view;host.syncTable(view);
    check(view.hand(1).size()==1,"Repeated request drew duplicate cards");
    draw.sequence=2;send(draw);run(20);host.syncTable(view);check(view.hand(1).size()==2,"Fresh request incorrectly deduplicated");
    enet_peer_disconnect_now(peer,0);enet_host_destroy(transport);host.stop();
}
void loopback() {
    const auto directory=std::filesystem::temp_directory_path()/("clubhouse-resume-test-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    NetworkRoom host,guest;guest.setResumeDirectory(directory);const auto port=hostAt(host);check(guest.join("127.0.0.1",port),"Join failed");pump(host,guest,100);
    Tabletop hostView,guestView;host.syncTable(hostView);guest.syncTable(guestView);
    check(hostView.valid() && guestView.valid(),"Initial UDP table sync failed");
    // Walk to each assigned chair, with host-authoritative collision and distance checks.
    pump(host,guest,20,{1,0,0,false},{-1,0,0,false});
    pump(host,guest,45,{0,1,0,false},{0,-1,0,false});pump(host,guest,20);
    host.interact();guest.interact();pump(host,guest,20);
    check(host.state().pawns[0].seat==0 && host.state().pawns[1].seat==1,"Test seating failed");
    auto draw=cmd(CardOp::Draw);draw.count=7;host.command(draw);guest.command(draw);pump(host,guest,30);
    host.syncTable(hostView);guest.syncTable(guestView);
    check(hostView.hand(0).size()==7 && guestView.hand(1).size()==7,"UDP draw failed");
    check(hostView.hand(1).size()==7 && guestView.hand(0).size()==7,"UDP other hand count wrong");
    for(auto id:guestView.hand(0)) check(guestView.card(id)->definition==hiddenDefinition,"UDP hand privacy failed");
    const CardId played=guestView.hand(1).front();guest.command(cmd(CardOp::Begin,0,{played}));
    auto move=cmd(CardOp::Move);move.point={0.5F,-0.4F};guest.command(move);guest.command(cmd(CardOp::Finish));pump(host,guest,30);
    host.syncTable(hostView);guest.syncTable(guestView);
    check(hostView.card(played) && hostView.visibleDefinition(played,0).has_value(),"UDP play/reveal failed");
    host.command(cmd(CardOp::Tap,0,{played}));pump(host,guest,20);guest.syncTable(guestView);check(guestView.card(played)->tapped,"UDP tap failed");
    host.syncTable(hostView);guest.syncTable(guestView);
    const auto hostHeld=hostView.hand(0).front(),guestHeld=guestView.hand(1).front();
    host.command(cmd(CardOp::Begin,0,{hostHeld}));guest.command(cmd(CardOp::Begin,0,{guestHeld}));pump(host,guest,20);
    move.point={-1,0.5F};host.command(move);move.point={1,-0.5F};guest.command(move);pump(host,guest,20);
    host.syncTable(hostView);guest.syncTable(guestView);
    check(hostView.card(hostHeld)->position.x<0 && guestView.card(guestHeld)->position.x>0,"UDP concurrent grabs failed");
    host.command(cmd(CardOp::Cancel));pump(host,guest,20);guest.command(cmd(CardOp::Cancel));pump(host,guest,20);guest.syncTable(guestView);
    // A held card survives a quiet stationary grab due to heartbeat renewal.
    const auto held=guestView.hand(1).front();guest.command(cmd(CardOp::Begin,0,{held}));pump(host,guest,350);
    host.command(cmd(CardOp::Draw,1));check(host.lastCardResult()==CardResult::Applied,"Unrelated draw was blocked");
    host.command(cmd(CardOp::Begin,0,{held}));check(host.lastCardResult()==CardResult::Busy,"Quiet grab lease expired despite keepalive");
    guest.reconnect();pump(host,guest,200);guest.syncTable(guestView);host.syncTable(hostView);
    check(guest.ready() && guestView.hand(1).size()==6,"Reconnect did not roll back in-progress grab");
    check(guestView.cards().size()==120 && guestView.valid(),"Reconnect duplicated cards");
    guest.stop();pump(host,guest,20);
    // A new process restores the same reserved slot and gets the host's command sequence.
    NetworkRoom restarted;restarted.setResumeDirectory(directory);check(restarted.join("127.0.0.1",port),"Restart join failed");pump(host,restarted,150);
    check(restarted.ready(),"Persisted resume token failed");restarted.syncTable(guestView);check(guestView.hand(1).size()==6,"Guest restart lost hand");
    restarted.command(cmd(CardOp::Draw,2));pump(host,restarted,20);restarted.syncTable(guestView);
    check(guestView.hand(1).size()==7,"Restart command sequence was ignored");
    // Fresh host accepts the stale cached token only after a clean retry.
    host.stop();restarted.stop();check(host.host(port),"Reopen host failed");check(restarted.join("127.0.0.1",port),"New room join failed");pump(host,restarted,300);
    check(restarted.ready(),"Stale resume token prevented joining fresh host");
    restarted.stop();host.stop();std::filesystem::remove_all(directory);
}
}
int main() {
    try {protocol();authority();simultaneousGrabs();networkControls();smoothing();retries();loopback();std::cout<<"PASS: filtered wire privacy, strict decoding, host permissions, card conservation, independent object locks/expiry, simultaneous UDP drags, all table operations, 60fps interpolation, UDP shared cards, reconnect rollback, process restart resume\n";return 0;}
    catch(const std::exception& e) {std::cerr<<"FAIL: "<<e.what()<<'\n';return 1;}
}
