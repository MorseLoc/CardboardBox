#include "networking/NetworkRoom.h"
#include <algorithm>
#include <cmath>
#include <random>
#include <fstream>

namespace fl::net {
namespace {
constexpr float step=1.0F/60.0F;
bool emptyToken(const Token& t) {return std::all_of(t.begin(),t.end(),[](auto n){return n==0;});}
const char* reason(Rejection r) {
    switch(r) {
    case Rejection::Version:return "REJECTED: MATCHING GAME BUILD REQUIRED";
    case Rejection::Content:return "REJECTED: CARD DATA DOES NOT MATCH";
    case Rejection::Full:return "REJECTED: ROOM ALREADY HAS TWO PLAYERS";
    case Rejection::Resume:return "REJECTED: GUEST SLOT RESERVED - HOST MUST REOPEN ROOM";
    }
    return "CONNECTION REJECTED";
}
}
NetworkRoom::~NetworkRoom() {stop();if(enetStarted_) enet_deinitialize();}
bool NetworkRoom::start() {
    stop();
    if(!enetStarted_) {
        if(enet_initialize()!=0) {status_="ENET INITIALIZATION FAILED";return false;}
        enetStarted_=true;
    }
    fingerprint_=catalogFingerprint();world_=Room{};players_={};inputs_={};inputTime_={};state_={};
    players_[0].applyNetworkState({-1.0F,layout::spawnZ},3.14159265F,-1);
    players_[1].applyNetworkState({1.0F,-layout::spawnZ},0,-1);
    clock_=0;simulation_=0;sendTime_=0;retryTime_=0;attemptTime_=0;inputSequence_=0;lastInputSequence_=0;
    tableSession_=TableSession{};tableView_={};tableDirty_=false;viewDirty_=false;tableReady_=false;
    tableSendTime_=0;commandWindow_=0;commandCount_=0;commandSequence_=0;lastCommandSequence_=0;viewSequence_=0;receivedViewSequence_=0;
    pendingMove_.reset();heldPoint_.reset();moveSendTime_=0;heartbeatTime_=0;tableStatus_="SHARED TABLE / HOST AUTHORITY";
    token_={};reserved_=false;terminal_=false;return true;
}
bool NetworkRoom::host(std::uint16_t port) {
    if(!start()) return false;
    const ENetAddress bind{ENET_HOST_ANY,port};
    transport_=enet_host_create(&bind,2,2,0,0);
    if(!transport_) {status_="HOST FAILED: UDP PORT MAY ALREADY BE IN USE";return false;}
    transport_->maximumPacketSize=maxPacketBytes;transport_->maximumWaitingData=262144;
    mode_=Mode::Host;state_.pawns[0].connected=true;refresh();
    tableDirty_=true;publishTable();
    status_="HOSTING UDP "+std::to_string(port)+" - WAITING FOR GUEST";return true;
}
bool NetworkRoom::join(const std::string& address,std::uint16_t port) {
    if(!start()) return false;
    if(address.empty() || address.size()>253 || enet_address_set_host(&address_,address.c_str())!=0) {status_="JOIN FAILED: INVALID HOST ADDRESS";return false;}
    address_.port=port;loadResume(address,port);
    transport_=enet_host_create(nullptr,1,2,0,0);
    if(!transport_) {status_="JOIN FAILED: CANNOT CREATE UDP CLIENT";return false;}
    transport_->maximumPacketSize=maxPacketBytes;transport_->maximumWaitingData=262144;
    mode_=Mode::Guest;connect();return true;
}
void NetworkRoom::stop() {
    if(transport_) {
        if(peer_) enet_peer_disconnect_now(peer_,0);
        enet_host_destroy(transport_);transport_=nullptr;
    }
    pendingMove_.reset();heldPoint_.reset();peer_=nullptr;pending_.clear();joined_=false;tableReady_=false;mode_=Mode::Local;status_="LOCAL TABLETOP";
}
void NetworkRoom::reconnect() {
    if(mode_!=Mode::Guest || terminal_) return;
    if(peer_) enet_peer_disconnect_now(peer_,0);
    loseGuest();
}
void NetworkRoom::connect() {
    peer_=enet_host_connect(transport_,&address_,2,0);attemptTime_=clock_;
    if(peer_) {enet_peer_timeout(peer_,8,3000,6000);status_="CONNECTING - F2 TO CANCEL";}
    else {status_="RETRYING CONNECTION - F2 TO CANCEL";retryTime_=clock_+1;}
}
void NetworkRoom::send(ENetPeer* peer,const Message& m,bool reliable) {
    if(!peer) return;
    const auto bytes=encode(m);
    auto* packet=enet_packet_create(bytes.data(),bytes.size(),reliable?ENET_PACKET_FLAG_RELIABLE:0);
    if(packet && enet_peer_send(peer,reliable?0:1,packet)!=0) enet_packet_destroy(packet);
}
void NetworkRoom::loseGuest() {
    if(mode_==Mode::Host) {
        if(tableSession_.release(1)) tableDirty_=true;
        state_.pawns[1].connected=false;inputs_[1]={};status_="GUEST DISCONNECTED - SLOT RESERVED FOR RECONNECT";
    } else {
        pendingMove_.reset();heldPoint_.reset();tableReady_=false;joined_=false;state_.pawns[0].connected=false;state_.pawns[1].connected=false;
        retryTime_=clock_+1;if(!terminal_) status_="DISCONNECTED - RECONNECTING - F2 TO CANCEL";
    }
    peer_=nullptr;
}
void NetworkRoom::receive(ENetPeer* from,const Message& m) {
    if(mode_==Mode::Host) {
        if(m.kind==Kind::Hello) {
            Rejection rejection{};
            if(m.build!=buildVersion) rejection=Rejection::Version;
            else if(m.catalog!=fingerprint_) rejection=Rejection::Content;
            else if(peer_ && peer_!=from && (!reserved_ || m.token!=token_)) rejection=Rejection::Full;
            else if(reserved_ && m.token!=token_) rejection=Rejection::Resume;
            else if(!reserved_ && !emptyToken(m.token)) rejection=Rejection::Resume;
            if(static_cast<int>(rejection)!=0) {
                Message reject;reject.kind=Kind::Reject;reject.rejection=rejection;send(from,reject,true);
                pending_.erase(from);enet_peer_disconnect_later(from,1);return;
            }
            if(peer_ && peer_!=from) enet_peer_disconnect_now(peer_,0);
            if(tableSession_.release(1)) tableDirty_=true;
            peer_=from;pending_.erase(from);
            if(!reserved_) {
                std::random_device random;do {for(auto& n:token_) n=random();} while(emptyToken(token_));
                reserved_=true;
            }
            state_.pawns[1].connected=true;inputs_[1]={};inputTime_[1]=clock_;lastInputSequence_=0;
            Message welcome;welcome.kind=Kind::Welcome;welcome.token=token_;welcome.sequence=lastCommandSequence_;send(from,welcome,true);
            tableDirty_=true;status_="TWO PLAYERS CONNECTED / HOST AUTHORITY";publish();publishTable();return;
        }
        if(from!=peer_ || !state_.pawns[1].connected) return;
        if(m.kind==Kind::CardCommand && m.sequence>lastCommandSequence_) {
            lastCommandSequence_=m.sequence;
            if(clock_-commandWindow_>=1) {commandWindow_=clock_;commandCount_=0;}
            if(++commandCount_>180) {enet_peer_disconnect_now(from,4);loseGuest();return;}
            const auto command=decodeCommand(m.payload);
            if(!command) {enet_peer_disconnect_now(from,2);loseGuest();return;}
            applyCommand(1,*command);
        }
        if(m.kind==Kind::Input && m.sequence>lastInputSequence_) {lastInputSequence_=m.sequence;inputs_[1]=m.input;inputTime_[1]=clock_;}
        if(m.kind==Kind::Interact) seat(1,false);
        if(m.kind==Kind::Stand) seat(1,true);
    } else if(from==peer_) {
        if(m.kind==Kind::Welcome) {
            token_=m.token;commandSequence_=m.sequence;saveResume();joined_=true;inputSequence_=0;state_.tick=0;receivedViewSequence_=0;status_="CONNECTED AS GUEST / HOST AUTHORITY";
        } else if(m.kind==Kind::State && joined_ && m.state.tick>=state_.tick) state_=m.state;
        else if(m.kind==Kind::TableState && joined_ && m.sequence>receivedViewSequence_) {
            const auto view=decodeView(m.payload);
            if(!view) {enet_peer_disconnect_now(from,2);loseGuest();return;}
            tableView_=*view;viewDirty_=true;tableReady_=true;receivedViewSequence_=m.sequence;
        } else if(m.kind==Kind::CardReply && joined_ && m.payload.size()==1 && m.payload[0]<=static_cast<unsigned>(CardResult::NoChange)) {
            cardResult_=static_cast<CardResult>(m.payload[0]);tableStatus_=cardResultText(cardResult_);
        }
        else if(m.kind==Kind::Reject) {
            if(m.rejection==Rejection::Resume && !emptyToken(token_)) {
                // A reopened host has a fresh room. Retry once without the old
                // cached token; a reserved room will still reject a stranger.
                token_={};saveResume();enet_peer_disconnect_now(from,0);loseGuest();
            } else {terminal_=true;joined_=false;status_=reason(m.rejection);}
        }
    }
}
void NetworkRoom::seat(int side,bool standOnly) {
    if(tableSession_.release(side)) tableDirty_=true;
    auto& p=players_[static_cast<std::size_t>(side)];
    if(p.seated()) {
        const Player before=p;
        if(p.stand(world_) && state_.pawns[static_cast<std::size_t>(1-side)].connected &&
           distanceSquared(p.position(),players_[static_cast<std::size_t>(1-side)].position())<4*Player::radius*Player::radius) {
            p=before;world_.claimSeat(before.seat());
        }
    }
    else if(!standOnly) p.sitAt(world_,side);
    inputs_[static_cast<std::size_t>(side)]={};refresh();publish();
}
void NetworkRoom::interact(bool standOnly) {
    if(mode_==Mode::Host) seat(0,standOnly);
    else if(joined_) {Message m;m.kind=standOnly?Kind::Stand:Kind::Interact;send(peer_,m,true);}
}
void NetworkRoom::applyCommand(int side,const TableCommand& command) {
    const auto result=tableSession_.apply(side,command,state_.pawns[static_cast<std::size_t>(side)].seat==side,clock_);
    if(result==CardResult::Applied) tableDirty_=true;
    if(side==0) {cardResult_=result;tableStatus_=cardResultText(result);}
    else if(command.op!=CardOp::Move || result!=CardResult::Applied) {
        Message reply;reply.kind=Kind::CardReply;reply.sequence=lastCommandSequence_;reply.payload={static_cast<std::uint8_t>(result)};send(peer_,reply,true);
    }
}
void NetworkRoom::command(const TableCommand& c) {
    if(!active() || !ready() || !tableReady_) return;
    if(c.op==CardOp::Begin) {heldPoint_=c.point;heartbeatTime_=clock_+0.5;}
    if(c.op==CardOp::Move) {heldPoint_=c.point;pendingMove_=c;return;}
    if(c.op==CardOp::Finish || c.op==CardOp::Cancel) heldPoint_.reset();
    // Coalesced movement is ordered before finish/cancel on the reliable channel.
    if(pendingMove_) {
        const auto move=*pendingMove_;pendingMove_.reset();
        if(mode_==Mode::Host) applyCommand(0,move);
        else {Message m;m.kind=Kind::CardCommand;m.sequence=++commandSequence_;m.payload=encodeCommand(move);send(peer_,m,true);}
    }
    if(mode_==Mode::Host) applyCommand(0,c);
    else {Message m;m.kind=Kind::CardCommand;m.sequence=++commandSequence_;m.payload=encodeCommand(c);send(peer_,m,true);}
}
void NetworkRoom::publishTable() {
    if(mode_!=Mode::Host || !tableDirty_) return;
    tableView_=tableSession_.table.viewFor(0);viewDirty_=true;tableReady_=true;
    if(peer_) {
        Message m;m.kind=Kind::TableState;m.sequence=++viewSequence_;m.payload=encodeView(tableSession_.table.viewFor(1));send(peer_,m,true);
    }
    tableDirty_=false;
}
bool NetworkRoom::syncTable(Tabletop& display) {
    if(!viewDirty_) return false;
    viewDirty_=false;
    if(!display.applyView(tableView_)) {tableReady_=false;tableStatus_="INVALID TABLE SNAPSHOT - RECONNECT";return false;}
    return true;
}
void NetworkRoom::loadResume(const std::string& address,std::uint16_t port) {
    resumeFile_.clear();if(resumeDirectory_.empty()) return;
    std::uint64_t key=14695981039346656037ULL;
    for(unsigned char c:address+":"+std::to_string(port)) {key^=c;key*=1099511628211ULL;}
    resumeFile_=resumeDirectory_/(std::to_string(key)+".resume");
    std::ifstream file(resumeFile_);Token saved{};
    if(file>>saved[0]>>saved[1]>>saved[2]>>saved[3]) token_=saved;
}
void NetworkRoom::saveResume() {
    if(resumeFile_.empty()) return;
    std::error_code error;
    if(emptyToken(token_)) {std::filesystem::remove(resumeFile_,error);return;}
    std::filesystem::create_directories(resumeDirectory_,error);if(error) return;
    std::ofstream file(resumeFile_,std::ios::trunc);
    if(file) for(auto n:token_) file<<n<<' ';
}
void NetworkRoom::refresh() {
    for(std::size_t i=0;i<2;++i) {
        auto& p=state_.pawns[i];p.position=players_[i].position();p.yaw=players_[i].yaw();p.seat=players_[i].seat();
    }
}
void NetworkRoom::publish() {if(peer_) {Message m;m.kind=Kind::State;m.state=state_;send(peer_,m,false);}}
void NetworkRoom::update(float dt,MovementInput input) {
    if(!active()) return;
    const float elapsed=std::clamp(dt,0.0F,0.1F);clock_+=elapsed;
    ENetEvent event{};
    for(int n=0;n<64 && enet_host_service(transport_,&event,0)>0;++n) {
        if(event.type==ENET_EVENT_TYPE_CONNECT) {
            enet_peer_timeout(event.peer,8,3000,6000);
            if(mode_==Mode::Host) pending_[event.peer]=clock_;
            else if(event.peer==peer_) {Message hello;hello.catalog=fingerprint_;hello.token=token_;send(peer_,hello,true);}
        } else if(event.type==ENET_EVENT_TYPE_RECEIVE) {
            const auto m=decode({event.packet->data,event.packet->dataLength});
            if(m) receive(event.peer,*m);
            else {enet_peer_disconnect_now(event.peer,2);pending_.erase(event.peer);if(event.peer==peer_) loseGuest();}
            enet_packet_destroy(event.packet);
        } else if(event.type==ENET_EVENT_TYPE_DISCONNECT) {
            pending_.erase(event.peer);if(event.peer==peer_) loseGuest();
        }
    }
    for(auto it=pending_.begin();it!=pending_.end();) {
        if(clock_-it->second>5) {enet_peer_disconnect_now(it->first,3);it=pending_.erase(it);}else ++it;
    }
    if(mode_==Mode::Host) {
        inputs_[0]=input;inputTime_[0]=clock_;simulation_+=elapsed;
        while(simulation_>=step) {
            simulation_-=step;
            for(std::size_t i=0;i<2;++i) {
                if(!state_.pawns[i].connected) continue;
                const auto in=clock_-inputTime_[i]<0.3 ? inputs_[i]:MovementInput{};
                const auto old=players_[i].position();const auto oldYaw=players_[i].yaw();
                players_[i].update(in.sideways,in.forward,in.run,in.yaw,step,world_);
                const auto other=players_[1-i].position();
                const float separation=distanceSquared(players_[i].position(),other);
                // On reconnect an idle pawn may overlap the returning pawn.
                // Allow movement that separates them instead of trapping both.
                if(state_.pawns[1-i].connected && separation<4*Player::radius*Player::radius &&
                   separation<=distanceSquared(old,other))
                    players_[i].applyNetworkState(old,oldYaw,players_[i].seat());
            }
            ++state_.tick;refresh();
        }
    } else if(!terminal_) {
        if(peer_ && !joined_ && clock_-attemptTime_>8) {enet_peer_reset(peer_);peer_=nullptr;retryTime_=clock_+1;}
        if(!peer_ && clock_>=retryTime_) connect();
    }
    if(heldPoint_ && clock_>=heartbeatTime_ && !pendingMove_) {
        TableCommand heartbeat;heartbeat.op=CardOp::Move;heartbeat.point=*heldPoint_;pendingMove_=heartbeat;heartbeatTime_=clock_+0.5;
    }
    if(pendingMove_ && ready() && tableReady_ && clock_>=moveSendTime_) {
        moveSendTime_=clock_+0.05;
        const auto move=*pendingMove_;pendingMove_.reset();
        if(mode_==Mode::Host) applyCommand(0,move);
        else {Message m;m.kind=Kind::CardCommand;m.sequence=++commandSequence_;m.payload=encodeCommand(move);send(peer_,m,true);}
    }
    if(mode_==Mode::Host) {
        if(tableSession_.expire(clock_)) tableDirty_=true;
        if(clock_>=tableSendTime_ && tableDirty_) {publishTable();tableSendTime_=clock_+0.05;}
    }
    if(clock_>=sendTime_) {
        sendTime_=clock_+0.05;
        if(mode_==Mode::Host) publish();
        else if(joined_) {Message m;m.kind=Kind::Input;m.sequence=++inputSequence_;m.input=input;send(peer_,m,false);}
    }
    enet_host_flush(transport_);
}
}
