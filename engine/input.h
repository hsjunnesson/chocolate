#pragma once

#include "math.inl"

struct GLFWwindow;

namespace foundation {
class Allocator;
template<typename T> struct Array;
}

namespace engine {
using namespace foundation;

struct Window;

enum class Action {
    None,
    Quit,
    ZoomIn,
    ZoomOut,
    MouseMoved,
    MouseTrigger,
    MoveDown,
    MoveLeft,
    MoveRight,
    MoveUp,
};

enum class TriggerState {
    None,
    Pressed,
    Released,
    Repeated,
};

struct KeyState {
    TriggerState trigger_state = TriggerState::None;
};

struct MouseState {
    Vector2f mouse_position = {0.0f, 0.0f};
    Vector2f mouse_relative_motion = {0.0f, 0.0f};
    TriggerState mouse_left_state = TriggerState::None;
    TriggerState mouse_right_state = TriggerState::None;
};

struct InputCommand {
    Action action = Action::None;

    union {
        KeyState key_state;
        MouseState mouse_state;
    };

    InputCommand() {};
};

// The input system that keeps track of the state of inputs.
struct Input {
    Input(Allocator &allocator, GLFWwindow *glfw_window);
    ~Input();
    
    Allocator &allocator;

    // Current set of input commands.
    Array<InputCommand> *input_commands;

    // Current mouse state.
    MouseState mouse_state;
};

// Processes all pending input events.
void process_events(Input &input);

} // namespace engine
