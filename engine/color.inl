#pragma once

#include <glm/glm.hpp>

namespace engine {
namespace color {

constexpr glm::vec4 black = {0.0f, 0.0f, 0.0f, 1.0f};
constexpr glm::vec4 white = {1.0f, 1.0f, 1.0f, 1.0f};
constexpr glm::vec4 red = {1.0f, 0.0f, 0.0f, 1.0f};
constexpr glm::vec4 green = {0.0f, 1.0f, 0.0f, 1.0f};
constexpr glm::vec4 blue = {0.0f, 0.0f, 1.0f, 1.0f};

// Colors based on the PICO-8 palette
namespace pico8 {

constexpr glm::vec4 black = {0.0f, 0.0f, 0.0f, 1.0f};
constexpr glm::vec4 dark_blue = {29.0f / 255, 43.0f / 255, 83.0f / 255, 1.0f};
constexpr glm::vec4 dark_purple = {126.0f / 255, 43.0f / 255, 83.0f / 255, 1.0f};
constexpr glm::vec4 dark_green = {0.0f / 255, 135.0f / 255, 81.0f / 255, 1.0f};
constexpr glm::vec4 brown = {171.0f / 255, 82.0f / 255, 54.0f / 255, 1.0f};
constexpr glm::vec4 dark_gray = {95.0f / 255, 87.0f / 255, 79.0f / 255, 1.0f};
constexpr glm::vec4 light_gray = {194.0f / 255, 195.0f / 255, 199.0f / 255, 1.0f};
constexpr glm::vec4 white = {255.0f / 255, 241.0f / 255, 232.0f / 255, 1.0f};
constexpr glm::vec4 red = {255.0f / 255, 0.0f / 255, 77.0f / 255, 1.0f};
constexpr glm::vec4 orange = {255.0f / 255, 163.0f / 255, 0.0f / 255, 1.0f};
constexpr glm::vec4 yellow = {255.0f / 255, 236.0f / 255, 39.0f / 255, 1.0f};
constexpr glm::vec4 green = {0.0f / 255, 228.0f / 255, 54.0f / 255, 1.0f};
constexpr glm::vec4 blue = {41.0f / 255, 173.0f / 255, 255.0f / 255, 1.0f};
constexpr glm::vec4 indigo = {131.0f / 255, 118.0f / 255, 156.0f / 255, 1.0f};
constexpr glm::vec4 pink = {255.0f / 255, 119.0f / 255, 168.0f / 255, 1.0f};
constexpr glm::vec4 peach = {255.0f / 255, 204.0f / 255, 170.0f / 255, 1.0f};

} // namespace pico8

inline float luminance(const glm::vec4 color) {
    return 0.2126f * powf(color.r, 2.2f) + 0.7152f * powf(color.g, 2.2f) + 0.0722f * powf(color.b, 2.2f);
}

} // namespace color
} // namespace engine
