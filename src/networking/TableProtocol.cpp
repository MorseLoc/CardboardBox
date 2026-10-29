#include "networking/TableProtocol.h"
#include <bit>
#include <cmath>
namespace fl::net {
namespace {
struct Writer {
    std::vector<std::uint8_t> b;
    void n(std::uint64_t x,int bytes=4) {for(int i=0;i<bytes;++i) b.push_back(static_cast<std::uint8_t>(x>>(8*i)));}
    void f(float x) {n(std::bit_cast<std::uint32_t>(x));}
    void point(Vec2 p) {f(p.x);f(p.z);}
};
struct Reader {
    std::span<const std::uint8_t> b;std::size_t at=0;bool ok=true;
    std::uint64_t n(int bytes=4) {
        if(b.size()-at<static_cast<std::size_t>(bytes)) {ok=false;return 0;}
        std::uint64_t x=0;for(int i=0;i<bytes;++i) x|=static_cast<std::uint64_t>(b[at++])<<(8*i);return x;
    }
    std::uint32_t id() {return static_cast<std::uint32_t>(n());}
    float f() {auto x=std::bit_cast<float>(id());if(!std::isfinite(x)) ok=false;return x;}
    Vec2 point() {return {f(),f()};}
    bool flag() {auto x=n(1);if(x>1) ok=false;return x==1;}
    std::vector<CardId> ids() {
        auto count=n(1);std::vector<CardId> out;if(count>120) {ok=false;return out;}
        for(unsigned i=0;i<count;++i) out.push_back(id());
        return out;
    }
    bool done() const {return ok && at==b.size();}
};
void ids(Writer& w,const std::vector<CardId>& v) {w.n(v.size(),1);for(auto id:v) w.n(id);}
}
std::vector<std::uint8_t> encodeCommand(const TableCommand& c) {
    Writer w;w.n(static_cast<unsigned>(c.op),1);ids(w,c.cards);w.n(c.pile);w.n(c.target);
    w.point(c.point);w.f(c.rotation);w.n(c.count,1);w.n(c.bottom,1);return w.b;
}
std::optional<TableCommand> decodeCommand(std::span<const std::uint8_t> bytes) {
    Reader r{bytes};TableCommand c;auto op=r.n(1);if(op<1 || op>14) return {};
    c.op=static_cast<CardOp>(op);c.cards=r.ids();c.pile=r.id();c.target=r.id();
    c.point=r.point();c.rotation=r.f();c.count=static_cast<std::uint8_t>(r.n(1));c.bottom=r.flag();
    return r.done()?std::optional{c}:std::nullopt;
}
std::vector<std::uint8_t> encodeView(const TableView& view) {
    Writer w;w.n(view.cards.size(),1);
    for(const auto& c:view.cards) {
        w.n(c.id);w.n(c.definition==hiddenDefinition?0xffffU:c.definition,2);w.n(c.owner,1);
        w.n(static_cast<unsigned>(c.zone),1);w.n(c.handOwner+1,1);w.n(c.pile);
        w.point(c.position);w.f(c.angle);w.f(c.height);w.n(c.faceUp,1);w.n(c.tapped,1);w.n(c.order,8);
    }
    w.n(view.piles.size(),1);
    for(const auto& p:view.piles) {
        w.n(p.id);w.n(p.owner,1);w.point(p.position);w.f(p.angle);w.n(p.hidden,1);w.n(p.tapped,1);ids(w,p.cards);
    }
    return w.b;
}
std::optional<TableView> decodeView(std::span<const std::uint8_t> bytes) {
    Reader r{bytes};TableView view;const auto count=r.n(1);if(count!=120) return {};
    for(unsigned i=0;i<count && r.ok;++i) {
        CardInstance c{};c.id=r.id();const auto definition=r.n(2);c.definition=definition==0xffffU?hiddenDefinition:static_cast<std::size_t>(definition);
        c.owner=static_cast<int>(r.n(1));c.zone=static_cast<CardZone>(r.n(1));c.handOwner=static_cast<int>(r.n(1))-1;c.pile=r.id();
        c.position=r.point();c.angle=r.f();c.height=r.f();c.faceUp=r.flag();c.tapped=r.flag();c.order=r.n(8);view.cards.push_back(c);
    }
    const auto piles=r.n(1);if(piles>122) return {};
    for(unsigned i=0;i<piles && r.ok;++i) {
        CardPile p{};p.id=r.id();p.owner=static_cast<int>(r.n(1));p.position=r.point();p.angle=r.f();p.hidden=r.flag();p.tapped=r.flag();p.cards=r.ids();view.piles.push_back(p);
    }
    return r.done()?std::optional{view}:std::nullopt;
}
}
