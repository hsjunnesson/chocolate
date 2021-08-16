#pragma once

#include <inttypes.h>
#include "math.inl"

struct GLFWwindow;

namespace foundation {
class Allocator;
}

namespace engine {
using namespace foundation;

class EngineParams;
struct Engine;
struct Input;
struct InputCommand;
struct Sprites;

struct EngineCallbacks {
    void (*on_input)(Engine &engine, void *game_object, InputCommand &input_command);
    void (*update)(Engine &engine, void *game_object, float frame_time, float delta);
    void (*render)(Engine &engine, void *game_object);
};

struct Engine {
    Engine(Allocator &allocator, const char *params_path);
    ~Engine();

    Allocator &allocator;
    EngineParams *params;
    EngineCallbacks *engine_callbacks;
    void *game_object;
    uint64_t frames;
    GLFWwindow *glfw_window;
    Rect window_rect;
    Input *input;
    float camera_zoom;
    Vector2 camera_offset;
    bool terminating;
    Sprites *sprites;
};

// Runs the engine, returns the exit code.
int run(Engine &engine);

// Moves camera to a position.
void move_camera(Engine &engine, int32_t x, int32_t y);

// Offsets a camera by pixels.
void offset_camera(Engine &engine, int32_t x, int32_t y);

// Flag the engine to terminate
void terminate(Engine &engine);

} // namespace engine
