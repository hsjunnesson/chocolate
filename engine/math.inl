#pragma once

#include <inttypes.h>
#include <algorithm>

namespace engine {

struct Vector2 {
    int32_t x = 0;
    int32_t y = 0;
};

struct Vector2f {
    float x = 0.0f;
    float y = 0.0f;
};

struct Vector3 {
    int32_t x = 0;
    int32_t y = 0;
    int32_t z = 0;
};

struct Vector3f {
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
};

struct Vertex {
    Vector3f position;
    Vector3f color;
    Vector2f texture_coords;
};

struct Rect {
    Vector2 origin;
    Vector2 size;
};

/**
 * @brief Returns the index of an x, y coordinate
 * 
 * @param x The x coord
 * @param y The y coord
 * @param max_width The maximum width.
 * @return constexpr uint32_t The index.
 */
constexpr uint32_t index(uint32_t const x, uint32_t const y, uint32_t const max_width) {
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
constexpr void coord(uint32_t const index, uint32_t &x, uint32_t &y, uint32_t const max_width) {
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
constexpr int32_t index_offset(int32_t const idx, int32_t const xoffset, int32_t const yoffset, uint32_t const max_width) {
    uint32_t x = 0;
    uint32_t y = 0;
    coord(idx, x, y, max_width);

    return index(x + xoffset, y + yoffset, max_width);
}

/**
 * @brief Returns a new value that approaches a target by an amount
 */
template<typename T> constexpr T approach(T value, T target, T amount) {
    T a = amount < 0 ? -amount : amount; // abs
    return value > target ? std::max(value - a, target) : std::min(value + a, target);
}

} // namespace engine
