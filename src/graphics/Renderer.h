#pragma once
#include "graphics/GlApi.h"
#include "world/Player.h"
#include "cards/TabletopController.h"
#include <glm/glm.hpp>

namespace fl {
class Renderer {
public:
    Renderer() = default;
    Renderer(const Renderer&) = delete;
    Renderer& operator=(const Renderer&) = delete;
    void initialize(const Tabletop& table);
    void shutdown() noexcept;
    void draw(int width,int height,const glm::mat4& viewProjection,
              const Room& room,const Player& player,bool paused,const Tabletop& table,const TabletopController& controls);
private:
    GLuint compile(GLenum type,const char* source);
    void mesh(const glm::mat4& vp,Shape shape,glm::vec3 position,glm::vec3 scale,
              glm::vec3 color,float rotation=0.0F);
    void drawPawn(const glm::mat4& vp,const Player& player);
    void drawCards(const glm::mat4& vp,const Tabletop& table,const TabletopController& controls,int viewer);
    void cardFace(const glm::mat4& vp,glm::vec3 position,float angle,GLuint texture);
    GLuint cardTexture(std::optional<std::size_t> definition) const;
    void drawHud(int width,int height,const glm::mat4& vp,const Room& room,const Player& player,bool paused,
                 const Tabletop& table,const TabletopController& controls);
    GlApi gl_;
    GLuint program_=0,vao_=0,vbo_=0,uiVao_=0,uiVbo_=0;
    GLint mvpLocation_=-1,tintLocation_=-1,useTextureLocation_=-1;
    std::vector<GLuint> textures_;
    GLint cardFirst_=0;
    GLint sphereFirst_=0,pawnFirst_=0;
    GLsizei sphereCount_=0,pawnCount_=0;
};
}
