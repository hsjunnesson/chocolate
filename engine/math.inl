#pragma once

#include <algorithm>
#include <glm/glm.hpp>

namespace math {

struct Vertex {
    glm::vec3 position;
    glm::vec4 color;
    glm::vec2 texture_coords;
};

struct Rect {
    glm::ivec2 origin;
    glm::ivec2 size;
};

/**
 * @brief Mixes two colors.
 *
 * @param x Color x.
 * @param y Color y.
 * @param a The ratio of blending.
 * @return constexpr Color4f The mixed color.
 */
constexpr glm::vec4 mix(const glm::vec4 x, const glm::vec4 y, const float a) {
    const float ratio = 1.0f - a;
    glm::vec4 res{
        x.r * ratio + y.r * a,
        x.g * ratio + y.g * a,
        x.b * ratio + y.b * a,
        x.a * ratio + y.a * a};
    return res;
}

/**
 * @brief Returns the index of an x, y coordinate
 *
 * @param x The x coord
 * @param y The y coord
 * @param max_width The maximum width.
 * @return constexpr int32_t The index.
 */
constexpr int32_t index(int32_t const x, int32_t const y, int32_t const max_width) {
    assert((x + max_width * y) >= 0);
    return x + max_width * y;
}

/**
 * @brief Calculates the x, y coordinates based on an index.
 *
 * @param index The index.
 * @param x The pass-by-reference x coord to calculate.
 * @param y The pass-by-reference y coord to calculate.
 * @param max_width The maximum width.
 */
constexpr void coord(int32_t const index, int32_t &x, int32_t &y, int32_t const max_width) {
    x = index % max_width;
    y = index / max_width;
}

/**
 * @brief Returns a new index offset by x and y coordinates.
 * Careful with wrapping, since coordinates are unsigned.
 *
 * @param idx The index.
 * @param xoffset The x offset.
 * @param yoffset They y offset.
 * @param max_width The maximum width.
 * @return The index.
 */
constexpr int32_t index_offset(int32_t const idx, int32_t const xoffset, int32_t const yoffset, int32_t const max_width) {
    int32_t x = 0;
    int32_t y = 0;
    coord(idx, x, y, max_width);

    return index(x + xoffset, y + yoffset, max_width);
}

/**
 * @brief Linear interpolation.
 */
template <typename T>
T lerp(T a, T b, float ratio) {
    return a + ratio * (b - a);
}

/**
 * @brief Returns a new value that approaches a target by an amount
 */
template <typename T>
T approach(T value, T target, T amount) {
    T a = amount < 0 ? -amount : amount; // abs
    return value > target ? std::max(value - a, target) : std::min(value + a, target);
}

/**
 * @brief Whether `point` is inside `rect`.
 */
constexpr bool is_inside(const Rect &rect, const glm::ivec2 point) {
    return point.x >= rect.origin.x &&
           point.x < (rect.origin.x + rect.size.x) &&
           point.y >= rect.origin.y &&
           point.y < (rect.origin.y + rect.size.y);
}

/**
 * @brief Whether `point` is inside `rect`.
 */
constexpr bool is_inside(const Rect &rect, const glm::vec2 point) {
    return point.x >= rect.origin.x &&
           point.x < (rect.origin.x + rect.size.x) &&
           point.y >= rect.origin.y &&
           point.y < (rect.origin.y + rect.size.y);
}

/**
 * @brief Whether `rect1` overlaps `rect2`.
 */
constexpr bool is_inside(const Rect &rect1, const Rect &rect2) {
    return rect1.origin.x < rect2.origin.x + rect2.size.x &&
           rect1.origin.x + rect1.size.x > rect2.origin.x &&
           rect1.origin.y < rect2.origin.y + rect2.size.y &&
           rect1.origin.y + rect1.size.y > rect2.origin.y;
}

} // namespace math
