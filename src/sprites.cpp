#include "engine/sprites.h"
#include "engine/atlas.h"
#include "engine/engine.h"
#include "engine/log.h"
#include "engine/shader.h"
#include "engine/texture.h"

#include <array.h>
#include <memory.h>

#include <GLFW/glfw3.h>
#include <glad/glad.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/euler_angles.hpp>

namespace {
const uint64_t max_sprites = 65536;

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
    out_color = texture(texture0, uv);
}
)";

const glm::vec4 unit_quad[] = {
    {0.0f, 0.0f, 0.0f, 1.0f},
    {1.0f, 1.0f, 0.0f, 1.0f},
    {0.0f, 1.0f, 0.0f, 1.0f},
    {1.0f, 0.0f, 0.0f, 1.0f}};

} // namespace

namespace engine {

Sprites::Sprites(Allocator &allocator)
: allocator(allocator)
, atlas(nullptr)
, shader(nullptr)
, vertex_data(nullptr)
, vbo(0)
, vao(0)
, ebo(0)
, sprites(nullptr) {
    shader = MAKE_NEW(allocator, Shader, nullptr, vertex_source, fragment_source);
    sprites = MAKE_NEW(allocator, Array<Sprite>, allocator);

    const size_t vertex_count = 4 * max_sprites;
    const size_t vertex_data_size = sizeof(Vertex) * vertex_count;

    size_t index_count = 6 * max_sprites;
    GLuint *index_data = (GLuint *)allocator.allocate(sizeof(GLuint) * index_count);

    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    glGenBuffers(1, &vbo);
    glGenBuffers(1, &ebo);

    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    //    glBufferData(GL_ARRAY_BUFFER, vertex_data_size, vertex_data, GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    glEnableVertexAttribArray(2);

    // position
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (const GLvoid *)0);

    // color
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (const GLvoid *)offsetof(Vertex, color));

    // texture_coords
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (const GLvoid *)offsetof(Vertex, texture_coords));

    // Immutable data storage
    {
        GLbitfield flags = GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT;
        glBufferStorage(GL_ARRAY_BUFFER, vertex_data_size, 0, flags);
        vertex_data = (Vertex *)glMapBufferRange(GL_ARRAY_BUFFER, 0, vertex_data_size, flags);

        for (uint64_t i = 0; i < max_sprites; ++i) {
            // position
            {
                // Copy unit quad into vertex data for this sprite
                for (int ii = 0; ii < 4; ++ii) {
                    vertex_data[i * 4 + ii].position = {unit_quad[ii].x, unit_quad[ii].y, unit_quad[ii].z};
                }
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
                vertex_data[i * 4 + 0].texture_coords = {0.0, 0.0};
                vertex_data[i * 4 + 1].texture_coords = {0.0, 0.0};
                vertex_data[i * 4 + 2].texture_coords = {0.0, 0.0};
                vertex_data[i * 4 + 3].texture_coords = {0.0, 0.0};

                index_data[i * 6 + 0] = i * 4 + 0;
                index_data[i * 6 + 1] = i * 4 + 1;
                index_data[i * 6 + 2] = i * 4 + 2;

                index_data[i * 6 + 3] = i * 4 + 0;
                index_data[i * 6 + 4] = i * 4 + 3;
                index_data[i * 6 + 5] = i * 4 + 1;
            }
        }
    }

    // Element index array
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, index_count * sizeof(GLuint), index_data, GL_STATIC_DRAW);

    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    glDisableVertexAttribArray(0);
    glDisableVertexAttribArray(1);
    glDisableVertexAttribArray(2);

    allocator.deallocate(index_data);
}

Sprites::~Sprites() {
    if (shader) {
        MAKE_DELETE(allocator, Shader, shader);
    }

    if (vbo) {
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glUnmapBuffer(GL_ARRAY_BUFFER);
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

    if (atlas) {
        MAKE_DELETE(allocator, Atlas, atlas);
    }
}

void init_sprites(Sprites &sprites, const char *atlas_filename) {
    sprites.atlas = MAKE_NEW(sprites.allocator, Atlas, sprites.allocator, atlas_filename);
}

Sprite *add_sprite(Sprites &sprites, const char *sprite_name) {
    if (array::size(*sprites.sprites) > max_sprites) {
        log_fatal("Sprites already at max size");
    }

    const Rect *rect = atlas_rect(*sprites.atlas, sprite_name);
    if (!rect) {
        log_error("Sprites atlas doesn't contain %s", sprite_name);
        return nullptr;
    }

    Sprite sprite;
    sprite.atlas_rect = rect;
    sprite.dirty = true;

    array::push_back(*sprites.sprites, sprite);
    engine::Sprite &back = array::back(*sprites.sprites);
    return &back;
}

void update_sprites(Sprites &sprites) {
    for (uint32_t i = 0; i < array::size(*sprites.sprites); ++i) {
        Sprite &sprite = (*sprites.sprites)[i];

        if (sprite.dirty) {
            glm::mat4 transform = glm::mat4(1.0f);
            transform = glm::translate(transform, sprite.position);
            transform = glm::scale(transform, glm::vec3(sprite.atlas_rect->size.x, sprite.atlas_rect->size.y, 1.0f) * sprite.scale);

            // position
            for (int ii = 0; ii < 4; ++ii) {
                glm::vec4 position = transform * unit_quad[ii];
                sprites.vertex_data[i * 4 + ii].position = {position.x, position.y, position.z};
            }

            // texture coords
            const int atlas_width = sprites.atlas->texture->width;
            const int atlas_height = sprites.atlas->texture->height;

            float texcoord_x = (float)sprite.atlas_rect->origin.x / atlas_width;
            float texcoord_y = (float)(sprite.atlas_rect->origin.y + sprite.atlas_rect->size.y) / atlas_height;
            float texcoord_w = (float)sprite.atlas_rect->size.x / atlas_width;
            float texcoord_h = (float)sprite.atlas_rect->size.y / atlas_height;

            sprites.vertex_data[i * 4 + 0].texture_coords = {texcoord_x, texcoord_y};
            sprites.vertex_data[i * 4 + 1].texture_coords = {texcoord_x + texcoord_w, texcoord_y - texcoord_h};
            sprites.vertex_data[i * 4 + 2].texture_coords = {texcoord_x, texcoord_y - texcoord_h};
            sprites.vertex_data[i * 4 + 3].texture_coords = {texcoord_x + texcoord_w, texcoord_y};

            sprite.dirty = false;
        }
    }
}

void render_sprites(Engine &engine, Sprites &sprites) {
    if (!(sprites.shader && sprites.shader->program && sprites.vao && sprites.ebo && sprites.atlas)) {
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

    glDrawElements(GL_TRIANGLES, 6 * quads, GL_UNSIGNED_INT, (void *)0);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

} // namespace engine
