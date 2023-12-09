#pragma once

#include "engine/math.inl"
#include <glm/glm.hpp>
#include <inttypes.h>
#include <memory_types.h>

struct GLFWwindow;

namespace engine {
struct Engine;
struct Input;
struct InputCommand;
struct Shader;

struct EngineCallbacks {
    void (*on_input)(Engine &engine, void *game_object, InputCommand &input_command) = nullptr;
    void (*update)(Engine &engine, void *game_object, float frame_time, float delta) = nullptr;
    void (*render)(Engine &engine, void *game_object) = nullptr;
    void (*render_imgui)(Engine &engine, void *game_object) = nullptr;

    // Return false to clear the should close flag.
    bool (*on_shutdown)(Engine &engine, void *game_object) = nullptr;
};

struct Engine {
    Engine(foundation::Allocator &allocator, const char *config_path);
    ~Engine();

    foundation::Allocator &allocator;

    EngineCallbacks *engine_callbacks;
    void *game_object;

    uint64_t frames;

    GLFWwindow *glfw_window;
    math::Rect window_rect;
    bool window_resizable;
    bool window_resized;
    float target_aspect_ratio;

    Input *input;

    float camera_zoom;
    int32_t render_scale;
    glm::ivec2 camera_offset;

    bool terminating;
    bool wait_vsync;
    uint32_t fps_limit;
};

// Runs the engine, returns the exit code.
int run(Engine &engine);

// Moves camera to a position.
void move_camera(Engine &engine, int32_t x, int32_t y);

// Offsets a camera by pixels.
void offset_camera(Engine &engine, int32_t x, int32_t y);

// Change camera zoom level.
void zoom_camera(Engine &engine, float camera_zoom);

// Flag the engine to terminate
void terminate(Engine &engine);

} // namespace engine
