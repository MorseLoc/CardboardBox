#include "cards/CardImages.h"
#include <algorithm>
#include <cmath>
#include <fstream>
#include <memory>
#define STBI_ONLY_JPEG
#define STBI_ONLY_PNG
#define STBI_NO_HDR
#define STBI_NO_LINEAR
#define STBI_MAX_DIMENSIONS 2048
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

namespace fl {
std::optional<CardImage> loadCardImage(const std::filesystem::path& path) {
    std::ifstream file(path,std::ios::binary|std::ios::ate);
    if(!file) return {};
    const auto length=file.tellg();
    if(length<=0 || length>16*1024*1024) return {};
    std::vector<unsigned char> bytes(static_cast<std::size_t>(length));file.seekg(0);
    if(!file.read(reinterpret_cast<char*>(bytes.data()),static_cast<std::streamsize>(bytes.size()))) return {};
    int width=0,height=0,channels=0;
    const int size=static_cast<int>(bytes.size());
    if(!stbi_info_from_memory(bytes.data(),size,&width,&height,&channels) || width<1 || height<1 || width>2048 || height>2048) return {};
    std::unique_ptr<unsigned char,decltype(&stbi_image_free)> pixels{
        stbi_load_from_memory(bytes.data(),size,&width,&height,&channels,4),stbi_image_free};
    if(!pixels) return {};
    CardImage result;result.width=width;result.height=height;
    result.rgba.assign(pixels.get(),pixels.get()+static_cast<std::size_t>(width*height*4));
    return result;
}
CardImage resizeCardImage(const CardImage& source,int width,int height) {
    CardImage out;out.width=width;out.height=height;
    out.rgba.resize(static_cast<std::size_t>(width*height*4));
    for(int y=0;y<height;++y) for(int x=0;x<width;++x) {
        const float fx=(static_cast<float>(x)+0.5F)*static_cast<float>(source.width)/static_cast<float>(width)-0.5F;
        const float fy=(static_cast<float>(y)+0.5F)*static_cast<float>(source.height)/static_cast<float>(height)-0.5F;
        const int x0=std::clamp(static_cast<int>(std::floor(fx)),0,source.width-1),x1=std::min(x0+1,source.width-1);
        const int y0=std::clamp(static_cast<int>(std::floor(fy)),0,source.height-1),y1=std::min(y0+1,source.height-1);
        const float tx=std::clamp(fx-static_cast<float>(x0),0.0F,1.0F),ty=std::clamp(fy-static_cast<float>(y0),0.0F,1.0F);
        for(int c=0;c<4;++c) {
            const auto at=[&](int px,int py){return static_cast<float>(source.rgba[static_cast<std::size_t>((py*source.width+px)*4+c)]);};
            const float top=at(x0,y0)*(1-tx)+at(x1,y0)*tx,bottom=at(x0,y1)*(1-tx)+at(x1,y1)*tx;
            out.rgba[static_cast<std::size_t>((y*width+x)*4+c)]=static_cast<std::uint8_t>(std::clamp(top*(1-ty)+bottom*ty,0.0F,255.0F));
        }
    }
    return out;
}
}
