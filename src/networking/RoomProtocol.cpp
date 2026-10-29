#include "networking/RoomProtocol.h"
#include "cards/CardCatalog.h"
#include <bit>
#include <cmath>
#include <limits>

namespace fl::net {
namespace {
struct Writer {
    std::vector<std::uint8_t> bytes;
    void integer(std::uint64_t n,int count) {for(int i=0;i<count;++i) bytes.push_back(static_cast<std::uint8_t>(n>>(i*8)));}
    void real(float f) {integer(std::bit_cast<std::uint32_t>(f),4);}
};
struct Reader {
    std::span<const std::uint8_t> bytes;std::size_t offset=0;bool valid=true;
    std::uint64_t integer(int count) {
        if(bytes.size()-offset<static_cast<std::size_t>(count)) {valid=false;return 0;}
        std::uint64_t n=0;for(int i=0;i<count;++i) n|=static_cast<std::uint64_t>(bytes[offset++])<<(i*8);return n;
    }
    float real() {const auto f=std::bit_cast<float>(static_cast<std::uint32_t>(integer(4)));if(!std::isfinite(f)) valid=false;return f;}
    bool flag() {const auto n=integer(1);if(n>1) valid=false;return n==1;}
};
constexpr std::uint32_t magic=0x33424c46;
}
std::vector<std::uint8_t> encode(const Message& m) {
    Writer w;w.integer(magic,4);w.integer(protocolVersion,2);w.integer(static_cast<std::uint8_t>(m.kind),1);
    switch(m.kind) {
    case Kind::Hello: w.integer(m.build,4);w.integer(m.catalog,8);for(auto n:m.token) w.integer(n,4);for(auto n:m.identity) w.integer(n,4);break;
    case Kind::Welcome: for(auto n:m.token) w.integer(n,4);w.integer(m.sequence,4);break;
    case Kind::Input: w.integer(m.sequence,4);w.real(m.input.sideways);w.real(m.input.forward);w.real(m.input.yaw);w.integer(m.input.run,1);break;
    case Kind::Interact: case Kind::Stand: break;
    case Kind::State:
        w.integer(m.state.tick,8);
        for(const auto& p:m.state.pawns) {w.real(p.position.x);w.real(p.position.z);w.real(p.yaw);w.integer(p.seat+1,1);w.integer(p.connected,1);}break;
    case Kind::ClubCommand:case Kind::ClubState:case Kind::FaceState:case Kind::CardCommand: case Kind::TableState: case Kind::CardReply:
        w.integer(m.sequence,4);w.integer(m.payload.size(),4);w.bytes.insert(w.bytes.end(),m.payload.begin(),m.payload.end());break;
    case Kind::Reject: w.integer(static_cast<std::uint8_t>(m.rejection),1);break;
    }
    return w.bytes;
}
std::optional<Message> decode(std::span<const std::uint8_t> bytes) {
    if(bytes.size()<7 || bytes.size()>maxPacketBytes) return {};
    Reader r{bytes};if(r.integer(4)!=magic || r.integer(2)!=protocolVersion) return {};
    const auto kind=r.integer(1);if(kind<1 || kind>13) return {};
    Message m;m.kind=static_cast<Kind>(kind);
    switch(m.kind) {
    case Kind::Hello: m.build=static_cast<std::uint32_t>(r.integer(4));m.catalog=r.integer(8);for(auto& n:m.token) n=static_cast<std::uint32_t>(r.integer(4));for(auto& n:m.identity) n=static_cast<std::uint32_t>(r.integer(4));break;
    case Kind::Welcome: for(auto& n:m.token) n=static_cast<std::uint32_t>(r.integer(4));m.sequence=static_cast<std::uint32_t>(r.integer(4));break;
    case Kind::Input:
        m.sequence=static_cast<std::uint32_t>(r.integer(4));m.input.sideways=r.real();m.input.forward=r.real();m.input.yaw=r.real();m.input.run=r.flag();
        if(std::abs(m.input.sideways)>1 || std::abs(m.input.forward)>1 || std::abs(m.input.yaw)>1000) return {};
        break;
    case Kind::Interact: case Kind::Stand: break;
    case Kind::State:
        m.state.tick=r.integer(8);
        for(auto& p:m.state.pawns) {
            p.position={r.real(),r.real()};p.yaw=r.real();p.seat=static_cast<int>(r.integer(1))-1;p.connected=r.flag();
            if(p.seat>1 || std::abs(p.position.x)>layout::roomHalfWidth || std::abs(p.position.z)>layout::roomHalfDepth || std::abs(p.yaw)>1000) return {};
        }break;
    case Kind::ClubCommand:case Kind::ClubState:case Kind::FaceState:case Kind::CardCommand: case Kind::TableState: case Kind::CardReply: {
        m.sequence=static_cast<std::uint32_t>(r.integer(4));
        const auto size=r.integer(4);if(size!=r.bytes.size()-r.offset) return {};
        m.payload.assign(r.bytes.begin()+static_cast<std::ptrdiff_t>(r.offset),r.bytes.end());r.offset=r.bytes.size();break;
    }
    case Kind::Reject: {const auto n=r.integer(1);if(n<1 || n>4) return {};m.rejection=static_cast<Rejection>(n);break;}
    }
    return r.valid && r.offset==bytes.size() ? std::optional{m}:std::nullopt;
}
std::uint64_t catalogFingerprint() {
    std::uint64_t hash=14695981039346656037ULL;
    const auto add=[&](const std::string& s) {for(unsigned char c:s) {hash^=c;hash*=1099511628211ULL;}hash^=0;hash*=1099511628211ULL;};
    for(const auto& d:unlimitedDefinitions()) {
        add(d.id);add(d.name);add(d.type);add(d.cost);add(d.rules);add(d.power);add(d.setId);add(d.rarity);add(d.collectorNumber);add(d.scryfallId);add(d.oracleText);add(d.colorIdentity);add(d.basicLand?"1":"0");
    }
    return hash;
}
}
