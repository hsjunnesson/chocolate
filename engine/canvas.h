#pragma once

#include "math.inl"

#pragma warning(push, 0)
#include <collection_types.h>
#include <memory_types.h>
#pragma warning(pop)

namespace engine {

struct Engine;
struct Shader;

using foundation::Allocator;
using foundation::Array;
using math::Color4f;

struct Canvas {
    Canvas(Allocator &allocator);
    ~Canvas();

    Allocator &allocator;
    Color4f col;
    Shader *shader;
    uint32_t texture;
    uint32_t vbo;
    uint32_t vao;
    Array<uint8_t> data;
};

void init_canvas(const Engine &engine, Canvas &canvas);
void render_canvas(const Engine &engine, Canvas &canvas);

namespace canvas {
void pset(Canvas &canvas, int x, int y);
void pset(Canvas &canvas, int x, int y, Color4f col);
} // namespace canvas
} // namespace engine
