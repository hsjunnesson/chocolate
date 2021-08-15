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
    const Rect *atlas_rect;
    glm::mat4 transform;
};

// A collection of sprites that share an atlas.
struct Sprites {
    Sprites(Allocator &allocator, Atlas &atlas);
    ~Sprites();

    Allocator &allocator;
    const Atlas *atlas;
    Shader *shader;
    uint32_t vbo;
    uint32_t vao;
    uint32_t ebo;

    Array<Sprite> *sprites;
};

// Adds a sprite and returns a pointer to the Sprite if successful.
Sprite *add_sprite(Sprites &sprites, const char *sprite_name);

// Renders the sprites.
void render_sprites(Engine &engine, Sprites &sprites);

} // namespace engine
