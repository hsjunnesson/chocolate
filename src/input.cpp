#include "engine/input.h"
#include "engine/engine.h"
#include "engine/log.h"

#include <array.h>

#include <GLFW/glfw3.h>


namespace engine {

using namespace foundation;

void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods) {
    (void)scancode;
    (void)mods;

    Engine *engine = (Engine *)glfwGetWindowUserPointer(window);

    if (!engine) {
        return;
    }

    InputCommand input_command;

    if (action == GLFW_PRESS) {
        input_command.key_state.trigger_state = TriggerState::Pressed;
    } else if (action == GLFW_RELEASE) {
        input_command.key_state.trigger_state = TriggerState::Released;
    } else if (action == GLFW_REPEAT) {
        input_command.key_state.trigger_state = TriggerState::Repeated;
    }

    switch (key) {
    case GLFW_KEY_ESCAPE: {
        input_command.action = Action::Quit;
        break;
    }

    default: {
        return;
    }
    }

    array::push_back(*engine->input->input_commands, input_command);
}

void cursor_callback(GLFWwindow *window, double x, double y) {
    Engine *engine = (Engine *)glfwGetWindowUserPointer(window);

    if (!engine) {
        return;
    }

    Input &input = *engine->input;

    MouseState mouse_state = input.mouse_state;
    mouse_state.mouse_relative_motion = {(float)x - mouse_state.mouse_position.x, (float)y - mouse_state.mouse_position.y};
    mouse_state.mouse_position = {(float)x, (float)y};

    input.mouse_state = mouse_state;

    InputCommand input_command;
    input_command.action = Action::MouseMoved;
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
    input_command.action = Action::MouseTrigger;
    input_command.mouse_state = mouse_state;

    array::push_back(*engine->input->input_commands, input_command);

}

Input::Input(Allocator &allocator, GLFWwindow *glfw_window)
: allocator(allocator)
, input_commands(nullptr)
, mouse_state() {
    glfwSetKeyCallback(glfw_window, key_callback);
    glfwSetCursorPosCallback(glfw_window, cursor_callback);
    glfwSetMouseButtonCallback(glfw_window, mouse_button_callback);
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

} // namespace engine
