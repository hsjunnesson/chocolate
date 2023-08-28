#pragma once

#include "math.inl"

#include <math.h>

namespace engine {
namespace color {
using math::Color4f;

constexpr Color4f black = {0.0f, 0.0f, 0.0f, 1.0f};
constexpr Color4f white = {1.0f, 1.0f, 1.0f, 1.0f};
constexpr Color4f red = {1.0f, 0.0f, 0.0f, 1.0f};
constexpr Color4f green = {0.0f, 1.0f, 0.0f, 1.0f};
constexpr Color4f blue = {0.0f, 0.0f, 1.0f, 1.0f};

// Colors based on the PICO-8 palette
namespace pico8 {

constexpr Color4f black = {0.0f, 0.0f, 0.0f, 1.0f};
constexpr Color4f dark_blue = {29.0f / 255, 43.0f / 255, 83.0f / 255, 1.0f};
constexpr Color4f dark_purple = {126.0f / 255, 43.0f / 255, 83.0f / 255, 1.0f};
constexpr Color4f dark_green = {0.0f / 255, 135.0f / 255, 81.0f / 255, 1.0f};
constexpr Color4f brown = {171.0f / 255, 82.0f / 255, 54.0f / 255, 1.0f};
constexpr Color4f dark_gray = {95.0f / 255, 87.0f / 255, 79.0f / 255, 1.0f};
constexpr Color4f light_gray = {194.0f / 255, 195.0f / 255, 199.0f / 255, 1.0f};
constexpr Color4f white = {255.0f / 255, 241.0f / 255, 232.0f / 255, 1.0f};
constexpr Color4f red = {255.0f / 255, 0.0f / 255, 77.0f / 255, 1.0f};
constexpr Color4f orange = {255.0f / 255, 163.0f / 255, 0.0f / 255, 1.0f};
constexpr Color4f yellow = {255.0f / 255, 236.0f / 255, 39.0f / 255, 1.0f};
constexpr Color4f green = {0.0f / 255, 228.0f / 255, 54.0f / 255, 1.0f};
constexpr Color4f blue = {41.0f / 255, 173.0f / 255, 255.0f / 255, 1.0f};
constexpr Color4f indigo = {131.0f / 255, 118.0f / 255, 156.0f / 255, 1.0f};
constexpr Color4f pink = {255.0f / 255, 119.0f / 255, 168.0f / 255, 1.0f};
constexpr Color4f peach = {255.0f / 255, 204.0f / 255, 170.0f / 255, 1.0f};

} // namespace pico8

inline float luminance(const math::Color4f color) {
    return 0.2126f * powf(color.r, 2.2f) + 0.7152f * powf(color.g, 2.2f) + 0.0722f * powf(color.b, 2.2f);
}

} // namespace color
} // namespace engine
