#include "engine/sprites.h"
#include "engine/atlas.h"
#include "engine/engine.h"
#include "engine/log.h"
#include "engine/shader.h"
#include "engine/texture.h"

#include <GLFW/glfw3.h>
#include <array.h>
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <hash.h>
#include <memory.h>
#include <mutex>
#include <temp_allocator.h>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/euler_angles.hpp>

namespace {
const uint64_t max_sprites = 1000000;

const char *vertex_source = R"(
#version 410 core

uniform mat4 projection;
uniform mat4 model;

layout (location = 0) in vec3 in_position;
layout (location = 1) in vec4 in_color;
layout (location = 2) in vec2 in_texture_coords;

smooth out vec2 uv;
smooth out vec4 color;

void main() {
    mat4 mvp = projection * model;
    gl_Position = mvp * vec4(in_position, 1.0);
    uv = in_texture_coords;
    color = in_color;
}
)";

const char *fragment_source = R"(
#version 410 core

uniform sampler2D texture0;
in vec2 uv;
in vec4 color;

out vec4 out_color;

void main() {
    out_color = color * texture(texture0, uv);
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
, sprites(nullptr)
, sprite_id_counter(0)
, sprites_mutex(nullptr)
, time(0)
, animation_id_counter(0)
, animations(nullptr)
, done_animations(nullptr)
, transforms(nullptr) {
    shader = MAKE_NEW(allocator, Shader, nullptr, vertex_source, fragment_source, "Sprites");
    sprites = MAKE_NEW(allocator, Array<Sprite>, allocator);
    sprites_mutex = MAKE_NEW(allocator, std::mutex);
    animations = MAKE_NEW(allocator, Array<SpriteAnimation>, allocator);
    done_animations = MAKE_NEW(allocator, Array<SpriteAnimation>, allocator);
    transforms = MAKE_NEW(allocator, Hash<Matrix4f>, allocator);

    const size_t vertex_count = 4 * max_sprites;
    const size_t vertex_data_size = sizeof(Vertex) * vertex_count;

    size_t index_count = 6 * max_sprites;
    GLuint *index_data = (GLuint *)allocator.allocate(sizeof(GLuint) * (uint32_t)index_count);

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
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), (const GLvoid *)offsetof(Vertex, color));

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
                vertex_data[i * 4 + 0].color = {1.0f, 1.0f, 1.0f, 1.0f};
                vertex_data[i * 4 + 1].color = {1.0f, 1.0f, 1.0f, 1.0f};
                vertex_data[i * 4 + 2].color = {1.0f, 1.0f, 1.0f, 1.0f};
                vertex_data[i * 4 + 3].color = {1.0f, 1.0f, 1.0f, 1.0f};
            }

            // texture_coords
            {
                vertex_data[i * 4 + 0].texture_coords = {0.0, 0.0};
                vertex_data[i * 4 + 1].texture_coords = {0.0, 0.0};
                vertex_data[i * 4 + 2].texture_coords = {0.0, 0.0};
                vertex_data[i * 4 + 3].texture_coords = {0.0, 0.0};

                index_data[i * 6 + 0] = (GLuint)i * 4 + 0;
                index_data[i * 6 + 1] = (GLuint)i * 4 + 1;
                index_data[i * 6 + 2] = (GLuint)i * 4 + 2;

                index_data[i * 6 + 3] = (GLuint)i * 4 + 0;
                index_data[i * 6 + 4] = (GLuint)i * 4 + 3;
                index_data[i * 6 + 5] = (GLuint)i * 4 + 1;
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

    if (sprites_mutex) {
        MAKE_DELETE(allocator, mutex, sprites_mutex);
    }

    if (atlas) {
        MAKE_DELETE(allocator, Atlas, atlas);
    }

    if (animations) {
        MAKE_DELETE(allocator, Array, animations);
    }

    if (done_animations) {
        MAKE_DELETE(allocator, Array, done_animations);
    }

    if (transforms) {
        MAKE_DELETE(allocator, Hash, transforms);
    }
}

