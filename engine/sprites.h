#pragma mark once

#include "math.inl"

#pragma warning(push, 0)
#include <collection_types.h>
#include <inttypes.h>
#pragma warning(pop)

namespace std {
class mutex;
}

namespace engine {
using namespace foundation;
using namespace math;

struct Engine;
struct Atlas;
struct Shader;

struct Sprite {
    uint64_t id;
    const Rect *atlas_rect = nullptr;
    Matrix4f transform = Matrix4f::identity();
    Color4f color = {1.0f, 1.0f, 1.0f, 1.0f};
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
    Matrix4f from_transform;
    Matrix4f to_transform;
    Color4f from_color;
    Color4f to_color;
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

    Array<Sprite> *sprites;
    uint64_t sprite_id_counter;
    std::mutex *sprites_mutex;

    float time;
    uint64_t animation_id_counter;
    Array<SpriteAnimation> *animations;
    Array<SpriteAnimation> *done_animations; // The list of done animations since last frame
};

// Initializes this Sprites with an atlas. Required before rendering.
void init_sprites(Sprites &sprites, const char *atlas_filename);

// Adds a sprite and returns a copy of the sprite.
const Sprite add_sprite(Sprites &sprites, const char *sprite_name);

// Remove sprite based on its id.
void remove_sprite(Sprites &sprites, const uint64_t id);

// Returns a pointer to a sprite by its id.
const Sprite *get_sprite(const Sprites &sprites, const uint64_t id);

// Transforms a sprite.
void transform_sprite(Sprites &sprites, const uint64_t id, const Matrix4f transform);

// Updates color of sprite.
void color_sprite(Sprites &sprites, const uint64_t id, const Color4f color);

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
uint64_t animate_sprite_position(Sprites &sprites, const uint64_t sprite_id, const Vector3 to_position, const float duration, const float delay = 0.0f);

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
uint64_t animate_sprite_color(Sprites &sprites, const uint64_t sprite_id, const Color4f to_color, const float duration, const float delay = 0.0f);

// Updates animations.
void update_sprites(Sprites &sprites, float t, float dt);

// Commits all dirty sprites.
void commit_sprites(Sprites &sprites);

// Renders the sprites.
void render_sprites(const Engine &engine, const Sprites &sprites);

} // namespace engine
