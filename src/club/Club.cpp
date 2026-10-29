#include "club/Club.h"
#include "cards/CardCatalog.h"
#include <sqlite3.h>
#include <algorithm>
#include <map>
#include <random>
#include <sstream>
#include <stdexcept>
namespace fl {
Face defaultFace() {
    Face f(facePixels,0);
    for(int y=70;y<115;++y) for(int x=55;x<201;++x) if((x>70 && x<97)||(x>159 && x<186)) f[static_cast<std::size_t>(y*256+x)]=1;
    for(int y=165;y<176;++y) for(int x=100;x<156;++x) f[static_cast<std::size_t>(y*256+x)]=1;
    return f;
}
std::string newRequestId() {std::random_device r;std::ostringstream o;o<<std::hex;for(int i=0;i<4;++i) {o.width(8);o.fill('0');o<<static_cast<std::uint32_t>(r());}return o.str();}
namespace {
const std::map<std::string,std::array<int,3>> boosterWeights{
#include "club/BoosterWeights.inc"
};
struct Query {
    sqlite3* db;sqlite3_stmt* s=nullptr;
    Query(sqlite3* d,const char* sql):db(d) {if(sqlite3_prepare_v2(db,sql,-1,&s,nullptr)!=SQLITE_OK) throw std::runtime_error(sqlite3_errmsg(db));}
    ~Query(){sqlite3_finalize(s);}
    void text(int at,const std::string& v){if(sqlite3_bind_text(s,at,v.c_str(),static_cast<int>(v.size()),SQLITE_TRANSIENT)!=SQLITE_OK) throw std::runtime_error("Bind text failed");}
    void number(int at,std::int64_t n){sqlite3_bind_int64(s,at,n);}
    void blob(int at,const Face& f){sqlite3_bind_blob(s,at,f.data(),static_cast<int>(f.size()),SQLITE_TRANSIENT);}
    bool row(){const int n=sqlite3_step(s);if(n!=SQLITE_ROW && n!=SQLITE_DONE) throw std::runtime_error(sqlite3_errmsg(db));return n==SQLITE_ROW;}
    std::string text(int col){const auto* p=sqlite3_column_text(s,col);return p?reinterpret_cast<const char*>(p):"";}
    std::int64_t number(int col){return sqlite3_column_int64(s,col);}
};
void exec(sqlite3* db,const char* sql){if(sqlite3_exec(db,sql,nullptr,nullptr,nullptr)!=SQLITE_OK) throw std::runtime_error(sqlite3_errmsg(db));}
bool validName(const std::string& s){return !s.empty() && s.size()<=32 && s.find_first_not_of(' ')!=std::string::npos && std::all_of(s.begin(),s.end(),[](unsigned char c){return c>=32 && c<127;});}
}
struct ClubStore::Impl {
    sqlite3* db=nullptr;std::vector<CardDefinition> defs;std::map<std::string,std::uint16_t> index;std::mt19937 rng{std::random_device{}()};
    void copy(const std::string& player,std::uint16_t def){Query q(db,"INSERT INTO copies(player,definition) VALUES(?,?)");q.text(1,player);q.text(2,defs.at(def).id);q.row();}
};
ClubStore::ClubStore(const std::filesystem::path& file,const std::vector<CardDefinition>& defs):impl_(std::make_unique<Impl>()) {
    auto& p=*impl_;p.defs=defs;for(std::size_t i=0;i<defs.size();++i) p.index[defs[i].id]=static_cast<std::uint16_t>(i);
    if(!file.empty() && !file.parent_path().empty()) std::filesystem::create_directories(file.parent_path());
    const auto u8=file.u8string();const auto utf=file.empty()?std::string{":memory:"}:std::string(u8.begin(),u8.end());
    if(sqlite3_open(utf.c_str(),&p.db)!=SQLITE_OK) {const std::string error=p.db?sqlite3_errmsg(p.db):"Cannot open collection database";if(p.db) sqlite3_close(p.db);p.db=nullptr;throw std::runtime_error(error);}
    try {
        sqlite3_busy_timeout(p.db,3000);exec(p.db,"PRAGMA foreign_keys=ON;PRAGMA synchronous=FULL;PRAGMA journal_mode=DELETE;");
        exec(p.db,"CREATE TABLE IF NOT EXISTS players(id TEXT PRIMARY KEY,cents INTEGER NOT NULL CHECK(cents>=0),selected TEXT NOT NULL,face BLOB NOT NULL);CREATE TABLE IF NOT EXISTS copies(id INTEGER PRIMARY KEY AUTOINCREMENT,player TEXT NOT NULL REFERENCES players(id),definition TEXT NOT NULL);CREATE INDEX IF NOT EXISTS copies_owner ON copies(player);CREATE TABLE IF NOT EXISTS decks(player TEXT NOT NULL REFERENCES players(id),name TEXT NOT NULL,cards TEXT NOT NULL,PRIMARY KEY(player,name));CREATE TABLE IF NOT EXISTS receipts(player TEXT NOT NULL REFERENCES players(id),request TEXT NOT NULL,result TEXT NOT NULL,opened TEXT NOT NULL,PRIMARY KEY(player,request));PRAGMA user_version=1;");
    }catch(...){sqlite3_close(p.db);p.db=nullptr;throw;}
}
ClubStore::~ClubStore(){if(impl_->db) sqlite3_close(impl_->db);}
void ClubStore::ensure(const std::string& id,int side) {
    (void)side;
    auto& p=*impl_;Query exists(p.db,"SELECT id FROM players WHERE id=?");exists.text(1,id);if(exists.row())return;
    exec(p.db,"BEGIN IMMEDIATE");try {
        Query add(p.db,"INSERT INTO players VALUES(?,50000,'',?)");add.text(1,id);add.blob(2,defaultFace());add.row();
        exec(p.db,"COMMIT");
    }catch(...){exec(p.db,"ROLLBACK");throw;}
}
ClubProfile ClubStore::profile(const std::string& id) {
    auto& p=*impl_;ClubProfile out;out.owned.resize(p.defs.size());
    Query player(p.db,"SELECT cents,selected FROM players WHERE id=?");player.text(1,id);if(!player.row())throw std::runtime_error("Unknown player");out.cents=player.number(0);out.selected=player.text(1);
    Query copies(p.db,"SELECT definition,COUNT(*) FROM copies WHERE player=? GROUP BY definition");copies.text(1,id);while(copies.row()){auto i=p.index.find(copies.text(0));if(i==p.index.end())throw std::runtime_error("Collection uses unknown card data");out.owned[i->second]=static_cast<std::uint32_t>(copies.number(1));}
    Query decks(p.db,"SELECT name,cards FROM decks WHERE player=? ORDER BY name");decks.text(1,id);while(decks.row()){SavedDeck d;d.name=decks.text(0);std::istringstream in(decks.text(1));std::string def;while(std::getline(in,def)){auto i=p.index.find(def);if(i==p.index.end())throw std::runtime_error("Deck uses unknown card data");d.cards.push_back(i->second);}out.decks.push_back(std::move(d));}
    return out;
}
Face ClubStore::face(const std::string& id){Query q(impl_->db,"SELECT face FROM players WHERE id=?");q.text(1,id);if(!q.row())return defaultFace();const auto n=sqlite3_column_bytes(q.s,0);if(n!=facePixels)throw std::runtime_error("Invalid saved face");const auto* data=static_cast<const std::uint8_t*>(sqlite3_column_blob(q.s,0));return Face(data,data+n);}
std::string ClubStore::apply(const std::string& id,const ClubCommand& c,std::vector<std::uint16_t>& opened) {
    auto& p=*impl_;opened.clear();if(c.request.size()!=32)return "INVALID REQUEST ID";
    exec(p.db,"BEGIN IMMEDIATE");try {
        Query old(p.db,"SELECT result,opened FROM receipts WHERE player=? AND request=?");old.text(1,id);old.text(2,c.request);
        if(old.row()){std::istringstream in(old.text(1));std::string d;while(in>>d){const auto at=p.index.find(d);if(at==p.index.end())throw std::runtime_error("Receipt uses unknown card data");opened.push_back(at->second);}const auto result=old.text(0);exec(p.db,"COMMIT");return result;}
        auto current=profile(id);std::string result="SAVED";bool valid=true;
        if(c.op==ClubOp::Pack || c.op==ClubOp::Land) {
            const std::int64_t price=c.op==ClubOp::Pack?10000:100;
            if(current.cents<price){valid=false;result="NOT ENOUGH COINS";}
            if(c.op==ClubOp::Land && (c.value<0 || static_cast<std::size_t>(c.value)>=p.defs.size() || !p.defs[static_cast<std::size_t>(c.value)].basicLand)){valid=false;result="CHOOSE A BASIC LAND";}
            if(valid) {
                if(c.op==ClubOp::Land)opened.push_back(static_cast<std::uint16_t>(c.value));
                else for(const auto& [rarity,count]:std::array<std::pair<const char*,int>,3>{{{"common",11},{"uncommon",3},{"rare",1}}}) {
                    std::vector<std::uint16_t> pool;const int sheet=std::string{rarity}=="common"?0:(std::string{rarity}=="uncommon"?1:2);for(std::size_t i=0;i<p.defs.size();++i){const auto weights=boosterWeights.find(p.defs[i].id);if(weights!=boosterWeights.end() && p.defs[i].rarity==rarity && !p.defs[i].basicLand)for(int n=0;n<weights->second[sheet];++n)pool.push_back(static_cast<std::uint16_t>(i));}
                    if(pool.empty())throw std::runtime_error("Missing booster rarity sheet");
                    std::uniform_int_distribution<std::size_t> pick(0,pool.size()-1);for(int j=0;j<count;++j)opened.push_back(pool[pick(p.rng)]);
                }
                for(auto d:opened)p.copy(id,d);
                Query balance(p.db,"UPDATE players SET cents=cents-? WHERE id=?");balance.number(1,price);balance.text(2,id);balance.row();result=c.op==ClubOp::Pack?"OPENED 11 COMMON / 3 UNCOMMON / 1 RARE":"BOUGHT BASIC LAND FOR 1.00 COINS";
            }
        } else if(c.op==ClubOp::SaveDeck) {
            if(!validName(c.name)||c.cards.empty()||c.cards.size()>120){valid=false;result="NAME AND 1 TO 120 CARDS REQUIRED";}
            std::vector<unsigned> used(p.defs.size());for(auto d:c.cards)if(d>=used.size()||++used[d]>current.owned[d]){valid=false;result="DECK EXCEEDS OWNED COPIES";break;}
            const bool present=std::any_of(current.decks.begin(),current.decks.end(),[&](const auto& d){return d.name==c.name;});
            if(!present && current.decks.size()>=16){valid=false;result="MAXIMUM 16 SAVED DECKS";}
            if(valid){std::string list;for(auto d:c.cards)list+=p.defs[d].id+"\n";Query q(p.db,"INSERT INTO decks VALUES(?,?,?) ON CONFLICT(player,name) DO UPDATE SET cards=excluded.cards");q.text(1,id);q.text(2,c.name);q.text(3,list);q.row();}
        } else if(c.op==ClubOp::DeleteDeck || c.op==ClubOp::SelectDeck) {
            const bool exists=std::any_of(current.decks.begin(),current.decks.end(),[&](const auto& d){return d.name==c.name;});
            if(!exists){valid=false;result="DECK NOT FOUND";}
            else if(c.op==ClubOp::DeleteDeck){Query q(p.db,"DELETE FROM decks WHERE player=? AND name=?");q.text(1,id);q.text(2,c.name);q.row();if(current.selected==c.name){Query clear(p.db,"UPDATE players SET selected='' WHERE id=?");clear.text(1,id);clear.row();}}
            else {Query q(p.db,"UPDATE players SET selected=? WHERE id=?");q.text(1,c.name);q.text(2,id);q.row();}
        } else if(c.op==ClubOp::Face) {
            if(c.face.size()!=facePixels || std::any_of(c.face.begin(),c.face.end(),[](auto n){return n>=16;})){valid=false;result="INVALID FACE";}
            if(valid){Query q(p.db,"UPDATE players SET face=? WHERE id=?");q.blob(1,c.face);q.text(2,id);q.row();result="FACE SAVED AND SHARED";}
        } else {valid=false;result="INVALID STORE ACTION";}
        std::string list;for(auto d:opened)list+=p.defs[d].id+" ";Query receipt(p.db,"INSERT INTO receipts VALUES(?,?,?,?)");receipt.text(1,id);receipt.text(2,c.request);receipt.text(3,result);receipt.text(4,list);receipt.row();exec(p.db,"COMMIT");return result;
    }catch(...){exec(p.db,"ROLLBACK");throw;}
}
bool ClubStore::backup(const std::filesystem::path& file){sqlite3* target=nullptr;if(sqlite3_open(reinterpret_cast<const char*>(file.u8string().c_str()),&target)!=SQLITE_OK){if(target)sqlite3_close(target);return false;}auto* b=sqlite3_backup_init(target,"main",impl_->db,"main");bool ok=false;if(b){ok=sqlite3_backup_step(b,-1)==SQLITE_DONE;ok=sqlite3_backup_finish(b)==SQLITE_OK&&ok;}sqlite3_close(target);return ok;}
}
