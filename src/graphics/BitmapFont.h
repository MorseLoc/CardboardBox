#pragma once
#include <string_view>
#include <vector>

namespace fl {
struct RenderVertex { float x, y, z, shade; };
// Built-in original 5x7 glyphs keep the prototype independent of font assets.
void appendText(std::vector<RenderVertex>& vertices, std::string_view text,
                float x, float y, float pixelSize);
void appendRectangle(std::vector<RenderVertex>& vertices, float x, float y, float width, float height);
}
