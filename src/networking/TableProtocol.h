#pragma once
#include "cards/TableCommand.h"
#include <optional>
namespace fl::net {
std::vector<std::uint8_t> encodeCommand(const TableCommand& command);
std::optional<TableCommand> decodeCommand(std::span<const std::uint8_t> bytes);
std::vector<std::uint8_t> encodeView(const TableView& view);
std::optional<TableView> decodeView(std::span<const std::uint8_t> bytes);
}
