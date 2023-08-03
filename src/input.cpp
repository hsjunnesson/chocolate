#include "engine/input.h"
#include "engine/engine.h"
#include "engine/log.h"

#pragma warning(push, 0)
#include <array.h>

#include <GLFW/glfw3.h>

#if defined(_WIN32)
#include <Windows.h>
#endif

#pragma warning(pop)

namespace engine {

using namespace foundation;

void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods) {
    (void)scancode;
    (void)mods;

    Engine *engine = (Engine *)glfwGetWindowUserPointer(window);

    if (!engine) {
        return;
    }

    Input &input = *engine->input;

    if (key == GLFW_KEY_LEFT_SHIFT || key == GLFW_KEY_RIGHT_SHIFT) {
        if (action == GLFW_PRESS) {
            input.shift_state = true;
        } else if (action == GLFW_RELEASE) {
            input.shift_state = false;
        }
    }

    if (key == GLFW_KEY_LEFT_ALT || key == GLFW_KEY_RIGHT_ALT) {
        if (action == GLFW_PRESS) {
            input.alt_state = true;
        } else if (action == GLFW_RELEASE) {
            input.alt_state = false;
        }
    }

    if (key == GLFW_KEY_LEFT_CONTROL || key == GLFW_KEY_RIGHT_CONTROL) {
        if (action == GLFW_PRESS) {
            input.ctrl_state = true;
        } else if (action == GLFW_RELEASE) {
            input.ctrl_state = false;
        }
    }

    InputCommand input_command;
    input_command.input_type = InputType::Key;
    input_command.key_state.keycode = static_cast<int16_t>(key);
    input_command.key_state.shift_state = input.shift_state;
    input_command.key_state.alt_state = input.alt_state;
    input_command.key_state.ctrl_state = input.ctrl_state;

    if (action == GLFW_PRESS) {
        input_command.key_state.trigger_state = TriggerState::Pressed;
    } else if (action == GLFW_RELEASE) {
        input_command.key_state.trigger_state = TriggerState::Released;
    } else if (action == GLFW_REPEAT) {
        input_command.key_state.trigger_state = TriggerState::Repeated;
    }

    array::push_back(*input.input_commands, input_command);
}

void cursor_callback(GLFWwindow *window, double x, double y) {
    Engine *engine = (Engine *)glfwGetWindowUserPointer(window);

    if (!engine) {
        return;
    }

    Input &input = *engine->input;

    MouseState mouse_state = input.mouse_state;
    mouse_state.mouse_action = MouseAction::MouseMoved;
    mouse_state.mouse_relative_motion = {(float)x - mouse_state.mouse_position.x, (float)y - mouse_state.mouse_position.y};
    mouse_state.mouse_position = {(float)x, (float)y};

    input.mouse_state = mouse_state;

    InputCommand input_command;
    input_command.input_type = InputType::Mouse;
    input_command.mouse_state = mouse_state;

    array::push_back(*engine->input->input_commands, input_command);
}

void mouse_button_callback(GLFWwindow *window, int button, int action, int mods) {
    (void)mods;

    Engine *engine = (Engine *)glfwGetWindowUserPointer(window);

    if (!engine) {
        return;
    }

    Input &input = *engine->input;

    MouseState mouse_state = input.mouse_state;
    mouse_state.mouse_action = MouseAction::MouseTrigger;

    if (button == GLFW_MOUSE_BUTTON_LEFT) {
        if (action == GLFW_RELEASE) {
            mouse_state.mouse_left_state = TriggerState::Released;
        } else if (action == GLFW_PRESS) {
            mouse_state.mouse_left_state = TriggerState::Pressed;
        } else if (action == GLFW_REPEAT) {
            mouse_state.mouse_left_state = TriggerState::Repeated;
        }
    } else if (button == GLFW_MOUSE_BUTTON_RIGHT) {
        if (action == GLFW_RELEASE) {
            mouse_state.mouse_right_state = TriggerState::Released;
        } else if (action == GLFW_PRESS) {
            mouse_state.mouse_right_state = TriggerState::Pressed;
        } else if (action == GLFW_REPEAT) {
            mouse_state.mouse_right_state = TriggerState::Repeated;
        }
    } else {
        return;
    }

    input.mouse_state = mouse_state;

    InputCommand input_command;
    input_command.input_type = InputType::Mouse;
    input_command.mouse_state = mouse_state;

    array::push_back(*engine->input->input_commands, input_command);
}

void scroll_callback(GLFWwindow *window, double xoffset, double yoffset) {
    Engine *engine = (Engine *)glfwGetWindowUserPointer(window);

    if (!engine) {
        return;
    }

    ScrollState scroll_state;
    scroll_state.x_offset = xoffset;
    scroll_state.y_offset = yoffset;

    InputCommand input_command;
    input_command.input_type = InputType::Scroll;
    input_command.scroll_state = scroll_state;

    array::push_back(*engine->input->input_commands, input_command);
}

Input::Input(Allocator &allocator, GLFWwindow *glfw_window)
: allocator(allocator)
, input_commands(nullptr)
, mouse_state()
, cursor_mode(CursorMode::Normal)
, shift_state(false)
, alt_state(false)
, ctrl_state(false) {
    glfwSetKeyCallback(glfw_window, key_callback);
    glfwSetCursorPosCallback(glfw_window, cursor_callback);
    glfwSetMouseButtonCallback(glfw_window, mouse_button_callback);
    glfwSetScrollCallback(glfw_window, scroll_callback);
    glfwSetInputMode(glfw_window, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);
    glfwSetInputMode(glfw_window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    input_commands = MAKE_NEW(allocator, Array<InputCommand>, allocator);
}

Input::~Input() {
    if (input_commands) {
        MAKE_DELETE(allocator, Array, input_commands);
    }
}

void process_events(Input &input) {
    array::clear(*input.input_commands);
    glfwPollEvents();
}

void set_cursor_mode(const Engine &engine, Input &input, CursorMode cursor_mode) {
    (void)engine;

    if (cursor_mode == input.cursor_mode) {
        return;
    }

    input.cursor_mode = cursor_mode;

#if defined(_WIN32)
    CURSORINFO cursor_info;
    cursor_info.cbSize = sizeof(CURSORINFO);

    if (!GetCursorInfo(&cursor_info)) {
        DWORD last_error = GetLastError();
        log_fatal("Could not get cursor info: 0x%" PRIx64 "", last_error);
    }

    if (cursor_mode == CursorMode::Normal) {
        ShowCursor(true);
    } else if (cursor_mode == CursorMode::Hidden) {
        ShowCursor(false);
    }
#else
    log_fatal("Unsupported platform");
#endif

    // This doesn't seem to work on GLFW
    //    int cm = 0;
    //
    //    switch (cursor_mode) {
    //    case CursorMode::Normal:
    //        cm = GLFW_CURSOR_NORMAL;
    //        break;
    //    case CursorMode::Hidden:
    //        cm = GLFW_CURSOR_HIDDEN;
    //        break;
    //    case CursorMode::Disabled:
    //        cm = GLFW_CURSOR_DISABLED;
    //        break;
    //    }
    //
    //    glfwSetInputMode(engine.glfw_window, GLFW_CURSOR, cm);
}

} // namespace engine
