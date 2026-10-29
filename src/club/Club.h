#pragma once
#include "cards/Tabletop.h"
#include <array>
#include <filesystem>
#include <memory>
namespace fl {
inline constexpr int faceSize=256;
inline constexpr int facePixels=faceSize*faceSize;
using Face=std::vector<std::uint8_t>;
inline constexpr std::array<std::array<std::uint8_t,3>,16> paintColors{{{250,199,174},{30,22,36},{255,255,255},{235,66,106},{246,158,70},{249,222,100},{68,174,108},{56,190,185},{62,119,215},{137,88,194},{234,142,192},{130,79,57},{134,140,151},{74,66,92},{176,224,250},{165,222,137}}};
Face defaultFace();
struct SavedDeck {std::string name;std::vector<std::uint16_t> cards;};
struct ClubProfile {std::int64_t cents=0;std::vector<std::uint32_t> owned;std::vector<SavedDeck> decks;std::string selected;};
enum class ClubOp:std::uint8_t {Pack=1,Land,SaveDeck,DeleteDeck,SelectDeck,Face,Ready,Life,Winner,Reset};
struct ClubCommand {ClubOp op=ClubOp::Pack;std::string request,name;int value=0;std::vector<std::uint16_t> cards;fl::Face face;};
struct MatchInfo {std::array<bool,2> ready{},reset{};std::array<int,2> life{20,20},votes{-1,-1};bool active=false;int winner=-1;};
struct ClubView {ClubProfile profile;std::array<Face,2> faces{defaultFace(),defaultFace()};MatchInfo match;std::string status="CONNECT TO YOUR CLUBHOUSE";std::vector<std::uint16_t> opened;};
std::string newRequestId();
class ClubStore {
public:
    ClubStore(const std::filesystem::path& file,const std::vector<CardDefinition>& definitions);
    ~ClubStore();
    ClubStore(const ClubStore&)=delete;
    ClubStore& operator=(const ClubStore&)=delete;
    void ensure(const std::string& player,int side);
    ClubProfile profile(const std::string& player);
    Face face(const std::string& player);
    std::string apply(const std::string& player,const ClubCommand& command,std::vector<std::uint16_t>& opened);
    bool backup(const std::filesystem::path& file);
private:
    struct Impl;std::unique_ptr<Impl> impl_;
};
std::vector<std::uint8_t> encodeClubCommand(const ClubCommand& c);
std::optional<ClubCommand> decodeClubCommand(std::span<const std::uint8_t> b);
std::vector<std::uint8_t> encodeClubView(const ClubView& v);
std::optional<ClubView> decodeClubView(std::span<const std::uint8_t> b);
}
