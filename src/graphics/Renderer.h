#pragma once
#include "graphics/GlApi.h"
#include "world/Player.h"
#include "ui/ConnectionView.h"
#include "cards/TabletopController.h"
#include "club/ClubUi.h"
#include <glm/glm.hpp>
#include <filesystem>

namespace fl {
struct CardImage;
class Renderer {
public:
    Renderer() = default;
    Renderer(const Renderer&) = delete;
    Renderer& operator=(const Renderer&) = delete;
    void initialize(const Tabletop& table,const std::filesystem::path& cacheRoot={},const std::filesystem::path& bundledRoot={});
    void shutdown() noexcept;
    void setClub(const ClubView* view,const ClubUi* ui,int side) {clubView_=view;clubUi_=ui;clubSide_=side;}
    void draw(int width,int height,const glm::mat4& viewProjection,
              const Room& room,const Player& player,bool paused,const Tabletop& table,const TabletopController& controls,const Player* remote=nullptr,const ConnectionView* connection=nullptr);
private:
    void updateFaces();
    void drawClub(int width,int height);
    const ClubView* clubView_=nullptr;
    const ClubUi* clubUi_=nullptr;
    int clubSide_=0;
    std::array<GLuint,2> faceTextures_{};
    std::array<Face,2> cachedFaces_;
    GLuint compile(GLenum type,const char* source);
    void mesh(const glm::mat4& vp,Shape shape,glm::vec3 position,glm::vec3 scale,
              glm::vec3 color,float rotation=0.0F);
    void drawPawn(const glm::mat4& vp,const Player& player,bool guest=false);
    void drawCards(const glm::mat4& vp,const Tabletop& table,const TabletopController& controls,int viewer);
    void cardFace(const glm::mat4& vp,glm::vec3 position,float angle,GLuint texture);
    GLuint cardTexture(std::optional<std::size_t> definition,bool detailed=false);
    GLuint uploadImage(const CardImage& image);
    CardImage sourceImage(std::optional<std::size_t> definition) const;
    void drawHud(int width,int height,const glm::mat4& vp,const Room& room,const Player& player,bool paused,
                 const Tabletop& table,const TabletopController& controls,const ConnectionView* connection);
    GlApi gl_;
    GLuint program_=0,vao_=0,vbo_=0,uiVao_=0,uiVbo_=0;
    GLint mvpLocation_=-1,tintLocation_=-1,useTextureLocation_=-1;
    std::vector<GLuint> textures_;
    std::vector<CardDefinition> definitions_;
    std::filesystem::path cacheRoot_,bundledRoot_;
    GLuint titleTexture_=0;
    GLuint detailTexture_=0;
    std::optional<std::size_t> detailDefinition_;
    GLint cardFirst_=0;
    GLint sphereFirst_=0,pawnFirst_=0;
    GLsizei sphereCount_=0,pawnCount_=0;
};
}
