#pragma once
#include "graphics/GlApi.h"
#include "world/Player.h"
#include <glm/glm.hpp>

namespace fl {
class Renderer {
public:
    Renderer() = default;
    Renderer(const Renderer&) = delete;
    Renderer& operator=(const Renderer&) = delete;
    void initialize();
    void shutdown() noexcept;
    void draw(int width,int height,const glm::mat4& viewProjection,
              const Room& room,const Player& player,bool paused);
private:
    GLuint compile(GLenum type,const char* source);
    void mesh(const glm::mat4& vp,Shape shape,glm::vec3 position,glm::vec3 scale,
              glm::vec3 color,float rotation=0.0F);
    void drawPawn(const glm::mat4& vp,const Player& player);
    void drawHud(int width,int height,const Room& room,const Player& player,bool paused);
    GlApi gl_;
    GLuint program_=0,vao_=0,vbo_=0,uiVao_=0,uiVbo_=0;
    GLint mvpLocation_=-1,tintLocation_=-1;
    GLint sphereFirst_=0,pawnFirst_=0;
    GLsizei sphereCount_=0,pawnCount_=0;
};
}
