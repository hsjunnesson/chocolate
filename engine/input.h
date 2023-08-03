#pragma once

#include "math.inl"

struct GLFWwindow;

namespace foundation {
class Allocator;
template <typename T> struct Array;
} // namespace foundation

namespace engine {
using namespace foundation;
using namespace math;

struct Engine;
struct Window;

enum class InputType {
    None,
    Mouse,
    Key,
    Scroll,
};

enum class MouseAction {
    None,
    MouseMoved,
    MouseTrigger,
};

enum class TriggerState {
    None,
    Pressed,
    Released,
    Repeated,
};

enum class CursorMode {
    Normal, // Visible cursor
    Hidden, // Hidden cursor while over the window
    //    Disabled, // Hidden and grabbed cursor, not yet supported.
};

struct KeyState {
    int16_t keycode = -1;
    TriggerState trigger_state = TriggerState::None;
    bool shift_state;
    bool alt_state;
    bool ctrl_state;
};

struct MouseState {
    MouseAction mouse_action = MouseAction::None;
    Vector2f mouse_position = {0.0f, 0.0f};
    Vector2f mouse_relative_motion = {0.0f, 0.0f};
    TriggerState mouse_left_state = TriggerState::None;
    TriggerState mouse_right_state = TriggerState::None;
};

struct ScrollState {
    double x_offset;
    double y_offset;
};

struct InputCommand {
    InputType input_type = InputType::None;

    union {
        KeyState key_state;
        MouseState mouse_state;
        ScrollState scroll_state;
    };

    InputCommand(){};
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

    // Current cursor mode.
    CursorMode cursor_mode;
    
    // Current keyboard states
    bool shift_state;
    bool alt_state;
    bool ctrl_state;
};

// Processes all pending input events.
void process_events(Input &input);

// Sets cursor mode.
void set_cursor_mode(const Engine &engine, Input &input, CursorMode cursor_mode);

} // namespace engine
