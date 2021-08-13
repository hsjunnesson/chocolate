#include "engine/engine.h"

#include "engine/input.h"
#include "engine/shader.h"
#include "engine/texture.h"
#include "engine/math.inl"
#include "engine/log.h"
#include "engine/config.inl"

#include <memory.h>
#include <temp_allocator.h>
#include <proto/engine.pb.h>
#include <string_stream.h>
#include <GLFW/glfw3.h>
#include <glad/glad.h>
#include <fstream>

// #include <glm/glm.hpp>
// #include <glm/gtc/matrix_transform.hpp>
// #include <glm/gtc/type_ptr.hpp>
// #define GLM_ENABLE_EXPERIMENTAL
// #include <glm/gtx/euler_angles.hpp>


namespace {

using namespace foundation;
using namespace string_stream;

void glfw_error_callback(int error, const char *description) {
    log_error("GLFW error %d: %s\n", error, description);
}

void APIENTRY gl_debug_callback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const char *message, const void *user_param) {
    (void)length;
    (void)user_param;

    // ignore non-significant error/warning codes
    if (id == 131169 || id == 131185 || id == 131218 || id == 131204) {
        return;
    }

    TempAllocator1024 ta;
    Buffer ss(ta);

    switch (severity) {
    case GL_DEBUG_SEVERITY_HIGH:
        ss << "Severity: high, ";
        break;
    case GL_DEBUG_SEVERITY_MEDIUM:
        ss << "Severity: medium, ";
        break;
    case GL_DEBUG_SEVERITY_LOW:
        ss << "Severity: low, ";
        break;
    case GL_DEBUG_SEVERITY_NOTIFICATION:
        ss << "Severity: notification, ";
        break;
    }

    switch (source) {
    case GL_DEBUG_SOURCE_API:
        ss << "Source: API, ";
        break;
    case GL_DEBUG_SOURCE_WINDOW_SYSTEM:
        ss << "Source: Window System, ";
        break;
    case GL_DEBUG_SOURCE_SHADER_COMPILER:
        ss << "Source: Shader Compiler, ";
        break;
    case GL_DEBUG_SOURCE_THIRD_PARTY:
        ss << "Source: Third Party, ";
        break;
    case GL_DEBUG_SOURCE_APPLICATION:
        ss << "Source: Application, ";
        break;
    case GL_DEBUG_SOURCE_OTHER:
        ss << "Source: Other, ";
        break;
    }

    switch (type) {
    case GL_DEBUG_TYPE_ERROR:
        ss << "Type: Error, ";
        break;
    case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR:
        ss << "Type: Deprecated Behaviour, ";
        break;
    case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:
        ss << "Type: Undefined Behaviour, ";
        break;
    case GL_DEBUG_TYPE_PORTABILITY:
        ss << "Type: Portability, ";
        break;
    case GL_DEBUG_TYPE_PERFORMANCE:
        ss << "Type: Performance, ";
        break;
    case GL_DEBUG_TYPE_MARKER:
        ss << "Type: Marker, ";
        break;
    case GL_DEBUG_TYPE_PUSH_GROUP:
        ss << "Type: Push Group, ";
        break;
    case GL_DEBUG_TYPE_POP_GROUP:
        ss << "Type: Pop Group, ";
        break;
    case GL_DEBUG_TYPE_OTHER:
        ss << "Type: Other, ";
        break;
    }

    ss << "(" << id << ") " << message;

    if (type == GL_DEBUG_TYPE_ERROR) {
        log_error("%s", c_str(ss));
    } else {
        log_debug("%s", c_str(ss));
    }
}

void framebuffer_size_callback(GLFWwindow *window, int width, int height) {
    (void)window;
    engine::Engine *engine = (engine::Engine *)glfwGetWindowUserPointer(window);

    if (!engine) {
        return;
    }

    engine->window_rect.size.x = width;
    engine->window_rect.size.y = height;

    glViewport(0, 0, width, height);
}

} // namespace