void init_sprites(Sprites &sprites, const char *atlas_filename) {
    sprites.atlas = MAKE_NEW(sprites.allocator, Atlas, sprites.allocator, atlas_filename);
}

const Sprite add_sprite(Sprites &sprites, const char *sprite_name, Color4f color) {
    std::scoped_lock lock(*sprites.sprites_mutex);

    if (array::size(*sprites.sprites) > max_sprites) {
        log_fatal("Sprites already at max size");
    }

    const Rect *rect = atlas_rect(*sprites.atlas, sprite_name);
    if (!rect) {
        log_fatal("Sprites atlas doesn't contain %s", sprite_name);
    }

    Sprite sprite;
    sprite.id = ++sprites.sprite_id_counter;
    sprite.atlas_rect = rect;
    sprite.transform = Matrix4f::identity();
	sprite.color = color;
    sprite.dirty = true;

    array::push_back(*sprites.sprites, sprite);

    return sprite;
}

void remove_sprite(Sprites &sprites, const uint64_t id) {
    std::scoped_lock lock(*sprites.sprites_mutex);

    for (engine::Sprite *iter = array::begin(*sprites.sprites); iter != array::end(*sprites.sprites); ++iter) {
        if (iter->id == id) {
            if (iter + 1 != array::end(*sprites.sprites)) {
                std::swap(*iter, array::back(*sprites.sprites));
            }
            array::pop_back(*sprites.sprites);
            break;
        }
    }
}

const Sprite *get_sprite(const Sprites &sprites, const uint64_t id) {
    std::scoped_lock lock(*sprites.sprites_mutex);

    for (Sprite *iter = array::begin(*sprites.sprites); iter != array::end(*sprites.sprites); ++iter) {
        if (iter->id == id) {
            return iter;
        }
    }

    return nullptr;
}

void transform_sprite(Sprites &sprites, const uint64_t id, const Matrix4f transform) {
    std::scoped_lock lock(*sprites.sprites_mutex);
    multi_hash::insert(*sprites.transforms, id, transform);
}

void color_sprite(Sprites &sprites, const uint64_t id, const Color4f color) {
    std::scoped_lock lock(*sprites.sprites_mutex);

    for (engine::Sprite *iter = array::begin(*sprites.sprites); iter != array::end(*sprites.sprites); ++iter) {
        if (iter->id == id) {
            iter->color = color;
            iter->dirty = true;
            break;
        }
    }
}

const Array<SpriteAnimation> &done_sprite_animations(Sprites &sprites) {
    return *sprites.done_animations;
}

uint64_t animate_sprite_position(Sprites &sprites, const uint64_t sprite_id, const Vector3 to_position, const float duration, const float delay) {
    const Sprite *sprite = get_sprite(sprites, sprite_id);
    if (!sprite) {
        return 0;
    }

    SpriteAnimation animation;
    animation.animation_id = ++sprites.animation_id_counter;
    animation.sprite_id = sprite_id;
    animation.type = SpriteAnimation::Type::Position;
    animation.start_time = sprites.time + delay;
    animation.duration = duration;
    animation.completed = false;
    animation.from_transform = sprite->transform;

    {
        glm::vec3 to_position_vec3 = glm::vec3(to_position.x, to_position.y, to_position.z);
        glm::mat4 from_transform_mat4 = glm::make_mat4(animation.from_transform.m);
        glm::vec3 from_position_vec3 = from_transform_mat4[3];
        glm::mat4 delta(1.0f);
        delta = glm::translate(delta, to_position_vec3 - from_position_vec3);

        const glm::mat4 to_transform_mat4 = delta * from_transform_mat4;
        const float *to_transform_mat4_value_ptr = glm::value_ptr(to_transform_mat4);
        memcpy(animation.to_transform.m, to_transform_mat4_value_ptr, sizeof(float) * 16);
    }

    array::push_back(*sprites.animations, animation);

    return animation.animation_id;
}

