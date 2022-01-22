#pragma once

#include "math.inl"
#include <inttypes.h>

namespace foundation {
class Allocator;
template<typename T> struct Array;
}

namespace engine {
using namespace foundation;
using namespace math;

struct Engine;
struct Shader;
struct Texture;
class TilesheetParams;

struct Tile {
    enum class Flip {
        NONE,
        VERTICAL,
        HORIZONTAL
    };

    // Index in the atlas.
    uint16_t tile;
    Vector3f color = {1.0f, 1.0f, 1.0f};
    Flip flip = Flip::NONE;
};

struct Tilesheet {
    Tilesheet(Allocator &allocator, const char *params_path);
    ~Tilesheet();

    Allocator &allocator;
    TilesheetParams *params;

    Shader *tilesheet_shader;
    uint32_t tilesheet_vbo;
    uint32_t tilesheet_vao;
    uint32_t tilesheet_ebo;
    
    Texture *tilesheet_atlas;
    uint32_t tile_size; // Size in pixels

    uint32_t tiles_width;
    uint32_t tiles_height;
    Array<Tile> *tiles;
};

// Inits a proper size tilesheet.
void init_tilesheet(Tilesheet &tilesheet, uint32_t tiles_width, uint32_t tiles_height);

// Creates and initializes the tilesheet buffer objects and data.
void commit_tilesheet(Tilesheet &tilesheet);

// Renders the tilesheet
void render_tilesheet(Engine &engine, Tilesheet &tilesheet);

} // namespace engine
