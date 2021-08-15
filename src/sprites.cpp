#include "engine/sprites.h"
#include "engine/atlas.h"
#include "engine/log.h"
#include "engine/texture.h"
#include "engine/shader.h"
#include "engine/engine.h"

#include <memory.h>
#include <array.h>

#include <GLFW/glfw3.h>
#include <glad/glad.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/euler_angles.hpp>

namespace {
const uint64_t max_sprites = 1024;

const char *vertex_source = R"(
#version 440 core

uniform mat4 projection;
uniform mat4 model;

layout (location = 0) in vec3 in_position;
layout (location = 1) in vec3 in_color;
layout (location = 2) in vec2 in_texture_coords;

smooth out vec2 uv;
smooth out vec4 color;

void main() {
    mat4 mvp = projection * model;
    gl_Position = mvp * vec4(in_position, 1.0);
    uv = in_texture_coords;
    color = vec4(in_color, 1.0);
}
)";

const char *fragment_source = R"(
#version 440 core

uniform sampler2D texture0;
in vec2 uv;
in vec4 color;

out vec4 out_color;

void main() {
    out_color = color;
//    out_color = vec4(uv, 0.0, 0.0);
//    out_color = color * texture(texture0, uv);
}
)";

}

namespace engine {

Sprites::Sprites(Allocator &allocator, Atlas &atlas)
: allocator(allocator)
, atlas(&atlas)
, shader(nullptr)
, vbo(0)
, vao(0)
, ebo(0)
, sprites(nullptr)
{
    shader = MAKE_NEW(allocator, Shader, nullptr, vertex_source, fragment_source);
    sprites = MAKE_NEW(allocator, Array<Sprite>, allocator);

    // Atlas rect
    {
        // const Rect *rect = engine::atlas_rect(atlas, sprite_name);
        // if (!rect) {
        //     log_fatal("Altas doesn't contain %s", sprite_name);
        // }

        // this->atlas_rect = rect;
    }

    size_t vertex_count = 4 * max_sprites;
    Vertex *vertex_data = (Vertex *)allocator.allocate(sizeof(Vertex) * vertex_count);

    size_t index_count = 6 * max_sprites;
    GLuint *index_data = (GLuint *)allocator.allocate(sizeof(GLuint) * index_count);

    // OpenGL setup unit quad
    for (uint64_t i = 0; i < max_sprites; ++i) {
        // position
        {
            float x = 0;
            float y = 0;

            vertex_data[i * 4 + 0].position = {x,     y,     0.0f};
            vertex_data[i * 4 + 1].position = {x + 10, y + 10, 0.0f};
            vertex_data[i * 4 + 2].position = {x,     y + 10, 0.0f};
            vertex_data[i * 4 + 3].position = {x + 10, y,     0.0f};
        }

        // color
        {
            vertex_data[i * 4 + 0].color = {1.0f, 1.0f, 1.0f};
            vertex_data[i * 4 + 1].color = {1.0f, 1.0f, 1.0f};
            vertex_data[i * 4 + 2].color = {1.0f, 1.0f, 1.0f};
            vertex_data[i * 4 + 3].color = {1.0f, 1.0f, 1.0f};
        }

        // texture_coords
        {
            // const int atlas_width = this->atlas->texture->width;
            // const int atlas_height = this->atlas->texture->height;

            // float texcoord_x = (float)atlas_rect->origin.x / atlas_width;
            // float texcoord_y = (float)atlas_rect->origin.y / atlas_height;
            // float texcoord_w = (float)atlas_rect->size.x / atlas_width;
            // float texcoord_h = (float)atlas_rect->size.y / atlas_height;

            // vertex_data[0].texture_coords = {texcoord_x,                texcoord_y             };
            // vertex_data[1].texture_coords = {texcoord_x + texcoord_w,   texcoord_y + texcoord_h};
            // vertex_data[2].texture_coords = {texcoord_x,                texcoord_y + texcoord_h};
            // vertex_data[3].texture_coords = {texcoord_x + texcoord_w,   texcoord_y             };

            vertex_data[i * 4 + 0].texture_coords = {0.0, 0.0};
            vertex_data[i * 4 + 1].texture_coords = {0.0, 0.0};
            vertex_data[i * 4 + 2].texture_coords = {0.0, 0.0};
            vertex_data[i * 4 + 3].texture_coords = {0.0, 0.0};

            index_data[i * 6 + 0] = 0;
            index_data[i * 6 + 1] = 1;
            index_data[i * 6 + 2] = 2;

            index_data[i * 6 + 3] = 0;
            index_data[i * 6 + 4] = 3;
            index_data[i * 6 + 5] = 1;
        }
    }

    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    glGenBuffers(1, &vbo);
    glGenBuffers(1, &ebo);

    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, vertex_count * sizeof(Vertex), vertex_data, GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    glEnableVertexAttribArray(2);

    // position
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (const GLvoid *)0);

