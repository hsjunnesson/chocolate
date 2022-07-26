#pragma once

#include "math.inl"

#pragma warning(push, 0)
#include <memory_types.h>
#pragma warning(pop)

namespace engine {

struct Engine;

using foundation::Allocator;
using math::Color4f;

struct Canvas {
    Canvas(Allocator &allocator);

    Color4f col;
};

void render_canvas(const Engine &engine, Canvas &canvas);

namespace canvas {
void pset(Canvas &canvas, int x, int y);
void pset(Canvas &canvas, int x, int y, Color4f col);
} // namespace canvas
} // namespace engine