namespace engine {
using namespace foundation;

#define WAIT_VSYNC 1

#if !WAIT_VSYNC
static const double Update_Rate = 60;
static const double Desired_Frametime = 1.0 / Update_Rate;
#endif


Engine::Engine(Allocator &allocator, const char *params_path)
: allocator(allocator)
, params(nullptr)
, engine_callbacks(nullptr)
, game_object(nullptr)
, frames(0)
, glfw_window(nullptr)
, window_rect({{0, 0}, {0, 0}})
, input(nullptr)
, camera_offset({0, 0})
, terminating(false)
{
    params = MAKE_NEW(allocator, EngineParams);
    config::read(params_path, params);

    // glfw
    {
        glfwSetErrorCallback(glfw_error_callback);

        if (!glfwInit()) {
            log_fatal("Unable to initialize GLFW");
        }

        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
        glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
        glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GLFW_TRUE);

        glfw_window = glfwCreateWindow(params->window_width(), params->window_height(), "Court of the Unseen", nullptr, nullptr);
        if (!glfw_window) {
            glfwTerminate();
            log_fatal("Unable to create GLFW window");
        }

        glfwMakeContextCurrent(glfw_window);

        if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
            glfwDestroyWindow(glfw_window);
            glfwTerminate();
            log_fatal("Unable to initialize glad");
        }

        glEnable(GL_DEBUG_OUTPUT);
        glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
        glDebugMessageCallback(gl_debug_callback, nullptr);
        glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, nullptr, GL_TRUE);

        int swap_interval = 0;

        #if WAIT_VSYNC
            swap_interval = 1;
        #endif

        glfwSwapInterval(swap_interval);

        glfwSetWindowUserPointer(glfw_window, this);

        glfwSetFramebufferSizeCallback(glfw_window, framebuffer_size_callback);
        framebuffer_size_callback(glfw_window, params->window_width(), params->window_height());
    }

    input = MAKE_NEW(allocator, Input, allocator, glfw_window);
}

Engine::~Engine() {
    if (params) {
        MAKE_DELETE(allocator, EngineParams, params);
    }

    if (input) {
        MAKE_DELETE(allocator, Input, input);
    }

    // glfw
    {
        glfwDestroyWindow(glfw_window);
        glfwTerminate();
    }
}

void render(Engine &engine) {
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    if (engine.engine_callbacks && engine.engine_callbacks->render) {
        engine.engine_callbacks->render(engine, engine.game_object);
    }
    
    int window_width, window_height;
    glfwGetFramebufferSize(engine.glfw_window, &window_width, &window_height);

    glfwSwapBuffers(engine.glfw_window);
}

int run(Engine &engine) {
    double prev_frame_time = glfwGetTime();

    bool running = true;
    int exit_code = 0;

    double current_frame_time = prev_frame_time;
    double delta_time = current_frame_time - prev_frame_time;

    while (running) {
        // Process queued events
        if (engine.engine_callbacks && engine.engine_callbacks->on_input) {
            for (uint32_t i = 0; i < array::size(*engine.input->input_commands); ++i) {
                InputCommand &input_command = (*engine.input->input_commands)[i];
                engine.engine_callbacks->on_input(engine, engine.game_object, input_command);
            }
        }

        // Update
        if (engine.engine_callbacks && engine.engine_callbacks->update) {
            engine.engine_callbacks->update(engine, engine.game_object, current_frame_time, delta_time);
        }

        if (glfwWindowShouldClose(engine.glfw_window)) {
            if (engine.engine_callbacks && engine.engine_callbacks->on_input) {
                InputCommand quit_command;
                quit_command.action = Action::Quit;
                engine.engine_callbacks->on_input(engine, engine.game_object, quit_command);
            } else {
                terminate(engine);
            }
        }

        // Check terminate
        if (engine.terminating) {
            running = false;
            break;
        }

        render(engine);

        process_events(*engine.input);

        // Calculate frame times
        current_frame_time = glfwGetTime();
        delta_time = current_frame_time - prev_frame_time;
        prev_frame_time = current_frame_time;

        // Handle anomalies
        if (delta_time < 0) {
            delta_time = 0;
        }

#if !WAIT_VSYNC
        if (delta_time < Desired_Frametime) {
            double delay = Desired_Frametime - delta_time;
            glfwWaitEventsTimeout(delay);
            delta_time += delay;
        }
#endif

        ++engine.frames;
    }

    return exit_code;
}

void move_camera(Engine &engine, int32_t x, int32_t y) {
    engine.camera_offset.x = x;
    engine.camera_offset.y = y;
}

void offset_camera(Engine &engine, int32_t x, int32_t y) {
    engine.camera_offset.x += x;
    engine.camera_offset.y += y;
}

void terminate(Engine &engine) {
    engine.terminating = true;
}

} // namespace engine
