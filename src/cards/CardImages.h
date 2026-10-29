#pragma once
#include "cards/CardArt.h"
#include <filesystem>
#include <optional>

namespace fl {
std::optional<CardImage> loadCardImage(const std::filesystem::path& file);
CardImage resizeCardImage(const CardImage& source,int width,int height);
}
