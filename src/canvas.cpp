#include "engine/canvas.h"
#include "engine/engine.h"
#include "engine/color.inl"
#include "engine/math.inl"
#include "engine/shader.h"

#pragma warning(push, 0)
#include <GLFW/glfw3.h>
#include <glad/glad.h>
#include <array.h>
#include <memory.h>
#pragma warning(pop)

using engine::Engine;
using engine::Canvas;
using foundation::Allocator;
using math::Color4f;

namespace {
const char *vertex_source = R"(
#version 410 core

layout (location = 0) in vec3 in_position;
layout (location = 1) in vec3 in_color;
layout (location = 2) in vec2 in_texture_coords;

smooth out vec3 col;

void main() {
    gl_Position = vec4(in_position.x, in_position.y, 0.0, 1.0);
    col = in_color;
}
)";

const char *fragment_source = R"(
#version 410 core

precision highp float;

in vec3 col;
out vec4 out_color;

void main() {
//    out_color = texture(texture0, uv);
//    out_color = vec4(col, 1.0);
    out_color = vec4(1.0, 0.0, 0.0, 1.0);
}
)";
}

namespace engine {

Canvas::Canvas(Allocator &allocator)
: allocator(allocator)
, col(engine::color::black)
, shader(nullptr)
, texture(0)
, vbo()
, vao(0)
, data(allocator)
{
    glGenTextures(1, &texture);

    const Vertex vertices[] = {

    };

    const size_t vertex_count = 4;
    const size_t vertex_data_size = sizeof(Vertex);

    const GLfloat vertices[] = {
    //   positions     texture coordinates
        -1.0f,  1.0f,  0.0f, 1.0f,
        -1.0f, -1.0f,  0.0f, 0.0f,
         1.0f, -1.0f,  1.0f, 0.0f,

        -1.0f,  1.0f,  0.0f, 1.0f,
         1.0f, -1.0f,  1.0f, 0.0f,
         1.0f,  1.0f,  1.0f, 1.0f
    };

    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);

    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, 8 * sizeof(GLfloat), vertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(0);



}

Canvas::~Canvas() {
    MAKE_DELETE(allocator, Shader, shader);
    glDeleteBuffers(2, vbo);
    glDeleteVertexArrays(1, &vao);
}

void init_canvas(const Engine &engine, Canvas &canvas) {
    // glBindTexture(GL_TEXTURE_2D, canvas.texture);
    // glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    // glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    // glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    // glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    // glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, engine.window_rect.size.x, engine.window_rect.size.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, array::begin(canvas.data));
    // glBindTexture(GL_TEXTURE_2D, 0);
}

void render_canvas(const Engine &engine, Canvas &canvas) {
    const GLuint shader_program = canvas.shader->program;

   glUseProgram(shader_program);
//    glActiveTexture(GL_TEXTURE0);
//    glBindTexture(GL_TEXTURE_2D_ARRAY, canvas.texture);
//    glUniform1i(glGetUniformLocation(shader_program, "texture0"), 0);

    glBindVertexArray(canvas.vao);
    glDrawArrays(GL_LINE_LOOP, 0, 4);
//    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, (void *)0);
    glBindVertexArray(0);
}

void canvas::pset(Canvas &canvas, int x, int y) {
    canvas::pset(canvas, x, y, canvas.col);
}

void canvas::pset(Canvas &canvas, int x, int y, Color4f col) {
}

} // namespace engine
