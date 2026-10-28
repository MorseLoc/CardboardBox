#include "graphics/Renderer.h"
#include "graphics/BitmapFont.h"
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
uniform mat4 mvp;
out float faceShade;
void main() {
    gl_Position = mvp * vec4(position, 1.0);
    faceShade = shade;
}
)";
constexpr const char* fragmentSource = R"(
#version 330 core
in float faceShade;
uniform vec3 tint;
out vec4 color;
void main() { color = vec4(tint * faceShade, 1.0); }
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
void Renderer::initialize() {
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
    if (mvpLocation_ < 0 || tintLocation_ < 0) throw std::runtime_error("Missing shader uniform.");

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
    };
    layout(vao_,vbo_);
    gl_.BufferData(GL_ARRAY_BUFFER,static_cast<GLsizeiptr>(vertices.size()*sizeof(Vertex)),vertices.data(),GL_STATIC_DRAW);
    layout(uiVao_,uiVbo_);
    gl_.BindVertexArray(0);
    glEnable(GL_DEPTH_TEST);
}
void Renderer::shutdown() noexcept {
    if (uiVbo_) { gl_.DeleteBuffers(1,&uiVbo_); uiVbo_=0; }
    if (uiVao_) { gl_.DeleteVertexArrays(1,&uiVao_); uiVao_=0; }
    if (vbo_) { gl_.DeleteBuffers(1, &vbo_); vbo_ = 0; }
    if (vao_) { gl_.DeleteVertexArrays(1, &vao_); vao_ = 0; }
    if (program_) { gl_.DeleteProgram(program_); program_ = 0; }
}
void Renderer::mesh(const glm::mat4& vp,Shape shape,glm::vec3 position,glm::vec3 scale,
                    glm::vec3 color,float rotation) {
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
void Renderer::drawPawn(const glm::mat4& vp,const Player& player) {
    const auto p=player.position();
    const float yaw=player.yaw(),bottom=player.seated() ? 0.64F : 0.025F;
    const auto local = [&](float x,float y,float z) {
        return glm::vec3{p.x+x*std::cos(yaw)+z*std::sin(yaw),bottom+y,
                         p.z-x*std::sin(yaw)+z*std::cos(yaw)};
    };
    if (!player.seated()) mesh(vp,Shape::Sphere,{p.x,0.027F,p.z},{0.82F,0.014F,0.82F},{0.19F,0.13F,0.20F});
    mesh(vp,Shape::Pawn,local(0,0,0),{1,1,1},{0.73F,0.41F,0.65F},yaw);
    mesh(vp,Shape::Sphere,local(0,1.00F,0),{0.68F,0.68F,0.68F},{0.98F,0.78F,0.68F},yaw);
    for (float side : {-1.0F,1.0F}) {
        mesh(vp,Shape::Sphere,local(side*0.11F,1.06F,0.306F),{0.065F,0.09F,0.045F},{0.12F,0.09F,0.16F},yaw);
        mesh(vp,Shape::Sphere,local(side*0.20F,0.97F,0.266F),{0.09F,0.04F,0.04F},{0.93F,0.43F,0.52F},yaw);
        mesh(vp,Shape::Sphere,local(side*0.30F,0.50F,0),{0.13F,0.34F,0.14F},{0.98F,0.78F,0.68F},yaw);
    }
    mesh(vp,Shape::Box,local(0,0.94F,0.329F),{0.11F,0.025F,0.025F},{0.35F,0.12F,0.21F},yaw);
}
void Renderer::drawHud(int width,int height,const Room& room,const Player& player,bool paused) {
    glDisable(GL_DEPTH_TEST);
    gl_.BindVertexArray(uiVao_);
    gl_.BindBuffer(GL_ARRAY_BUFFER,uiVbo_);
    const float w=static_cast<float>(width),h=static_cast<float>(height);
    const float scale=std::max(0.65F,std::min(w/1280.0F,h/720.0F));
    const auto ortho=glm::ortho(0.0F,w,h,0.0F,-1.0F,1.0F);
    gl_.UniformMatrix4fv(mvpLocation_,1,GL_FALSE,glm::value_ptr(ortho));
    const auto batch = [&](const std::vector<Vertex>& vertices,glm::vec3 color) {
        if (vertices.empty()) return;
        gl_.Uniform3fv(tintLocation_,1,glm::value_ptr(color));
        gl_.BufferData(GL_ARRAY_BUFFER,static_cast<GLsizeiptr>(vertices.size()*sizeof(Vertex)),vertices.data(),GL_STREAM_DRAW);
        glDrawArrays(GL_TRIANGLES,0,static_cast<GLsizei>(vertices.size()));
    };
    std::vector<Vertex> panels;
    appendRectangle(panels,16*scale,16*scale,640*scale,104*scale);
    appendRectangle(panels,16*scale,h-100*scale,w-32*scale,84*scale);
    if (paused) appendRectangle(panels,w/2-240*scale,h/2-60*scale,480*scale,120*scale);
    batch(panels,{0.12F,0.09F,0.15F});
    std::vector<Vertex> text;
    appendText(text,"FEMBOYLOVER.EXE / PHASE 1.1",30*scale,30*scale,3*scale);
    appendText(text,player.seated() ? "TABLE VIEW - ONE LOCAL PLAYER" : "THE CLUBHOUSE - ONE LOCAL PLAYER",30*scale,64*scale,2*scale);
    appendText(text,room.capacityPreviewEnabled() ? "G: HIDE PREVIEW / 20 X 20 SLOTS / 400 PLACEHOLDERS" : "G: SHOW 20 X 20 CARD-CAPACITY PREVIEW",30*scale,90*scale,2*scale);
    const float bottom=h-84*scale;
    if (player.seated()) {
        appendText(text,"E OR ESC: STAND   RIGHT-DRAG: PAN   WHEEL: ZOOM",30*scale,bottom,2*scale);
        appendText(text,"HOME: RESET VIEW   SAMPLE CARDS ARE DECORATIVE",30*scale,bottom+26*scale,2*scale);
    } else {
        appendText(text,"WASD: WALK   SHIFT: RUN   RIGHT-DRAG: LOOK   WHEEL: ZOOM",30*scale,bottom,2*scale);
        appendText(text,"E: SIT NEAR A CHAIR   HOME: RESET CAMERA   ESC: PAUSE",30*scale,bottom+26*scale,2*scale);
    }
    batch(text,{0.94F,0.85F,0.80F});
    std::vector<Vertex> accent;
    if (paused) {
        appendText(accent,"PAUSED",w/2-54*scale,h/2-34*scale,3*scale);
        appendText(accent,"ESC: RESUME   Q: QUIT",w/2-120*scale,h/2+10*scale,2*scale);
    } else if (!player.seated()) {
        if (room.nearestFreeSeat(player.position())>=0)
            appendText(accent,"PRESS E TO SIT",30*scale,bottom-38*scale,3*scale);
        else {
            const auto label=room.nearbyStation(player.position());
            if (!label.empty()) appendText(accent,label,30*scale,bottom-32*scale,2*scale);
        }
    }
    batch(accent,{1.0F,0.71F,0.79F});
    gl_.BindVertexArray(0);
    glEnable(GL_DEPTH_TEST);
}
void Renderer::draw(int width,int height,const glm::mat4& vp,const Room& room,const Player& player,bool paused) {
    glViewport(0,0,width,height);
    glClearColor(0.12F,0.09F,0.16F,1.0F);
    glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
    gl_.UseProgram(program_); gl_.BindVertexArray(vao_);
    for (const auto& o : room.objects())
        mesh(vp,o.shape,{o.position.x,o.position.y,o.position.z},{o.size.x,o.size.y,o.size.z},
              {o.color.x,o.color.y,o.color.z},o.yaw);
    for (const auto& o : room.displayCards())
        mesh(vp,o.shape,{o.position.x,o.position.y,o.position.z},{o.size.x,o.size.y,o.size.z},
             {o.color.x,o.color.y,o.color.z},o.yaw);
    drawPawn(vp,player);
    drawHud(width,height,room,player,paused);
}
}
