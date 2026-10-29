#pragma once
#include "networking/RoomProtocol.h"
#include "world/Player.h"
#include <enet/enet.h>
#include <map>
#include <string>

namespace fl::net {
enum class Mode {Local,Host,Guest};
class NetworkRoom {
public:
    NetworkRoom()=default;
    ~NetworkRoom();
    NetworkRoom(const NetworkRoom&)=delete;
    NetworkRoom& operator=(const NetworkRoom&)=delete;
    bool host(std::uint16_t port=defaultPort);
    bool join(const std::string& address,std::uint16_t port=defaultPort);
    void stop();
    void reconnect(); // Keep the in-memory resume token; useful after a VPN reconnect.
    void update(float dt,MovementInput input={});
    void interact(bool standOnly=false);
    Mode mode() const {return mode_;}
    bool active() const {return mode_!=Mode::Local;}
    bool ready() const {return mode_==Mode::Host || joined_;}
    int localSide() const {return mode_==Mode::Guest?1:0;}
    const RoomState& state() const {return state_;}
    const std::string& status() const {return status_;}
    std::uint32_t ping() const {return peer_ && joined_ ? peer_->roundTripTime:0;}
private:
    bool start();
    void connect();
    void send(ENetPeer* peer,const Message& message,bool reliable);
    void receive(ENetPeer* peer,const Message& message);
    void publish();
    void refresh();
    void seat(int side,bool standOnly);
    void loseGuest();
    bool enetStarted_=false,joined_=false,terminal_=false,reserved_=false;
    Mode mode_=Mode::Local;
    ENetHost* transport_=nullptr;
    ENetPeer* peer_=nullptr;
    ENetAddress address_{};
    Room world_;
    std::array<Player,2> players_{};
    std::array<MovementInput,2> inputs_{};
    std::array<double,2> inputTime_{};
    RoomState state_{};
    Token token_{};
    std::uint64_t fingerprint_=0;
    std::uint32_t inputSequence_=0,lastInputSequence_=0;
    double clock_=0,simulation_=0,sendTime_=0,retryTime_=0,attemptTime_=0;
    std::map<ENetPeer*,double> pending_;
    std::string status_="LOCAL TABLETOP";
};
}
