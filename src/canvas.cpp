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
#include <cassert>

#include <glm/glm.hpp>
#pragma warning(pop)

using engine::Engine;
using engine::Canvas;
using foundation::Allocator;
using math::Color4f;

namespace {
const char *vertex_source = R"(
#version 410 core

layout (location = 0) in vec3 in_position;
layout (location = 1) in vec2 in_texture_coords;

smooth out vec2 uv;

void main() {
    gl_Position = vec4(in_position.x, in_position.y, 0.0, 1.0);
    uv = in_texture_coords;
}
)";

const char *fragment_source = R"(
#version 410 core

precision highp float;

uniform sampler2D texture0;
in vec2 uv;

out vec4 out_color;

void main() {
    out_color = texture(texture0, uv);
}
)";

math::Vertex vertices[] {
    { // top right
        {1.0f, 1.0f, 0.0f},
        {1.0f, 1.0f, 1.0f, 1.0f},
        {1.0f, 1.0f}
    },
    { // bottom right
        {1.0f, -1.0f, 0.0f},
        {1.0f, 1.0f, 1.0f, 1.0f},
        {1.0f, 0.0f}
    },
    { // bottom left
        {-1.0f, -1.0f, 0.0f},
        {1.0f, 1.0f, 1.0f, 1.0f},
        {0.0f, .0f}
    },
    { // top left
        {-1.0f, 1.0f, 0.0f},
        {1.0f, 1.0f, 1.0f, 1.0f},
        {0.0f, 1.0f}
    },
};

GLuint indices[] = {
    0, 1, 3,  // first Triangle
    1, 2, 3   // second Triangle
};

}

namespace engine {

Canvas::Canvas(Allocator &allocator)
: allocator(allocator)
, col(engine::color::black)
, shader(nullptr)
, texture(0)
, vao(0)
, vbo(0)
, ebo(0)
, data(allocator)
{
    shader = MAKE_NEW(allocator, Shader, nullptr, vertex_source, fragment_source);

    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glGenBuffers(1, &ebo);
    glGenTextures(1, &texture);

    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, 4 * sizeof(math::Vertex), vertices, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, 6 * sizeof(GLuint), indices, GL_STATIC_DRAW);

    // position
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(math::Vertex), (const GLvoid *)0);
    glEnableVertexAttribArray(0);

    // texture_coords
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (const GLvoid *)offsetof(Vertex, texture_coords));
    glEnableVertexAttribArray(1);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

Canvas::~Canvas() {
    MAKE_DELETE(allocator, Shader, shader);
    
    if (vbo) {
        glDeleteBuffers(1, &vbo);
    }

    if (ebo) {
        glDeleteBuffers(1, &ebo);
    }

    if (vao) {
        glDeleteVertexArrays(1, &vao);
    }

    if (texture) {
        glDeleteTextures(1, &texture);
    }
}

void init_canvas(const Engine &engine, Canvas &canvas) {
    glBindTexture(GL_TEXTURE_2D, canvas.texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    int32_t size = (engine.window_rect.size.x / engine.render_scale) * (engine.window_rect.size.y / engine.render_scale) * 4;
    array::resize(canvas.data, size);

    for (int32_t i = 0; i < size; i += 4) {
        canvas.data[i] = 0;
        canvas.data[i+1] = 0;
        canvas.data[i+2] = 0;
        canvas.data[i+3] = 255;
    }

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, engine.window_rect.size.x, engine.window_rect.size.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, array::begin(canvas.data));
    glBindTexture(GL_TEXTURE_2D, 0);
}

void render_canvas(const Engine &engine, Canvas &canvas) {
    assert(canvas.texture);

    const GLuint shader_program = canvas.shader->program;

    glUseProgram(shader_program);
    glBindVertexArray(canvas.vao);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, canvas.texture);
    glUniform1i(glGetUniformLocation(shader_program, "texture0"), 0);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

    glDisable(GL_BLEND);
    glBindVertexArray(0);
}

void canvas::pset(Canvas &canvas, int x, int y) {
    canvas::pset(canvas, x, y, canvas.col);
}

void canvas::pset(Canvas &canvas, int x, int y, Color4f col) {
}

} // namespace engine
