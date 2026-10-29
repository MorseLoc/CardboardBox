#include "networking/NetworkRoom.h"
#include <chrono>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <thread>

namespace {
using namespace fl;
using namespace fl::net;
void check(bool ok,const char* message) {if(!ok) throw std::runtime_error(message);}
void protocolTests() {
    for(int kind=1;kind<=7;++kind) {
        Message m;m.kind=static_cast<Kind>(kind);m.catalog=catalogFingerprint();m.token={1,2,3,4};
        m.input={-1,1,0.75F,true};m.state.tick=157;m.state.pawns[0]={{1,6},2,0,true};
        const auto bytes=encode(m);const auto decoded=decode(bytes);
        check(decoded.has_value(),"Protocol round trip failed");
        check(encode(*decoded)==bytes,"Protocol round trip changed data");
        for(std::size_t n=0;n<bytes.size();++n) check(!decode({bytes.data(),n}),"Truncated packet accepted");
        auto trailing=bytes;trailing.push_back(0);check(!decode(trailing),"Trailing payload accepted");
    }
    Message m;m.kind=Kind::Input;m.input.yaw=std::numeric_limits<float>::quiet_NaN();
    check(!decode(encode(m)),"NaN accepted");m.input.yaw=0;m.input.forward=3;
    check(!decode(encode(m)),"Unbounded movement input accepted");
    m.input.forward=0;auto b=encode(m);b[4]=99;check(!decode(b),"Wrong protocol version accepted");
    check(catalogFingerprint()!=0,"Empty content fingerprint");
    Message state;state.kind=Kind::State;
    check(encode(state).size()==43,"Room snapshot unexpectedly includes additional state");
}
void pump(NetworkRoom& host,NetworkRoom& guest,int n,MovementInput hostInput={},MovementInput guestInput={}) {
    for(int i=0;i<n;++i) {host.update(0.01F,hostInput);guest.update(0.01F,guestInput);std::this_thread::sleep_for(std::chrono::milliseconds(1));}
}
std::uint16_t startHost(NetworkRoom& host) {
    for(std::uint16_t port=35445;port<35465;++port) if(host.host(port)) return port;
    throw std::runtime_error("No free test UDP port 35445-35464");
}
void rejectedHandshake(NetworkRoom& host,std::uint16_t port,Message hello,Rejection expected) {
    ENetHost* transport=enet_host_create(nullptr,1,2,0,0);check(transport!=nullptr,"Probe creation failed");
    ENetAddress address{};enet_address_set_host(&address,"127.0.0.1");address.port=port;
    ENetPeer* peer=enet_host_connect(transport,&address,2,0);check(peer!=nullptr,"Probe connect failed");
    bool rejected=false;
    for(int n=0;n<300 && !rejected;++n) {
        host.update(0.01F);ENetEvent e{};
        while(enet_host_service(transport,&e,0)>0) {
            if(e.type==ENET_EVENT_TYPE_CONNECT) {
                const auto bytes=encode(hello);enet_peer_send(peer,0,enet_packet_create(bytes.data(),bytes.size(),ENET_PACKET_FLAG_RELIABLE));
            }
            if(e.type==ENET_EVENT_TYPE_RECEIVE) {
                const auto m=decode({e.packet->data,e.packet->dataLength});
                rejected=m && m->kind==Kind::Reject && m->rejection==expected;enet_packet_destroy(e.packet);
            }
        }
        enet_host_flush(transport);std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    enet_peer_disconnect_now(peer,0);enet_host_destroy(transport);
    check(rejected,"Handshake was not rejected with expected reason");
}
void loopbackTests() {
    NetworkRoom host,guest;const auto port=startHost(host);
    NetworkRoom conflict;check(!conflict.host(port),"Two hosts bound the same UDP port");
    Message badBuild;badBuild.build=0;badBuild.catalog=catalogFingerprint();
    rejectedHandshake(host,port,badBuild,Rejection::Version);
    Message badContent;badContent.catalog=0;
    rejectedHandshake(host,port,badContent,Rejection::Content);
    check(guest.join("127.0.0.1",port),"Client creation failed");
    pump(host,guest,150);
    check(guest.ready() && host.state().pawns[1].connected,"Loopback handshake failed");
    check(guest.localSide()==1 && host.localSide()==0,"Player sides not stable");
    check(distanceSquared(host.state().pawns[1].position,guest.state().pawns[1].position)<0.001F,"Initial snapshots differ");
    const auto before=host.state().pawns[1].position;
    pump(host,guest,30,{},MovementInput{1,0,0,false});pump(host,guest,20);
    const auto after=host.state().pawns[1].position;
    check(after.x>before.x+0.5F,"Guest movement input not applied by host");
    check(distanceSquared(after,guest.state().pawns[1].position)<0.001F,"Guest did not receive authoritative position");
    // Long movement against the exterior wall must stay inside collision bounds.
    pump(host,guest,600,{},MovementInput{1,0,0,true});pump(host,guest,20);
    check(host.state().pawns[1].position.x<layout::roomHalfWidth-0.4F,"Remote player escaped room collision");
    Message intruder;intruder.catalog=catalogFingerprint();
    rejectedHandshake(host,port,intruder,Rejection::Full);
    check(host.state().pawns[1].connected,"Third-player rejection displaced real guest");
    const auto saved=host.state().pawns[1].position;
    guest.reconnect();pump(host,guest,200);
    check(guest.ready() && host.state().pawns[1].connected,"Resume handshake failed");
    check(distanceSquared(saved,host.state().pawns[1].position)<0.001F,"Reconnect reset player position");
    guest.stop();host.update(0.01F);pump(host,guest,10);
    check(!host.state().pawns[1].connected,"Disconnect not reflected in host state");
    rejectedHandshake(host,port,intruder,Rejection::Resume);
    // A separate fresh room validates seating and stand requests from the guest.
    host.stop();const auto nextPort=startHost(host);guest.join("127.0.0.1",nextPort);pump(host,guest,100);
    guest.interact();pump(host,guest,15);check(host.state().pawns[1].seat==-1,"Guest sat from across the room");
    pump(host,guest,20,{},MovementInput{-1,0,0,false});
    pump(host,guest,45,{},MovementInput{0,-1,0,false});pump(host,guest,20);
    guest.interact();pump(host,guest,20);
    check(host.state().pawns[1].seat==1 && guest.state().pawns[1].seat==1,"Assigned guest seating failed");
    const auto seated=host.state().pawns[1].position;
    pump(host,guest,30,{},MovementInput{1,1,0,true});
    check(distanceSquared(seated,host.state().pawns[1].position)<0.001F,"Seated pawn moved");
    guest.interact(true);pump(host,guest,20);check(host.state().pawns[1].seat==-1,"Guest stand request failed");
    host.stop();guest.stop();
}
}
int main() {
    try {protocolTests();loopbackTests();std::cout<<"PASS: strict packet decoding, build/content handshake, real ENet loopback, host movement/collision, assigned seats, third-player rejection, reserved-token reconnect\n";return EXIT_SUCCESS;}
    catch(const std::exception& e) {std::cerr<<"FAIL: "<<e.what()<<'\n';return EXIT_FAILURE;}
}