uint64_t animate_sprite_color(Sprites &sprites, const uint64_t sprite_id, const Color4f to_color, const float duration, const float delay) {
    const Sprite *sprite = get_sprite(sprites, sprite_id);
    if (!sprite) {
        return 0;
    }

    SpriteAnimation animation;
    animation.animation_id = ++sprites.animation_id_counter;
    animation.sprite_id = sprite_id;
    animation.type = SpriteAnimation::Type::Color;
    animation.start_time = sprites.time + delay;
    animation.duration = duration;
    animation.completed = false;
    animation.from_color = sprite->color;
    animation.to_color = to_color;

    array::push_back(*sprites.animations, animation);

    return animation.animation_id;
}

void update_sprites(Sprites &sprites, float t, float dt) {
    (void)dt;
    sprites.time = t;

    bool dirty = false;

    array::clear(*sprites.done_animations);

    for (SpriteAnimation *animation = array::begin(*sprites.animations); animation != array::end(*sprites.animations); ++animation) {
        if (t < animation->start_time) {
            continue;
        }

        bool completed = false;
        float a = (t - animation->start_time) / animation->duration;
        if (a > 1.0f) {
            a = 1.0f;
            completed = true;
        }

        switch (animation->type) {
        case SpriteAnimation::Type::Position: {
            const glm::vec3 from_pos = glm::make_mat4(animation->from_transform.m)[3];
            const glm::vec3 to_pos = glm::make_mat4(animation->to_transform.m)[3];
            const glm::vec3 mixed_pos = glm::mix(from_pos, to_pos, a);

            glm::mat4 delta(1.0f);
            delta = glm::translate(delta, -from_pos);
            delta = glm::translate(delta, mixed_pos);

            const glm::mat4 mixed_transform = delta * glm::make_mat4(animation->from_transform.m);
            const float *mixed_transform_value_ptr = glm::value_ptr(mixed_transform);
            const Matrix4f final_transform(mixed_transform_value_ptr);

            transform_sprite(sprites, animation->sprite_id, final_transform);
            break;
        }
        case SpriteAnimation::Type::Rotation: {
            log_fatal("Sprite rotation animation not implemented");
            // TODO: Implement
            break;
        }
        case SpriteAnimation::Type::Color: {
            const Color4f from_color = animation->from_color;
            const Color4f to_color = animation->to_color;
            const Color4f mixed_color = math::mix(from_color, to_color, a);
            color_sprite(sprites, animation->sprite_id, mixed_color);
        }
        }

        if (completed) {
            animation->completed = true;
            dirty = true;
        }
    }

    if (dirty) {
        Array<SpriteAnimation> animations(sprites.allocator);
        array::reserve(animations, array::size(*sprites.animations));

        for (SpriteAnimation *animation = array::begin(*sprites.animations); animation != array::end(*sprites.animations); ++animation) {
            if (animation->completed) {
                array::push_back(*sprites.done_animations, *animation);
            } else {
                array::push_back(animations, *animation);
            }
        }

        *sprites.animations = animations;
    }
}

