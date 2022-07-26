#include "engine/canvas.h"
#include "engine/engine.h"
#include "engine/color.inl"

using engine::Engine;
using engine::Canvas;
using foundation::Allocator;
using math::Color4f;

namespace engine {

Canvas::Canvas(Allocator &allocator)
: col(engine::color::black)
{
}

void render_canvas(const Engine &engine, Canvas &canvas) {
}

void canvas::pset(Canvas &canvas, int x, int y) {
    canvas::pset(canvas, x, y, canvas.col);
}

void canvas::pset(Canvas &canvas, int x, int y, Color4f col) {
}

} // namespace engine