    // color
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (const GLvoid *)offsetof(Vertex, color));

    // texture_coords
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (const GLvoid *)offsetof(Vertex, texture_coords));

    // Element index array
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, index_count * sizeof(GLuint), index_data, GL_STATIC_DRAW);

    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    glDisableVertexAttribArray(0);
    glDisableVertexAttribArray(1);
    glDisableVertexAttribArray(2);

    allocator.deallocate(vertex_data);
    allocator.deallocate(index_data);
}

Sprites::~Sprites() {
    if (shader) {
        MAKE_DELETE(allocator, Shader, shader);
    }

    if (vbo) {
        glDeleteBuffers(1, &vbo);
    }

    if (vao) {
        glDeleteVertexArrays(1, &vao);
    }

    if (ebo) {
        glDeleteBuffers(1, &ebo);
    }

    if (sprites) {
        MAKE_DELETE(allocator, Array, sprites);
    }
}

Sprite *add_sprite(Sprites &sprites, const char *sprite_name) {
    const Rect *rect = atlas_rect(*sprites.atlas, sprite_name);
    if (!rect) {
        log_error("Sprites atlas doesn't contain %s", sprite_name);
        return nullptr;
    }

    Sprite sprite;
    sprite.atlas_rect = rect;
    sprite.transform = glm::mat4(1.0);

    array::push_back(*sprites.sprites, sprite);

    return array::end(*sprites.sprites);
}

void render_sprites(Engine &engine, Sprites &sprites) {
    if (!(sprites.shader && sprites.shader->program && sprites.vao && sprites.ebo)) {
        return;
    }

    const float render_scale = engine.camera_zoom;

    const glm::mat4 projection = glm::ortho(0.0f, (float)engine.window_rect.size.x, 0.0f, (float)engine.window_rect.size.y);
    glm::mat4 view = glm::mat4(1);
    view = glm::translate(view, glm::vec3(engine.camera_offset.x, engine.camera_offset.y, 0.0f));

    const GLuint shader_program = sprites.shader->program;

    glUseProgram(shader_program);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, sprites.atlas->texture->texture);
    glUniform1i(glGetUniformLocation(shader_program, "texture0"), 0);

    glm::mat4 model = glm::mat4(1);
//    model = glm::scale(model, glm::vec3(tilesheet.tile_size * render_scale, tilesheet.tile_size * render_scale, 1));

    glUniformMatrix4fv(glGetUniformLocation(shader_program, "projection"), 1, GL_FALSE, glm::value_ptr(projection * view));
    glUniformMatrix4fv(glGetUniformLocation(shader_program, "model"), 1, GL_FALSE, glm::value_ptr(model));

    glBindVertexArray(sprites.vao);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, sprites.ebo);

    uint64_t quads = array::size(*sprites.sprites);

    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

    glDrawElements(GL_TRIANGLES, 6 * quads, GL_UNSIGNED_INT, (void *)0);

    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

} // namespace engine
