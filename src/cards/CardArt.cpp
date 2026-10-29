#include "cards/CardArt.h"
#include "graphics/BitmapFont.h"
#include <algorithm>
#include <cmath>
#include <sstream>

namespace fl {
namespace {
void rectangle(CardImage& image,int x,int y,int w,int h,Vec3 color) {
    for(int row=std::max(0,y);row<std::min(image.height,y+h);++row)
        for(int column=std::max(0,x);column<std::min(image.width,x+w);++column) {
            const auto offset=static_cast<std::size_t>((row*image.width+column)*4);
            image.rgba[offset]=static_cast<std::uint8_t>(std::clamp(color.x,0.0F,1.0F)*255);
            image.rgba[offset+1]=static_cast<std::uint8_t>(std::clamp(color.y,0.0F,1.0F)*255);
            image.rgba[offset+2]=static_cast<std::uint8_t>(std::clamp(color.z,0.0F,1.0F)*255);
            image.rgba[offset+3]=255;
        }
}
void circle(CardImage& image,int x,int y,int radius,Vec3 color) {
    for(int dy=-radius;dy<=radius;++dy) {
        const int half=static_cast<int>(std::sqrt(static_cast<float>(radius*radius-dy*dy)));
        rectangle(image,x-half,y+dy,half*2+1,1,color);
    }
}
void text(CardImage& image,std::string_view value,int x,int y,int size,Vec3 color) {
    std::vector<RenderVertex> vertices;appendText(vertices,value,static_cast<float>(x),static_cast<float>(y),static_cast<float>(size));
    for(std::size_t i=0;i<vertices.size();i+=6) rectangle(image,static_cast<int>(vertices[i].x),static_cast<int>(vertices[i].y),size,size,color);
}
std::string wrap(std::string_view value) {
    std::istringstream lines{std::string(value)};std::string line,result;
    while(std::getline(lines,line)) {
        std::istringstream words{line};std::string word;int length=0;
        while(words>>word) {
            if(length && length+1+static_cast<int>(word.size())>25) {result+='\n';length=0;}
            if(length) {result+=' ';++length;}
            result+=word;length+=static_cast<int>(word.size());
        }
        result+='\n';
    }
    return result;
}
}
CardImage makeCardImage(const CardDefinition* definition) {
    CardImage image;image.rgba.resize(static_cast<std::size_t>(image.width*image.height*4),255);
    const Vec3 ink{0.12F,0.10F,0.15F};
    rectangle(image,0,0,512,720,ink);
    if(!definition) {
        rectangle(image,12,12,488,696,{0.34F,0.24F,0.46F});
        rectangle(image,26,26,460,668,{0.18F,0.19F,0.33F});
        for(int i=0;i<5;++i) circle(image,256,390,140-i*22,i%2 ? Vec3{0.18F,0.19F,0.33F}:Vec3{0.71F,0.44F,0.65F});
        text(image,"FEMBOYLOVER.EXE",88,80,4,{0.97F,0.82F,0.73F});
        text(image,"PRIVATE CLUBHOUSE",52,590,4,{0.97F,0.82F,0.73F});
        return image;
    }
    const auto& d=*definition;
    rectangle(image,12,12,488,696,d.color);
    rectangle(image,24,24,464,38,{0.96F,0.90F,0.78F});
    text(image,d.name,32,32,3,ink);text(image,d.cost,424,32,3,ink);
    // Original procedural placeholder illustration: a moonlit pawn in a
    // landscape, recolored for each definition. No external art/database used.
    CardImage art=image;
    rectangle(art,24,78,464,278,{0.15F,0.20F,0.29F});
    circle(art,397,137,36,{0.97F,0.85F,0.58F});
    circle(art,130,335,125,{0.18F,0.34F,0.30F});
    circle(art,367,354,150,{0.13F,0.27F,0.27F});
    rectangle(art,205,238,102,78,d.color);
    circle(art,256,202,49,{0.94F,0.74F,0.64F});
    circle(art,240,195,6,ink);circle(art,272,195,6,ink);
    rectangle(art,244,220,24,4,ink);
    for(int row=78;row<356;++row) {
        const auto offset=static_cast<std::size_t>((row*image.width+24)*4);
        std::copy_n(art.rgba.begin()+static_cast<std::ptrdiff_t>(offset),464*4,image.rgba.begin()+static_cast<std::ptrdiff_t>(offset));
    }
    rectangle(image,24,372,464,38,{0.96F,0.90F,0.78F});
    text(image,d.type,32,380,3,ink);
    rectangle(image,24,422,464,264,{0.96F,0.90F,0.78F});
    text(image,wrap(d.rules),32,438,3,ink);
    text(image,d.power,410,650,3,ink);
    text(image,"ORIGINAL TEST CARD",32,692,2,{1,0.94F,0.86F});
    return image;
}
}
