#pragma mark once

#include "math.inl"
#include <inttypes.h>
#include <glm/glm.hpp>

namespace foundation {
class Allocator;
template<typename T> struct Array;
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
    glm::mat4 transform = glm::mat4(1.0f);
    Color4f color = {1.0f, 1.0f, 1.0f, 1.0f};
    bool dirty = false;
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
};

// Initializes this Sprites with an atlas. Required before rendering.
void init_sprites(Sprites &sprites, const char *atlas_filename);

// Adds a sprite and returns a copy of the sprite.
const Sprite add_sprite(Sprites &sprites, const char *sprite_name);

// Remove sprite based on its id.
void remove_sprite(Sprites &sprites, const uint64_t id);

// Transforms a sprite.
void transform_sprite(Sprites &sprites, const uint64_t id, const glm::mat4 transform);

// Updates color of sprite.
void color_sprite(Sprites &sprites, const uint64_t id, const Color4f color);

// Updates all dirty sprites.
void update_sprites(Sprites &sprites);

// Renders the sprites.
void render_sprites(const Engine &engine, const Sprites &sprites);

} // namespace engine
