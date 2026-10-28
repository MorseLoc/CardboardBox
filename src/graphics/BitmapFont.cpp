#include "graphics/BitmapFont.h"
#include <array>
#include <cstdint>

namespace fl {
namespace {
using Glyph = std::array<std::uint8_t,7>;
constexpr std::string_view characters = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789.:/-+[]?!";
constexpr Glyph glyphs[] = {
    {14,17,17,31,17,17,17}, {30,17,17,30,17,17,30},
    {14,17,16,16,16,17,14}, {30,17,17,17,17,17,30},
    {31,16,16,30,16,16,31}, {31,16,16,30,16,16,16},
    {14,17,16,23,17,17,15}, {17,17,17,31,17,17,17},
    {14,4,4,4,4,4,14}, {7,2,2,2,18,18,12},
    {17,18,20,24,20,18,17}, {16,16,16,16,16,16,31},
    {17,27,21,21,17,17,17}, {17,25,21,19,17,17,17},
    {14,17,17,17,17,17,14}, {30,17,17,30,16,16,16},
    {14,17,17,17,21,18,13}, {30,17,17,30,20,18,17},
    {15,16,16,14,1,1,30}, {31,4,4,4,4,4,4},
    {17,17,17,17,17,17,14}, {17,17,17,17,17,10,4},
    {17,17,17,21,21,21,10}, {17,17,10,4,10,17,17},
    {17,17,10,4,4,4,4}, {31,1,2,4,8,16,31},
    {14,17,19,21,25,17,14}, {4,12,4,4,4,4,14},
    {14,17,1,2,4,8,31}, {30,1,1,14,1,1,30},
    {2,6,10,18,31,2,2}, {31,16,16,30,1,1,30},
    {14,16,16,30,17,17,14}, {31,1,2,4,8,8,8},
    {14,17,17,14,17,17,14}, {14,17,17,15,1,1,14},
    {0,0,0,0,0,6,6}, {0,6,6,0,6,6,0},
    {1,1,2,4,8,16,16}, {0,0,0,31,0,0,0},
    {0,4,4,31,4,4,0}, {14,8,8,8,8,8,14},
    {14,2,2,2,2,2,14}, {14,17,1,2,4,0,4},
    {4,4,4,4,4,0,4}
};
static_assert(std::size(glyphs)==characters.size());
}
void appendRectangle(std::vector<RenderVertex>& out,float x,float y,float w,float h) {
    out.insert(out.end(),{{x,y,0,1},{x+w,y,0,1},{x+w,y+h,0,1},
                          {x,y,0,1},{x+w,y+h,0,1},{x,y+h,0,1}});
}
void appendText(std::vector<RenderVertex>& out,std::string_view text,float x,float y,float pixelSize) {
    const float start=x;
    for (char c : text) {
        if (c=='\n') { x=start; y+=10*pixelSize; continue; }
        if (c>='a' && c<='z') c=static_cast<char>(c-'a'+'A');
        const auto index=characters.find(c);
        if (index!=std::string_view::npos) {
            for (int row=0;row<7;++row) for (int col=0;col<5;++col) {
                if (glyphs[index][static_cast<std::size_t>(row)] & (1<<(4-col)))
                    appendRectangle(out,x+static_cast<float>(col)*pixelSize,
                                    y+static_cast<float>(row)*pixelSize,pixelSize,pixelSize);
            }
        }
        x+=6*pixelSize;
    }
}
}
