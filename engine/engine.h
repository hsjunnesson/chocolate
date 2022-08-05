#pragma once

#include "math.inl"

#pragma warning(push, 0)
#include <inttypes.h>
#pragma warning(pop)

struct GLFWwindow;

namespace foundation {
class Allocator;
}

namespace engine {
using namespace foundation;
using namespace math;

class EngineParams;
struct Engine;
struct Input;
struct InputCommand;

struct EngineCallbacks {
    void (*on_input)(Engine &engine, void *game_object, InputCommand &input_command) = nullptr;
    void (*update)(Engine &engine, void *game_object, float frame_time, float delta) = nullptr;
    void (*render)(Engine &engine, void *game_object) = nullptr;
    void (*render_imgui)(Engine &engine, void *game_object) = nullptr;
    void (*on_shutdown)(Engine &engine, void *game_object) = nullptr;
};

struct Engine {
    Engine(Allocator &allocator, const char *config_path);
    ~Engine();

    Allocator &allocator;
    EngineCallbacks *engine_callbacks;
    void *game_object;
    uint64_t frames;
    GLFWwindow *glfw_window;
    Rect window_rect;
    bool window_resized;
    Input *input;
    float camera_zoom;
    int32_t render_scale;
    Vector2 camera_offset;
    bool terminating;
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
