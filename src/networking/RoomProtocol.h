#pragma once
#include "world/Geometry.h"
#include "world/TableLayout.h"
#include <array>
#include <cstdint>
#include <optional>
#include <span>
#include <vector>

namespace fl::net {
// This room-only protocol deliberately contains no card-instance fields.
inline constexpr std::uint16_t protocolVersion=1;
inline constexpr std::uint32_t buildVersion=0x00030000;
inline constexpr std::uint16_t defaultPort=33445;
inline constexpr std::size_t maxPacketBytes=512;
using Token=std::array<std::uint32_t,4>;
struct MovementInput {float sideways=0,forward=0,yaw=0;bool run=false;};
struct PawnState {Vec2 position{};float yaw=0;int seat=-1;bool connected=false;};
struct RoomState {std::uint64_t tick=0;std::array<PawnState,2> pawns{};};
enum class Kind:std::uint8_t {Hello=1,Welcome,Input,Interact,Stand,State,Reject};
enum class Rejection:std::uint8_t {Version=1,Content,Full,Resume};
struct Message {
    Kind kind=Kind::Hello;
    std::uint32_t build=buildVersion,sequence=0;
    std::uint64_t catalog=0;
    Token token{};
    MovementInput input{};
    RoomState state{};
    Rejection rejection=Rejection::Version;
};
std::vector<std::uint8_t> encode(const Message& message);
std::optional<Message> decode(std::span<const std::uint8_t> bytes);
// Stable across compilers/platforms; hashes all public definition data, not art files.
std::uint64_t catalogFingerprint();
}