void commit_sprites(Sprites &sprites) {
    std::scoped_lock lock(*sprites.sprites_mutex);

    TempAllocator1024 ta;
    Array<Matrix4f> transform_updates(ta);

    int i = 0;
    for (engine::Sprite *sprite = array::begin(*sprites.sprites); sprite != array::end(*sprites.sprites); ++sprite) {
        multi_hash::get(*sprites.transforms, sprite->id, transform_updates);

        glm::mat4 sprite_transform = glm::make_mat4(sprite->transform.m);

        if (!array::empty(transform_updates)) {
            // Apply cummulated transform matrices
            for (Matrix4f *transform_update = array::begin(transform_updates); transform_update != array::end(transform_updates); ++transform_update) {
                if (transform_update == array::begin(transform_updates)) {
                    sprite_transform = glm::make_mat4(transform_update->m);
                } else {
                    sprite_transform *= glm::make_mat4(transform_update->m);
                }
            }

            sprite->transform = Matrix4f(glm::value_ptr(sprite_transform));
            sprite->dirty = true;
            array::clear(transform_updates);
        }

        if (sprite->dirty) {
            // position
            {
                for (int ii = 0; ii < 4; ++ii) {
                    const glm::vec4 position = sprite_transform * unit_quad[ii];
                    sprites.vertex_data[i * 4 + ii].position = {position.x, position.y, position.z};
                }
            }

            // texture coords
            {
                const int atlas_width = sprites.atlas->texture->width;
                const int atlas_height = sprites.atlas->texture->height;

                float texcoord_x = (float)sprite->atlas_rect->origin.x / atlas_width;
                float texcoord_y = (float)(sprite->atlas_rect->origin.y + sprite->atlas_rect->size.y) / atlas_height;
                float texcoord_w = (float)sprite->atlas_rect->size.x / atlas_width;
                float texcoord_h = (float)sprite->atlas_rect->size.y / atlas_height;

                sprites.vertex_data[i * 4 + 0].texture_coords = {texcoord_x, texcoord_y};
                sprites.vertex_data[i * 4 + 1].texture_coords = {texcoord_x + texcoord_w, texcoord_y - texcoord_h};
                sprites.vertex_data[i * 4 + 2].texture_coords = {texcoord_x, texcoord_y - texcoord_h};
                sprites.vertex_data[i * 4 + 3].texture_coords = {texcoord_x + texcoord_w, texcoord_y};
            }

            // color
            {
                sprites.vertex_data[i * 4 + 0].color = sprite->color;
                sprites.vertex_data[i * 4 + 1].color = sprite->color;
                sprites.vertex_data[i * 4 + 2].color = sprite->color;
                sprites.vertex_data[i * 4 + 3].color = sprite->color;
            }

            sprite->dirty = false;
        }

        ++i;
    }

    hash::clear(*sprites.transforms);
}

void render_sprites(const Engine &engine, const Sprites &sprites) {
    std::scoped_lock lock(*sprites.sprites_mutex);

    if (!(sprites.shader && sprites.shader->program && sprites.vao && sprites.ebo && sprites.atlas)) {
        return;
    }

    glPushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, 0, -1, "render sprites");

    const float render_scale = engine.camera_zoom * engine.render_scale;

    const glm::mat4 projection = glm::ortho(0.0f, (float)engine.window_rect.size.x, 0.0f, (float)engine.window_rect.size.y, 0.1f, 100.0f);
    glm::mat4 view = glm::mat4(1);
    view = glm::translate(view, glm::vec3(-engine.camera_offset.x, -engine.camera_offset.y, 0.0f));

    const GLuint shader_program = sprites.shader->program;

    glUseProgram(shader_program);
    glBindVertexArray(sprites.vao);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, sprites.atlas->texture->texture);
    glUniform1i(glGetUniformLocation(shader_program, "texture0"), 0);

    glm::mat4 model = glm::mat4(1);
    model = glm::scale(model, glm::vec3(render_scale, render_scale, 1.0f));

    glUniformMatrix4fv(glGetUniformLocation(shader_program, "projection"), 1, GL_FALSE, glm::value_ptr(projection * view));
    glUniformMatrix4fv(glGetUniformLocation(shader_program, "model"), 1, GL_FALSE, glm::value_ptr(model));

    uint64_t quads = array::size(*sprites.sprites);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_DEPTH_TEST);

    glDrawElements(GL_TRIANGLES, 6 * (GLsizei)quads, GL_UNSIGNED_INT, (void *)0);

	glEnable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);

    glBindVertexArray(0);
    glPopDebugGroup();
}

} // namespace engine
