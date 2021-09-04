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

struct Engine;
struct Atlas;
struct Shader;

struct Sprite {
    const Rect *atlas_rect = nullptr;
    glm::vec3 scale = { 1.0f, 1.0f, 1.0f };
    glm::vec3 position = { 0.0f, 0.0f, 0.0f };
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
};

// Initializes this Sprites with an atlas. Required before rendering.
void init_sprites(Sprites &sprites, const char *atlas_filename);

// Adds a sprite and returns a pointer to the Sprite if successful.
Sprite *add_sprite(Sprites &sprites, const char *sprite_name);

// Updates all dirty sprites.
void update_sprites(Sprites &sprites);

// Renders the sprites.
void render_sprites(Engine &engine, Sprites &sprites);

} // namespace engine
