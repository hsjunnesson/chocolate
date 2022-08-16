#pragma once

#pragma warning(push, 0)
#include <algorithm>
#include <assert.h>
#include <inttypes.h>
#pragma warning(pop)

namespace math {

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

struct Color4f {
    float r = 0;
    float g = 0;
    float b = 0;
    float a = 0;

    // constexpr Color4f(float r = 0.0f, float g = 0.0f, float b = 0.0f, float a = 0.0f)
    // : r(r)
    // , g(g)
    // , b(b)
    // , a(a) {}
};

struct Matrix4f {
    float m[16];

    Matrix4f()
    : m{
          0.0f, 0.0f, 0.0f, 0.0f,
          0.0f, 0.0f, 0.0f, 0.0f,
          0.0f, 0.0f, 0.0f, 0.0f,
          0.0f, 0.0f, 0.0f, 0.0f} {}

    explicit Matrix4f(const float *float_data) {
        memcpy(m, float_data, sizeof(float) * 16);
    }

    static Matrix4f identity() {
        Matrix4f mat4;
        float m[16] = {
            1.0f, 0.0f, 0.0f, 0.0f,
            0.0f, 1.0f, 0.0f, 0.0f,
            0.0f, 0.0f, 1.0f, 0.0f,
            0.0f, 0.0f, 0.0f, 1.0f};
        memcpy(mat4.m, m, sizeof(m));
        return mat4;
    }
};

struct Vertex {
    Vector3f position;
    Color4f color;
    Vector2f texture_coords;
};

struct Rect {
    Vector2 origin;
    Vector2 size;
};

/**
 * @brief Mixes two colors.
 *
 * @param x Color x.
 * @param y Color y.
 * @param a The ratio of blending.
 * @return constexpr Color4f The mixed color.
 */
constexpr Color4f mix(const Color4f x, const Color4f y, const float a) {
    const float ratio = 1.0f - a;
    Color4f res;
    res.r = x.r * ratio + y.r * a;
    res.g = x.g * ratio + y.g * a;
    res.b = x.b * ratio + y.b * a;
    res.a = x.a * ratio + y.a * a;
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

} // namespace math
