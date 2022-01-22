#include "engine/engine.h"
#include "engine/config.inl"
#include "engine/input.h"
#include "engine/log.h"
#include "engine/math.inl"
#include "engine/shader.h"
#include "engine/sprites.h"
#include "engine/texture.h"

#include <GLFW/glfw3.h>
#include <cassert>
#include <fstream>
#include <glad/glad.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <memory.h>
#include <proto/engine.pb.h>
#include <string_stream.h>
#include <temp_allocator.h>

namespace {

using namespace foundation;
using namespace string_stream;

GLsync gl_lock;

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

void lock_buffer() {
    if (gl_lock) {
        glDeleteSync(gl_lock);
    }

    gl_lock = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
}

void wait_buffer() {
    if (gl_lock) {
        while (true) {
            GLenum wait_return = glClientWaitSync(gl_lock, GL_SYNC_FLUSH_COMMANDS_BIT, 1);
            if (wait_return == GL_ALREADY_SIGNALED || wait_return == GL_CONDITION_SATISFIED) {
                return;
            }
        }
    }
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
, camera_zoom(1)
, camera_offset({0, 0})
, terminating(false)
, sprites(nullptr) {
    params = MAKE_NEW(allocator, EngineParams);
    config::read(params_path, params);

    camera_zoom = params->render_scale();

    // glfw
    {
        glfwSetErrorCallback(glfw_error_callback);

        if (!glfwInit()) {
            log_fatal("Unable to initialize GLFW");
        }

        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 4);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
        glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
        glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GLFW_TRUE);

        glfw_window = glfwCreateWindow(params->window_width(), params->window_height(), params->title().c_str(), nullptr, nullptr);
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

    sprites = MAKE_NEW(allocator, Sprites, allocator);

    // imgui
    {
        if (!IMGUI_CHECKVERSION()) {
            log_fatal("Invalid imgui version");
        }

        ImGui::CreateContext();
        ImGui_ImplGlfw_InitForOpenGL(glfw_window, true);
        ImGui_ImplOpenGL3_Init(nullptr);

        ImGui::StyleColorsDark();
    }
}

Engine::~Engine() {
    if (params) {
        MAKE_DELETE(allocator, EngineParams, params);
    }

    if (input) {
        MAKE_DELETE(allocator, Input, input);
    }

    if (sprites) {
        MAKE_DELETE(allocator, Sprites, sprites);
    }

    // imgui
    {
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();
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

    wait_buffer();

    // imgui
    {
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
    }

    render_sprites(engine, *engine.sprites);

    if (engine.engine_callbacks && engine.engine_callbacks->render) {
        engine.engine_callbacks->render(engine, engine.game_object);
    }

    // imgui
    {
        if (engine.engine_callbacks->render_imgui) {
            engine.engine_callbacks->render_imgui(engine, engine.game_object);
        }

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    }

    lock_buffer();

    glfwSwapBuffers(engine.glfw_window);
}

int run(Engine &engine) {
    assert(engine.engine_callbacks);

    float prev_frame_time = glfwGetTime();

    bool running = true;
    int exit_code = 0;

    float current_frame_time = prev_frame_time;
    float delta_time = current_frame_time - prev_frame_time;

    while (true) {
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

        if (engine.sprites) {
            update_sprites(*engine.sprites);
        }

        if (glfwWindowShouldClose(engine.glfw_window)) {
            if (engine.engine_callbacks && engine.engine_callbacks->on_shutdown) {
                engine.engine_callbacks->on_shutdown(engine, engine.game_object);
            } else {
                terminate(engine);
            }
        }

        // Check terminate
        if (engine.terminating) {
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
