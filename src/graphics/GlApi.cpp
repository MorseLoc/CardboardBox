#include "graphics/GlApi.h"
#include <SDL3/SDL.h>
#include <stdexcept>
#include <string>

namespace fl {
namespace {
template<typename T> void loadEntry(T& entry, const char* name) {
    entry = reinterpret_cast<T>(SDL_GL_GetProcAddress(name));
    if (!entry) throw std::runtime_error(std::string{"Missing OpenGL function: "} + name);
}
}
void GlApi::load() {
#define FL_LOAD(name) loadEntry(name, "gl" #name)
    FL_LOAD(GenVertexArrays); FL_LOAD(BindVertexArray); FL_LOAD(DeleteVertexArrays);
    FL_LOAD(GenBuffers); FL_LOAD(BindBuffer); FL_LOAD(BufferData); FL_LOAD(DeleteBuffers);
    FL_LOAD(EnableVertexAttribArray); FL_LOAD(VertexAttribPointer);
    FL_LOAD(CreateShader); FL_LOAD(ShaderSource); FL_LOAD(CompileShader);
    FL_LOAD(GetShaderiv); FL_LOAD(GetShaderInfoLog); FL_LOAD(DeleteShader);
    FL_LOAD(CreateProgram); FL_LOAD(AttachShader); FL_LOAD(LinkProgram);
    FL_LOAD(GetProgramiv); FL_LOAD(GetProgramInfoLog); FL_LOAD(DeleteProgram);
    FL_LOAD(UseProgram); FL_LOAD(GetUniformLocation);
    FL_LOAD(UniformMatrix4fv); FL_LOAD(Uniform3fv);
#undef FL_LOAD
}
}
