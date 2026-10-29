#include "graphics/Renderer.h"
#include "graphics/BitmapFont.h"
#include "cards/CardArt.h"
#include "cards/CardImages.h"
#include <glm/gtc/constants.hpp>
#include <algorithm>
#include <cmath>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <array>
#include <cstddef>
#include <stdexcept>
#include <string>
#include <vector>

namespace fl {
namespace {
constexpr const char* vertexSource = R"(
#version 330 core
layout(location = 0) in vec3 position;
layout(location = 1) in float shade;
layout(location = 2) in vec2 texCoord;
out vec2 uv;
uniform mat4 mvp;
out float faceShade;
void main() {
    gl_Position = mvp * vec4(position, 1.0);
    faceShade = shade;
    uv=texCoord;
}
)";
constexpr const char* fragmentSource = R"(
#version 330 core
in float faceShade;
in vec2 uv;
uniform sampler2D imageTexture;
uniform int useTexture;
uniform vec3 tint;
out vec4 color;
void main() { color = vec4(tint * faceShade, 1.0);
    if(useTexture!=0) color*=texture(imageTexture,uv);
}
)";
using Vertex = RenderVertex;
}
GLuint Renderer::compile(GLenum type, const char* source) {
    const GLuint shader = gl_.CreateShader(type);
    if (!shader) throw std::runtime_error("Could not allocate shader.");
    gl_.ShaderSource(shader, 1, &source, nullptr);
    gl_.CompileShader(shader);
    GLint success = GL_FALSE;
    gl_.GetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        std::array<char, 4096> log{};
        gl_.GetShaderInfoLog(shader, static_cast<GLsizei>(log.size()), nullptr, log.data());
        gl_.DeleteShader(shader);
        throw std::runtime_error(std::string{"Shader compilation failed: "} + log.data());
    }
    return shader;
}
void Renderer::initialize(const Tabletop& table,const std::filesystem::path& cacheRoot,const std::filesystem::path& bundledRoot) {
    definitions_=table.definitions();cacheRoot_=cacheRoot;bundledRoot_=bundledRoot;
    gl_.load();
    // Application owns this renderer and calls shutdown even after partial setup.
    const GLuint vertex = compile(GL_VERTEX_SHADER, vertexSource);
    GLuint fragment = 0;
    try { fragment = compile(GL_FRAGMENT_SHADER, fragmentSource); }
    catch (...) { gl_.DeleteShader(vertex); throw; }
    program_ = gl_.CreateProgram();
    if (!program_) {
        gl_.DeleteShader(vertex); gl_.DeleteShader(fragment);
        throw std::runtime_error("Could not allocate shader program.");
    }
    gl_.AttachShader(program_, vertex);
    gl_.AttachShader(program_, fragment);
    gl_.LinkProgram(program_);
    gl_.DeleteShader(vertex); gl_.DeleteShader(fragment);
    GLint linked = GL_FALSE;
    gl_.GetProgramiv(program_, GL_LINK_STATUS, &linked);
    if (!linked) {
        std::array<char, 4096> log{};
        gl_.GetProgramInfoLog(program_, static_cast<GLsizei>(log.size()), nullptr, log.data());
        throw std::runtime_error(std::string{"Shader linking failed: "} + log.data());
    }
    mvpLocation_ = gl_.GetUniformLocation(program_, "mvp");
    tintLocation_ = gl_.GetUniformLocation(program_, "tint");
    useTextureLocation_=gl_.GetUniformLocation(program_,"useTexture");
    if (mvpLocation_ < 0 || tintLocation_ < 0 || useTextureLocation_<0) throw std::runtime_error("Missing shader uniform.");

    // Reuse primitive meshes for all room pieces and the pawn. Face shading
    // keeps the prototype readable without textures or a lighting dependency.
    constexpr std::array<glm::vec3, 8> corners{{
        {-0.5F,-0.5F,-0.5F}, {0.5F,-0.5F,-0.5F}, {0.5F,0.5F,-0.5F}, {-0.5F,0.5F,-0.5F},
        {-0.5F,-0.5F,0.5F}, {0.5F,-0.5F,0.5F}, {0.5F,0.5F,0.5F}, {-0.5F,0.5F,0.5F}}};
    constexpr int faces[6][4] = {{4,5,6,7},{1,0,3,2},{0,4,7,3},{5,1,2,6},{3,7,6,2},{0,1,5,4}};
    constexpr float shades[] = {0.85F,0.65F,0.70F,0.90F,1.0F,0.55F};
    constexpr int triangles[] = {0,1,2,0,2,3};
    std::vector<Vertex> vertices;
    vertices.reserve(4096);
    for (int face = 0; face < 6; ++face) {
        for (int index : triangles) {
            const auto p = corners[static_cast<std::size_t>(faces[face][index])];
            vertices.push_back({p.x, p.y, p.z, shades[face]});
        }
    }
    const auto addSurface = [&vertices](glm::vec3 a,glm::vec3 b,glm::vec3 c) {
        const glm::vec3 cross = glm::cross(b-a,c-a);
        if (glm::length(cross)<0.000001F) return;
        const auto normal = glm::normalize(cross);
        const float shade = std::clamp(0.74F+normal.y*0.20F+normal.x*0.10F-normal.z*0.08F,0.42F,1.0F);
        for (auto p : {a,b,c}) vertices.push_back({p.x,p.y,p.z,shade});
    };
    sphereFirst_=static_cast<GLint>(vertices.size());
    constexpr int slices=18,rings=12;
    const auto point = [](int ring,int slice) {
        const float theta=glm::pi<float>()*static_cast<float>(ring)/static_cast<float>(rings);
        const float phi=glm::two_pi<float>()*static_cast<float>(slice)/static_cast<float>(slices);
        return glm::vec3{0.5F*std::sin(theta)*std::cos(phi),0.5F*std::cos(theta),0.5F*std::sin(theta)*std::sin(phi)};
    };
    for (int r=0;r<rings;++r) for (int i=0;i<slices;++i) {
        const auto a=point(r,i),b=point(r+1,i),c=point(r+1,i+1),d=point(r,i+1);
        addSurface(a,c,b); addSurface(a,d,c);
    }
    sphereCount_=static_cast<GLsizei>(vertices.size())-sphereFirst_;
    pawnFirst_=static_cast<GLint>(vertices.size());
    // Revolve a simple radius/height profile to create a chess-pawn silhouette.
    constexpr std::array<glm::vec2,9> profile{{{0,0},{0.30F,0},{0.36F,0.07F},
        {0.35F,0.15F},{0.24F,0.22F},{0.17F,0.48F},{0.22F,0.64F},{0.18F,0.73F},{0,0.73F}}};
    for (std::size_t r=0;r+1<profile.size();++r) for (int i=0;i<slices;++i) {
        const float a=glm::two_pi<float>()*static_cast<float>(i)/static_cast<float>(slices);
        const float b=glm::two_pi<float>()*static_cast<float>(i+1)/static_cast<float>(slices);
        const glm::vec3 p0{profile[r].x*std::cos(a),profile[r].y,profile[r].x*std::sin(a)};
        const glm::vec3 p1{profile[r+1].x*std::cos(a),profile[r+1].y,profile[r+1].x*std::sin(a)};
        const glm::vec3 p2{profile[r+1].x*std::cos(b),profile[r+1].y,profile[r+1].x*std::sin(b)};
        const glm::vec3 p3{profile[r].x*std::cos(b),profile[r].y,profile[r].x*std::sin(b)};
        addSurface(p0,p1,p2); addSurface(p0,p2,p3);
    }
    pawnCount_=static_cast<GLsizei>(vertices.size())-pawnFirst_;
    cardFirst_=static_cast<GLint>(vertices.size());
    vertices.insert(vertices.end(),{{-0.5F,0,-0.5F,1,0,0},{0.5F,0,-0.5F,1,1,0},{0.5F,0,0.5F,1,1,1},
                                    {-0.5F,0,-0.5F,1,0,0},{0.5F,0,0.5F,1,1,1},{-0.5F,0,0.5F,1,0,1}});
    gl_.GenVertexArrays(1,&vao_); gl_.GenBuffers(1,&vbo_);
    gl_.GenVertexArrays(1,&uiVao_); gl_.GenBuffers(1,&uiVbo_);
    if (!vao_ || !vbo_ || !uiVao_ || !uiVbo_) throw std::runtime_error("Could not allocate mesh buffers.");
    const auto layout = [this](GLuint vao,GLuint vbo) {
        gl_.BindVertexArray(vao); gl_.BindBuffer(GL_ARRAY_BUFFER,vbo);
        gl_.EnableVertexAttribArray(0);
        gl_.VertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,sizeof(Vertex),nullptr);
        gl_.EnableVertexAttribArray(1);
        gl_.VertexAttribPointer(1,1,GL_FLOAT,GL_FALSE,sizeof(Vertex),
                               reinterpret_cast<const void*>(offsetof(Vertex,shade)));
        gl_.EnableVertexAttribArray(2);
        gl_.VertexAttribPointer(2,2,GL_FLOAT,GL_FALSE,sizeof(Vertex),reinterpret_cast<const void*>(offsetof(Vertex,u)));
    };
    layout(vao_,vbo_);
    gl_.BufferData(GL_ARRAY_BUFFER,static_cast<GLsizeiptr>(vertices.size()*sizeof(Vertex)),vertices.data(),GL_STATIC_DRAW);
    layout(uiVao_,uiVbo_);
    gl_.BindVertexArray(0);
    // Small textures load only when a front/back is actually visible. Keep
    // one separate full-resolution inspection texture instead of 302 large ones.
    textures_.resize(definitions_.size()+1,0);
    gl_.ActiveTexture(GL_TEXTURE0);
    gl_.UseProgram(program_);
    gl_.Uniform1i(gl_.GetUniformLocation(program_,"imageTexture"),0);
    gl_.Uniform1i(useTextureLocation_,0);
    glEnable(GL_DEPTH_TEST);
}
void Renderer::shutdown() noexcept {
    for(auto& texture:faceTextures_)if(texture){glDeleteTextures(1,&texture);texture=0;}
    if(detailTexture_) {glDeleteTextures(1,&detailTexture_);detailTexture_=0;}
    detailDefinition_.reset();
    if(!textures_.empty()) {glDeleteTextures(static_cast<GLsizei>(textures_.size()),textures_.data());textures_.clear();}
    if (uiVbo_) { gl_.DeleteBuffers(1,&uiVbo_); uiVbo_=0; }
    if (uiVao_) { gl_.DeleteVertexArrays(1,&uiVao_); uiVao_=0; }
    if (vbo_) { gl_.DeleteBuffers(1, &vbo_); vbo_ = 0; }
    if (vao_) { gl_.DeleteVertexArrays(1, &vao_); vao_ = 0; }
    if (program_) { gl_.DeleteProgram(program_); program_ = 0; }
}
void Renderer::mesh(const glm::mat4& vp,Shape shape,glm::vec3 position,glm::vec3 scale,
                    glm::vec3 color,float rotation) {
    gl_.Uniform1i(useTextureLocation_,0);
    auto model=glm::translate(glm::mat4{1.0F},position);
    model=glm::rotate(model,rotation,glm::vec3{0,1,0});
    model=glm::scale(model,scale);
    const auto mvp=vp*model;
    gl_.UniformMatrix4fv(mvpLocation_,1,GL_FALSE,glm::value_ptr(mvp));
    gl_.Uniform3fv(tintLocation_,1,glm::value_ptr(color));
    const GLint first=shape==Shape::Sphere ? sphereFirst_ : (shape==Shape::Pawn ? pawnFirst_ : 0);
    const GLsizei count=shape==Shape::Sphere ? sphereCount_ : (shape==Shape::Pawn ? pawnCount_ : 36);
    glDrawArrays(GL_TRIANGLES,first,count);
}
void Renderer::drawPawn(const glm::mat4& vp,const Player& player,bool guest) {
    const auto p=player.position();
    const float yaw=player.yaw(),bottom=player.seated() ? 0.64F : 0.025F;
    const auto local = [&](float x,float y,float z) {
        return glm::vec3{p.x+x*std::cos(yaw)+z*std::sin(yaw),bottom+y,
                         p.z-x*std::sin(yaw)+z*std::cos(yaw)};
    };
    if (!player.seated()) mesh(vp,Shape::Sphere,{p.x,0.027F,p.z},{0.82F,0.014F,0.82F},{0.19F,0.13F,0.20F});
    mesh(vp,Shape::Pawn,local(0,0,0),{1,1,1},guest?glm::vec3{0.35F,0.62F,0.83F}:glm::vec3{0.73F,0.41F,0.65F},yaw);
    mesh(vp,Shape::Sphere,local(0,1.00F,0),{0.68F,0.68F,0.68F},{0.98F,0.78F,0.68F},yaw);
    for(float side:{-1.0F,1.0F})mesh(vp,Shape::Sphere,local(side*0.30F,0.50F,0),{0.13F,0.34F,0.14F},{0.98F,0.78F,0.68F},yaw);
    const GLuint texture=faceTextures_[guest?1:0];
    if(texture) {
        std::vector<Vertex> vertices;
        const auto vertex=[&](float u,float v){const float x=(u*2-1)*0.23F,y=(1-v*2)*0.23F;const auto p=local(x,1+y,std::sqrt(std::max(0.0F,0.34F*0.34F-x*x-y*y))+0.004F);return Vertex{p.x,p.y,p.z,1,u,v};};
        for(int y=0;y<16;++y)for(int x=0;x<16;++x){const float u=x/16.0F,v=y/16.0F,du=1/16.0F;auto a=vertex(u,v),b=vertex(u+du,v),c=vertex(u+du,v+du),d=vertex(u,v+du);vertices.insert(vertices.end(),{a,b,c,a,c,d});}
        gl_.BindVertexArray(uiVao_);gl_.BindBuffer(GL_ARRAY_BUFFER,uiVbo_);gl_.Uniform1i(useTextureLocation_,1);glBindTexture(GL_TEXTURE_2D,texture);gl_.UniformMatrix4fv(mvpLocation_,1,GL_FALSE,glm::value_ptr(vp));const glm::vec3 white{1};gl_.Uniform3fv(tintLocation_,1,glm::value_ptr(white));gl_.BufferData(GL_ARRAY_BUFFER,static_cast<GLsizeiptr>(vertices.size()*sizeof(Vertex)),vertices.data(),GL_STREAM_DRAW);glDrawArrays(GL_TRIANGLES,0,static_cast<GLsizei>(vertices.size()));gl_.Uniform1i(useTextureLocation_,0);gl_.BindVertexArray(vao_);
    } else {
        for(float side:{-1.0F,1.0F})mesh(vp,Shape::Sphere,local(side*0.11F,1.06F,0.306F),{0.065F,0.09F,0.045F},{0.12F,0.09F,0.16F},yaw);
        mesh(vp,Shape::Box,local(0,0.94F,0.329F),{0.11F,0.025F,0.025F},{0.35F,0.12F,0.21F},yaw);
    }
}
GLuint Renderer::uploadImage(const CardImage& image) {
    GLuint texture=0;glGenTextures(1,&texture);glBindTexture(GL_TEXTURE_2D,texture);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA8,image.width,image.height,0,GL_RGBA,GL_UNSIGNED_BYTE,image.rgba.data());
    return texture;
}
CardImage Renderer::sourceImage(std::optional<std::size_t> definition) const {
    if(!definition || *definition>=definitions_.size()) return makeCardImage(nullptr);
    const auto& d=definitions_[*definition];
    for(const auto& root:{cacheRoot_,bundledRoot_}) if(!root.empty() && !d.scryfallId.empty()) {
        for(const auto* extension:{".jpg",".png"})
            if(auto image=loadCardImage(root/d.setId/(d.scryfallId+extension))) return std::move(*image);
    }
    return makeCardImage(&d);
}
GLuint Renderer::cardTexture(std::optional<std::size_t> definition,bool detailed) {
    if(definition && *definition>=definitions_.size()) definition.reset();
    if(detailed && definition) {
        if(!detailTexture_ || detailDefinition_!=definition) {
            const auto image=sourceImage(definition);
            if(detailTexture_) glDeleteTextures(1,&detailTexture_);
            detailTexture_=uploadImage(image);detailDefinition_=definition;
        }
        return detailTexture_;
    }
    const auto index=definition ? *definition:definitions_.size();
    if(!textures_[index]) textures_[index]=uploadImage(resizeCardImage(sourceImage(definition),192,270));
    return textures_[index];
}
void Renderer::cardFace(const glm::mat4& vp,glm::vec3 position,float angle,GLuint texture) {
    auto model=glm::translate(glm::mat4{1},position);
    model=glm::rotate(model,angle,glm::vec3{0,1,0});
    model=glm::scale(model,glm::vec3{layout::cardWidth,1,layout::cardDepth});
    const auto mvp=vp*model;const glm::vec3 white{1};
    gl_.UniformMatrix4fv(mvpLocation_,1,GL_FALSE,glm::value_ptr(mvp));
    gl_.Uniform3fv(tintLocation_,1,glm::value_ptr(white));
    gl_.Uniform1i(useTextureLocation_,1);glBindTexture(GL_TEXTURE_2D,texture);
    glDrawArrays(GL_TRIANGLES,cardFirst_,6);
    gl_.Uniform1i(useTextureLocation_,0);
}
void Renderer::drawCards(const glm::mat4& vp,const Tabletop& table,const TabletopController& controls,int viewer) {
    const auto render=[&](Vec2 position,float top,float angle,float thickness,GLuint texture,bool selected,bool held) {
        if(held) mesh(vp,Shape::Box,{position.x,1.352F,position.z},
                       {layout::cardWidth+0.04F,0.0003F,layout::cardDepth+0.04F},{0.08F,0.15F,0.13F},angle);
        mesh(vp,Shape::Box,{position.x,top-thickness/2,position.z},
             {layout::cardWidth,thickness,layout::cardDepth},{0.22F,0.18F,0.24F},angle);
        if(selected) mesh(vp,Shape::Box,{position.x,top+0.0002F,position.z},
                          {layout::cardWidth+0.026F,0.0002F,layout::cardDepth+0.026F},{0.98F,0.48F,0.69F},angle);
        cardFace(vp,{position.x,top+0.0007F,position.z},angle,texture);
    };
    for(const auto& [id,c]:table.cards()) if(c.zone==CardZone::Table)
        render(c.position,c.height+layout::cardThickness/2,c.angle+(c.tapped?1.57079633F:0),layout::cardThickness,
               cardTexture(table.visibleDefinition(id,viewer)),controls.selected(id),table.dragged(id));
    for(const auto& [id,p]:table.piles()) {
        if(p.cards.empty()) continue;
        const float thickness=layout::cardThickness*static_cast<float>(std::max<std::size_t>(1,p.cards.size()));
        const auto definition=p.cards.empty()?std::nullopt:table.visibleDefinition(p.cards.back(),viewer);
        render(p.position,1.35F+thickness+(table.draggedPile(id)?0.15F:0),p.angle+(p.tapped?1.57079633F:0),thickness,
               cardTexture(definition),controls.selectedPile()==id,table.draggedPile(id));
    }
}
void Renderer::drawHud(int width,int height,const glm::mat4& vp,const Room& room,const Player& player,bool paused,
                       const Tabletop& table,const TabletopController& controls,const ConnectionView* connection) {
    glDisable(GL_DEPTH_TEST);gl_.BindVertexArray(uiVao_);gl_.BindBuffer(GL_ARRAY_BUFFER,uiVbo_);
    const float w=static_cast<float>(width),h=static_cast<float>(height);
    const TableUiLayout ui{w,h};const float s=ui.scale;
    const auto ortho=glm::ortho(0.0F,w,h,0.0F,-1.0F,1.0F);
    const auto batch=[&](const std::vector<Vertex>& vertices,glm::vec3 color) {
        if(vertices.empty()) return;
        gl_.Uniform1i(useTextureLocation_,0);
        gl_.UniformMatrix4fv(mvpLocation_,1,GL_FALSE,glm::value_ptr(ortho));
        gl_.Uniform3fv(tintLocation_,1,glm::value_ptr(color));
        gl_.BufferData(GL_ARRAY_BUFFER,static_cast<GLsizeiptr>(vertices.size()*sizeof(Vertex)),vertices.data(),GL_STREAM_DRAW);
        glDrawArrays(GL_TRIANGLES,0,static_cast<GLsizei>(vertices.size()));
    };
    const auto panel=[&](UiRect r,glm::vec3 color) {std::vector<Vertex> v;appendRectangle(v,r.x,r.y,r.w,r.h);batch(v,color);};
    const auto picture=[&](UiRect r,GLuint texture) {
        const std::vector<Vertex> v{{r.x,r.y,0,1,0,0},{r.x+r.w,r.y,0,1,1,0},{r.x+r.w,r.y+r.h,0,1,1,1},
                                    {r.x,r.y,0,1,0,0},{r.x+r.w,r.y+r.h,0,1,1,1},{r.x,r.y+r.h,0,1,0,1}};
        gl_.Uniform1i(useTextureLocation_,1);glBindTexture(GL_TEXTURE_2D,texture);
        gl_.UniformMatrix4fv(mvpLocation_,1,GL_FALSE,glm::value_ptr(ortho));
        const glm::vec3 white{1};gl_.Uniform3fv(tintLocation_,1,glm::value_ptr(white));
        gl_.BufferData(GL_ARRAY_BUFFER,static_cast<GLsizeiptr>(v.size()*sizeof(Vertex)),v.data(),GL_STREAM_DRAW);
        glDrawArrays(GL_TRIANGLES,0,static_cast<GLsizei>(v.size()));gl_.Uniform1i(useTextureLocation_,0);
    };
    const bool networked=connection && connection->networked;
    const bool active=player.seated() && !room.capacityPreviewEnabled() && !(connection && connection->menu);
    const glm::vec3 dark{0.12F,0.09F,0.15F},white{0.94F,0.85F,0.80F},pink{1,0.68F,0.78F};
    panel({16*s,16*s,900*s,92*s},dark);
    panel(active?ui.handArea():UiRect{16*s,h-100*s,w-32*s,84*s},dark);
    std::vector<Vertex> text;
    appendText(text,"FEMBOYLOVER.EXE / PHASE 4",30*s,28*s,3*s);
    std::string heading="20 WIDE X 10 DEEP / G: CAPACITY PREVIEW / LOCAL TABLETOP";
    if(room.capacityPreviewEnabled()) heading="200-SLOT CAPACITY PREVIEW / G: RETURN TO CARDS";
    if(networked) heading=connection->side==0?"NETWORK ROOM / PINK HOST / F2: MAIN MENU":"NETWORK ROOM / BLUE GUEST / F2: MAIN MENU";
    if(networked&&clubView_)heading+=" / LIFE "+std::to_string(clubView_->match.life[clubSide_])+":"+std::to_string(clubView_->match.life[1-clubSide_]);
    appendText(text,heading,30*s,62*s,2*s);
    if(networked) appendText(text,(active?connection->tableStatus:connection->status).substr(0,75),30*s,84*s,1.5F*s);
    if(active) {
        if(!networked) appendText(text,controls.status().substr(0,68),30*s,84*s,1.5F*s);
        const auto hand=table.hand(controls.viewer());
        const auto other=table.hand(1-controls.viewer());
        const std::string counts="YOUR HAND "+std::to_string(hand.size())+" / OTHER HAND "+std::to_string(other.size())+" / WHEEL OVER HAND TO SCROLL";
        appendText(text,counts,24*s,h-177*s,2*s);
        appendText(text,"T TAP / F FLIP / R ROTATE / H HAND / B PILE / X SPLIT / L LIBRARY TOP / SHIFT L BOTTOM",24*s,h-38*s,1.5F*s);
        appendText(text,"D DRAW / SHIFT D DRAW 7 / S SHUFFLE / SHIFT DROP MERGES PILES / RIGHT-CLICK INSPECT / E STAND",24*s,h-20*s,1.5F*s);
        appendText(text,"[",24*s,h-111*s,3*s);appendText(text,"]",w-40*s,h-111*s,3*s);
        const int maxOffset=std::max(0,static_cast<int>(hand.size())-ui.handSlots());
        const int offset=std::clamp(controls.handOffset(),0,maxOffset);
        for(int i=0;i<ui.handSlots() && offset+i<static_cast<int>(hand.size());++i) {
            const auto id=hand[static_cast<std::size_t>(offset+i)];const auto r=ui.handCard(i);
            if(controls.selected(id)) panel({r.x-3*s,r.y-3*s,r.w+6*s,r.h+6*s},pink);
            picture(r,cardTexture(table.visibleDefinition(id,controls.viewer())));
        }
        for(const auto& [id,p]:table.piles()) {
            if(p.cards.empty()) continue;
            (void)id;
            const auto projected=vp*glm::vec4{p.position.x,1.45F+static_cast<float>(p.cards.size())*layout::cardThickness,p.position.z,1};
            if(projected.w>0) appendText(text,std::to_string(p.cards.size()),(projected.x/projected.w+1)*w/2,
                                       (1-projected.y/projected.w)*h/2-18*s,2*s);
        }
    } else {
        appendText(text,"WASD: WALK / SHIFT: RUN / RIGHT-DRAG: LOOK OR TABLE PAN / WHEEL: ZOOM",30*s,h-82*s,1.8F*s);
        appendText(text,networked?"E: YOUR SEAT / F5: RECONNECT / F2: MENU / ESC: PAUSE":"E: SIT OR STAND / HOME: RESET CAMERA / G: PREVIEW / F2: MENU / ESC: PAUSE",30*s,h-53*s,1.8F*s);
        if(networked) appendText(text,"F3 SHOP / F4 DECKS / F6 FACE / F7 MATCH / E AT STATIONS",30*s,h-28*s,1.6F*s);
        if(!player.seated() && room.nearestFreeSeat(player.position())>=0)
            appendText(text,"PRESS E TO SIT",30*s,h-138*s,3*s);
        else if(!player.seated()) {
            const auto label=room.nearbyStation(player.position());
            if(!label.empty()) appendText(text,label,30*s,h-138*s,2*s);
        }
    }
    batch(text,white);
    if(active && controls.menuOpen()) {
        const auto r=controls.menuRect();panel(r,{0.20F,0.14F,0.24F});text.clear();
        for(int row=0;row<TabletopController::menuRows;++row) {
            text.clear();
            appendText(text,TabletopController::menuLabel(row),r.x+9*s,r.y+(8+static_cast<float>(row)*28)*s,1.5F*s);
            batch(text,controls.menuEnabled(row)?white:glm::vec3{0.45F,0.40F,0.45F});
        }
    }
    if(active) if(const auto inspected=controls.inspected()) {
        const float ih=std::min(h-100*s,720*s),iw=ih*0.7F;
        panel({(w-iw)/2-14*s,30*s,iw+28*s,ih+56*s},dark);
        picture({(w-iw)/2,42*s,iw,ih},cardTexture(table.visibleDefinition(*inspected,controls.viewer()),true));
        text.clear();appendText(text,controls.inspectionCaption(),(w-iw)/2,ih+56*s,1.5F*s);batch(text,white);
    }
    if(paused) {
        panel({w/2-240*s,h/2-60*s,480*s,120*s},dark);text.clear();
        appendText(text,"PAUSED",w/2-54*s,h/2-34*s,3*s);
        appendText(text,"ESC: RESUME / Q: QUIT",w/2-120*s,h/2+10*s,2*s);batch(text,pink);
    }
    if(connection && connection->menu) {
        const float x=std::max(12*s,(w-880*s)/2),y=std::max(12*s,(h-420*s)/2);
        panel({x,y,880*s,420*s},dark);text.clear();
        appendText(text,"FEMBOYLOVER.EXE",x+30*s,y+28*s,4*s);
        appendText(text,"PHASE 4 / BRING YOUR OWN BOYFRIEND",x+30*s,y+78*s,2*s);
        if(connection->editingAddress) {
            appendText(text,"HOST ADDRESS / IPV4 OR HOSTNAME / UDP 33445",x+30*s,y+145*s,2*s);
            appendText(text,connection->address.substr(connection->address.size()>45?connection->address.size()-45:0),x+30*s,y+188*s,2.5F*s);
            appendText(text,"ENTER: CONNECT / CTRL V: PASTE / CTRL A: CLEAR",x+30*s,y+250*s,2*s);
        } else {
            appendText(text,"H: HOST ROOM / J: JOIN ROOM",x+30*s,y+145*s,2.5F*s);
            appendText(text,"L: LOCAL TABLETOP / Q: EXIT",x+30*s,y+192*s,2.5F*s);
            appendText(text,"TWO PCS: USE HOST TAILSCALE IPV4 ADDRESS",x+30*s,y+250*s,2*s);
            appendText(text,"SAME PC: JOIN 127.0.0.1 IN A SECOND WINDOW",x+30*s,y+282*s,2*s);
        }
        appendText(text,connection->status.substr(0,65),x+30*s,y+332*s,1.8F*s);
        appendText(text,"HOST AUTHORITY / PRIVATE HANDS / SHARED TABLETOP",x+30*s,y+374*s,1.5F*s);batch(text,pink);
    }
    gl_.BindVertexArray(0);glEnable(GL_DEPTH_TEST);
}
void Renderer::updateFaces() {
    if(!clubView_)return;
    for(int side=0;side<2;++side){const auto& face=clubUi_&&clubUi_->open&&clubUi_->page==ClubPage::Face&&side==clubSide_?clubUi_->canvas:clubView_->faces[side];if(face.size()!=facePixels||face==cachedFaces_[side])continue;CardImage image;image.width=256;image.height=256;image.rgba.resize(facePixels*4);for(int i=0;i<facePixels;++i){const auto& color=paintColors[std::min<int>(face[static_cast<std::size_t>(i)],15)];for(int j=0;j<3;++j)image.rgba[static_cast<std::size_t>(i*4+j)]=color[j];image.rgba[static_cast<std::size_t>(i*4+3)]=255;}if(faceTextures_[side])glDeleteTextures(1,&faceTextures_[side]);faceTextures_[side]=uploadImage(image);cachedFaces_[side]=face;}
}
void Renderer::drawClub(int width,int height) {
    if(!clubView_||!clubUi_||!clubUi_->open)return;
    const auto& ui=*clubUi_;const auto& view=*clubView_;const float s=ui.scale(),left=ui.left(),top=ui.top();
    glDisable(GL_DEPTH_TEST);gl_.BindVertexArray(uiVao_);gl_.BindBuffer(GL_ARRAY_BUFFER,uiVbo_);const auto ortho=glm::ortho(0.0F,static_cast<float>(width),static_cast<float>(height),0.0F,-1.0F,1.0F);
    const auto batch=[&](const std::vector<Vertex>& v,glm::vec3 color){if(v.empty())return;gl_.Uniform1i(useTextureLocation_,0);gl_.UniformMatrix4fv(mvpLocation_,1,GL_FALSE,glm::value_ptr(ortho));gl_.Uniform3fv(tintLocation_,1,glm::value_ptr(color));gl_.BufferData(GL_ARRAY_BUFFER,static_cast<GLsizeiptr>(v.size()*sizeof(Vertex)),v.data(),GL_STREAM_DRAW);glDrawArrays(GL_TRIANGLES,0,static_cast<GLsizei>(v.size()));};
    const auto rect=[&](UiRect r,glm::vec3 color){std::vector<Vertex>v;appendRectangle(v,left+r.x*s,top+r.y*s,r.w*s,r.h*s);batch(v,color);};
    const auto text=[&](const std::string& t,float x,float y,float size=2.0F){std::vector<Vertex>v;appendText(v,t,left+x*s,top+y*s,size*s);batch(v,{0.96F,0.89F,0.85F});};
    const auto picture=[&](UiRect r,GLuint texture){const float x=left+r.x*s,y=top+r.y*s,w=r.w*s,h=r.h*s;const std::vector<Vertex>v{{x,y,0,1,0,0},{x+w,y,0,1,1,0},{x+w,y+h,0,1,1,1},{x,y,0,1,0,0},{x+w,y+h,0,1,1,1},{x,y+h,0,1,0,1}};gl_.Uniform1i(useTextureLocation_,1);glBindTexture(GL_TEXTURE_2D,texture);gl_.UniformMatrix4fv(mvpLocation_,1,GL_FALSE,glm::value_ptr(ortho));const glm::vec3 white{1};gl_.Uniform3fv(tintLocation_,1,glm::value_ptr(white));gl_.BufferData(GL_ARRAY_BUFFER,static_cast<GLsizeiptr>(v.size()*sizeof(Vertex)),v.data(),GL_STREAM_DRAW);glDrawArrays(GL_TRIANGLES,0,6);};
    rect({12,12,1256,696},{0.10F,0.08F,0.14F});
    const auto cents=view.profile.cents;const auto money=std::to_string(cents/100)+"."+(cents%100<10?"0":"")+std::to_string(cents%100);
    text("CLUBHOUSE / "+money+" COINS / SELECTED: "+view.profile.selected,32,27,2.4F);
    for(const auto& b:ui.buttons(view)){rect(b.rect,{0.26F,0.17F,0.30F});text(b.label,b.rect.x+10,b.rect.y+13,1.7F);}
    text(view.status.substr(0,96),32,652,1.8F);if(!ui.notice.empty())text(ui.notice.substr(0,96),32,679,1.8F);
    if(ui.page==ClubPage::Shop){text("ONE CENT PER BASIC LAND",36,520,1.7F);text("LATEST PACK / PURCHASE",350,130);for(std::size_t i=0;i<view.opened.size();++i){const auto d=view.opened[i];if(d<definitions_.size())text(definitions_[d].name.substr(0,35)+" / "+definitions_[d].rarity,350,170+static_cast<float>(i)*28,1.6F);}const int preview=ui.selected>=0?ui.selected:(view.opened.empty()?-1:view.opened.front());if(preview>=0&&preview<static_cast<int>(definitions_.size()))picture({950,170,245,345},cardTexture(static_cast<std::size_t>(preview),true));}
    if(ui.page==ClubPage::Decks){rect({36,118,530,36},{0.19F,0.15F,0.24F});text("SEARCH: "+ui.query.substr(0,38),45,130,1.7F);rect({620,165,610,38},ui.naming?glm::vec3{0.36F,0.21F,0.34F}:glm::vec3{0.20F,0.16F,0.25F});text("NAME: "+ui.name,630,178,1.7F);const auto list=ui.filtered(view,definitions_);const int start=std::clamp(ui.offset,0,std::max(0,static_cast<int>(list.size())-12));for(int i=0;i<12&&start+i<static_cast<int>(list.size());++i){const int d=list[start+i];if(d==ui.selected)rect({36,165+34.0F*i,530,32},{0.32F,0.21F,0.34F});const auto count=std::count(ui.draft.begin(),ui.draft.end(),d);text(std::to_string(view.profile.owned[d])+" OWN / "+std::to_string(count)+" DECK / "+definitions_[d].name.substr(0,27),44,176+34.0F*i,1.6F);}text("DRAFT: "+std::to_string(ui.draft.size())+" / 120",620,550,1.6F);text("CLICK NAME TO EDIT",620,578,1.5F);text("TYPE TO SEARCH / CTRL+A TO CLEAR FIELD",36,632,1.5F);if(ui.selected>=0&&ui.selected<static_cast<int>(definitions_.size()))picture({935,220,245,345},cardTexture(static_cast<std::size_t>(ui.selected),true));}
    if(ui.page==ClubPage::Face){picture({36,160,400,400},faceTextures_[clubSide_]);for(int i=0;i<16;++i){const auto& c=paintColors[i];rect({500+static_cast<float>(i%8)*35,170+static_cast<float>(i/8)*45,30,35},{c[0]/255.0F,c[1]/255.0F,c[2]/255.0F});}text("BRUSH: "+std::to_string(ui.brush)+(ui.eraser?" / ERASER":(ui.bucket?" / FILL":" / PENCIL")),500,265,1.8F);text("LIVE ORB PREVIEW",850,145);text("ESC CANCELS UNSAVED EDITS",36,590,1.7F);
        const int x=static_cast<int>(left+830*s),y=height-static_cast<int>(top+560*s),w=static_cast<int>(370*s);glViewport(x,y,w,w);glEnable(GL_DEPTH_TEST);glClear(GL_DEPTH_BUFFER_BIT);gl_.BindVertexArray(vao_);Player preview;preview.applyNetworkState({0,0},0,-1);const auto vp=glm::perspective(0.53F,1.0F,0.1F,10.0F)*glm::lookAt(glm::vec3{0,1.03F,1.55F},glm::vec3{0,1.03F,0},glm::vec3{0,1,0});drawPawn(vp,preview,clubSide_==1);glViewport(0,0,width,height);glDisable(GL_DEPTH_TEST);gl_.BindVertexArray(uiVao_);gl_.BindBuffer(GL_ARRAY_BUFFER,uiVbo_);
    }
    if(ui.page==ClubPage::Match){text(view.match.active?"MATCH IN PROGRESS":"SELECT DECKS, THEN BOTH PRESS READY",50,150,2.3F);for(int side=0;side<2;++side){const std::string player=side==0?"PINK HOST":"BLUE GUEST";text(player+" / LIFE "+std::to_string(view.match.life[side]),530,220+90.0F*side,2.8F);text(std::string(view.match.ready[side]?"READY":"NOT READY")+" / WINNER VOTE "+(view.match.votes[side]<0?"NONE":(view.match.votes[side]==0?"PINK":"BLUE")),530,254+90.0F*side,1.8F);}if(view.match.winner>=0)text("AGREED WINNER: "+std::string(view.match.winner==0?"PINK HOST":"BLUE GUEST"),530,450,2.2F);text("RULES AND CARD EFFECTS ARE MANUAL",530,520,1.8F);if(view.match.reset[0]||view.match.reset[1])text(std::string{"RESET: PINK "}+(view.match.reset[0]?"YES":"NO")+" / BLUE "+(view.match.reset[1]?"YES":"NO"),530,565,1.8F);}
    gl_.BindVertexArray(0);glEnable(GL_DEPTH_TEST);
}
void Renderer::draw(int width,int height,const glm::mat4& vp,const Room& room,const Player& player,bool paused,
                    const Tabletop& table,const TabletopController& controls,const Player* remote,const ConnectionView* connection) {
    updateFaces();
    glViewport(0,0,width,height);glClearColor(0.12F,0.09F,0.16F,1);
    glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
    gl_.UseProgram(program_);gl_.ActiveTexture(GL_TEXTURE0);gl_.BindVertexArray(vao_);
    for(const auto& o:room.objects()) mesh(vp,o.shape,{o.position.x,o.position.y,o.position.z},
        {o.size.x,o.size.y,o.size.z},{o.color.x,o.color.y,o.color.z},o.yaw);
    if(room.capacityPreviewEnabled()) {
        for(const auto& o:room.displayCards()) mesh(vp,o.shape,{o.position.x,o.position.y,o.position.z},
            {o.size.x,o.size.y,o.size.z},{o.color.x,o.color.y,o.color.z},o.yaw);
    } else drawCards(vp,table,controls,player.seated()?player.seat():-1);
    drawPawn(vp,player,connection && connection->networked && connection->side==1);
    if(remote) drawPawn(vp,*remote,connection && connection->side==0);
    drawHud(width,height,vp,room,player,paused,table,controls,connection);
    drawClub(width,height);
}
}
