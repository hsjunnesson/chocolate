#pragma once

#include "color.inl"
#include "math.inl"
#include <collection_types.h>
#include <inttypes.h>
#ifdef __APPLE__
#include <mutex>
#else

namespace std {
class mutex;
} // namespace std
#endif

namespace engine {
using namespace foundation;
using namespace math;

struct Engine;
struct Atlas;
struct AtlasFrame;
struct Shader;

struct Sprite {
    uint64_t id;
    const AtlasFrame *atlas_frame = nullptr;
    glm::mat4 transform = glm::mat4(1.0f);
    glm::vec4 color = {1.0f, 1.0f, 1.0f, 1.0f};
    bool dirty = false;
};

struct SpriteAnimation {
    enum class Type {
        Position,
        Rotation,
        Color,
    };

    uint64_t animation_id;
    uint64_t sprite_id;
    Type type;
    float start_time;
    float duration;
    bool completed;
    glm::mat4 from_transform;
    glm::mat4 to_transform;
    glm::vec4 from_color;
    glm::vec4 to_color;
};

// A collection of sprites that share an atlas.
struct Sprites {
    Sprites(Allocator &allocator);
    ~Sprites();
    
    Allocator &allocator;
    Atlas *atlas;
    Shader *shader;
    Vertex *vertex_data;
    uint32_t vbo;
    uint32_t vao;
    uint32_t ebo;

    float time;

    uint64_t sprite_id_counter;
    uint64_t animation_id_counter;

    std::mutex *sprites_mutex;
    
    Array<Sprite> sprites;
    Array<SpriteAnimation> animations;
    Array<SpriteAnimation> done_animations; // The list of done animations since last frame
    Hash<glm::mat4> transforms;             // A multihash map of sprite ids to a list of transforms waiting to be applied and cleared on commit_sprites.
};

// Initializes this Sprites with an atlas. Required before rendering.
void init_sprites(Sprites &sprites, const char *atlas_filename);

// Adds a sprite and returns a copy of the sprite.
const Sprite add_sprite(Sprites &sprites, const char *sprite_name, glm::vec4 color = engine::color::white);

// Remove sprite based on its id.
void remove_sprite(Sprites &sprites, const uint64_t id);

// Returns a pointer to a sprite by its id.
const Sprite *get_sprite(const Sprites &sprites, const uint64_t id);

// Transforms a sprite. Will take effect on next commit;
void transform_sprite(Sprites &sprites, const uint64_t id, const glm::mat4 transform);

// Updates color of sprite.
void color_sprite(Sprites &sprites, const uint64_t id, const glm::vec4 color);

// Returns the array of done animation since last frame.
const Array<SpriteAnimation> &done_sprite_animations(Sprites &sprites);

/**
 * @brief Creates a sprite animation for position.
 *
 * @param sprites The Sprites.
 * @param sprite_id The sprite id
 * @param to_position The position to animate to.
 * @param duration The duration in seconds.
 * @param delay The delay before starting animation in seconds.
 * @return uint64_t The id of the SpriteAnimation. 0 on errors.
 */
uint64_t animate_sprite_position(Sprites &sprites, const uint64_t sprite_id, const glm::vec3 to_position, const float duration, const float delay = 0.0f);

/**
 * @brief Creates a sprite animation for color.
 *
 * @param sprites The sprites.
 * @param sprite_id The sprite id.
 * @param to_color The color to animate to.
 * @param duration The duration in seconds.
 * @param delay The delay before starting animation in seconds.
 * @return uint64_t The id of the SpriteAnimation. 0 on errors.
 */
uint64_t animate_sprite_color(Sprites &sprites, const uint64_t sprite_id, const glm::vec4 to_color, const float duration, const float delay = 0.0f);

// Updates animations.
void update_sprites(Sprites &sprites, float t, float dt);

// Commits all dirty sprites.
void commit_sprites(Sprites &sprites);

// Renders the sprites.
void render_sprites(const Engine &engine, const Sprites &sprites);

} // namespace engine
