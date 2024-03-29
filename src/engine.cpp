#include "engine/engine.h"
#include "engine/config.h"
#include "engine/file.h"
#include "engine/ini.h"
#include "engine/input.h"
#include "engine/log.h"
#include "engine/math.inl"
#include "engine/shader.h"
#include "engine/texture.h"

#include <GLFW/glfw3.h>
#include <cassert>
#include <fstream>
#include <functional>
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <memory.h>
#include <stdlib.h>
#include <string_stream.h>
#include <temp_allocator.h>

#if defined(SUPERLUMINAL)
#include <Superluminal/PerformanceAPI.h>
#include <cstdio>
#endif

#define STB_IMAGE_IMPLEMENTATION
#include "engine/stb_image.h"

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

    // Ignore notification-level messages
    if (severity == GL_DEBUG_SEVERITY_NOTIFICATION) {
        return;
    }

    // ignore non-significant error/warning codes
    if (id == 131169 || id == 131185 || id == 131218 || id == 131204) {
        return;
    }

    // ignore debug spam
    if (type == GL_DEBUG_TYPE_PUSH_GROUP || type == GL_DEBUG_TYPE_POP_GROUP || type == GL_DEBUG_TYPE_MARKER) {
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
    engine->window_resized = true;

    // new aspect ratio
    float aspect = width / (float)height;

    // viewport dimensions
    int vp_width, vp_height;

    if (aspect > engine->target_aspect_ratio) {
        // window is too wide
        vp_height = height;
        vp_width = (int)(engine->target_aspect_ratio * vp_height);
    } else {
        // window is too tall
        vp_width = width;
        vp_height = (int)(vp_width / engine->target_aspect_ratio);
    }

    // calculate position of viewport within window
    int vp_x = (width - vp_width) / 2;
    int vp_y = (height - vp_height) / 2;

    // set viewport
    glViewport(vp_x, vp_y, vp_width, vp_height);
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

Engine::Engine(Allocator &allocator, const char *config_path)
: allocator(allocator)
, engine_callbacks(nullptr)
, game_object(nullptr)
, frames(0)
, glfw_window(nullptr)
, window_rect({{0, 0}, {0, 0}})
, window_resizable(true)
, window_resized(false)
, target_aspect_ratio(1.0f)
, input(nullptr)
, camera_zoom(1.0f)
, render_scale(1)
, camera_offset({0, 0})
, terminating(false)
, wait_vsync(true)
, fps_limit(0) {
    using namespace foundation::string_stream;

    TempAllocator1024 ta;

    int window_width = 0;
    int window_height = 0;
    bool always_on_top = false;
    Buffer window_title(ta);
    Buffer window_icon(ta);

    // Load config
    {
        Buffer buffer(ta);

        if (!file::read(buffer, config_path)) {
            log_fatal("Could not open config file %s", config_path);
        }

        ini_t *ini = ini_load(c_str(buffer), nullptr);

        if (!ini) {
            log_fatal("Could not parse config file %s", config_path);
        }

        auto read_property = [&ini](const char *section, const char *property, const std::function<void(const char *)> success) {
            const char *s = config::read_property(ini, section, property);
            if (s) {
                success(s);
            } else {
                if (!section) {
                    section = "global property";
                }

                log_fatal("Invalid config file, missing [%s] %s", section, property);
            }
        };

        if (config::has_property(ini, "engine", "render_scale")) {
            read_property("engine", "render_scale", [this](const char *property) {
                this->render_scale = atoi(property);
            });
        }

        if (config::has_property(ini, "engine", "vsync")) {
            read_property("engine", "vsync", [this](const char *property) {
                if (strcmp("true", property) == 0) {
                    this->wait_vsync = true;
                } else {
                    this->wait_vsync = false;
                }
            });
        }

        if (config::has_property(ini, "engine", "fps_limit")) {
            read_property("engine", "fps_limit", [this](const char *property) {
                this->fps_limit = atoi(property);
            });
        }

        read_property("engine", "window_width", [&window_width](const char *property) {
            window_width = atoi(property);
        });

        read_property("engine", "window_height", [&window_height](const char *property) {
            window_height = atoi(property);
        });

        if (config::has_property(ini, "engine", "window_resizable")) {
            read_property("engine", "window_resizable", [this](const char *property) {
                if (strcmp("true", property) == 0) {
                    this->window_resizable = true;
                } else {
                    this->window_resizable = false;
                }
            });
        }

        target_aspect_ratio = window_width / (float)window_height;

        read_property("engine", "title", [&window_title](const char *property) {
            window_title << property;
        });

        if (config::has_property(ini, "engine", "always_on_top")) {
            read_property("engine", "always_on_top", [&always_on_top](const char *property) {
                if (strcmp("true", property) == 0) {
                    always_on_top = true;
                }
            });
        }

        if (config::has_property(ini, "engine", "window_icon")) {
            read_property("engine", "window_icon", [&window_icon](const char *property) {
                window_icon << property;
            });
        }

        ini_destroy(ini);
    }

    // glfw
    {
        glfwSetErrorCallback(glfw_error_callback);

        if (!glfwInit()) {
            log_fatal("Unable to initialize GLFW");
        }

        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
        glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
        glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GLFW_TRUE);

        if (always_on_top) {
            glfwWindowHint(GLFW_FLOATING, GLFW_TRUE);
        }

        glfw_window = glfwCreateWindow(window_width, window_height, c_str(window_title), nullptr, nullptr);
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

#if defined(_DEBUG) && defined(WIN32)
#elif defined(__APPLE__)
#else
        glEnable(GL_DEBUG_OUTPUT);
        glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
        glDebugMessageCallback(gl_debug_callback, nullptr);
        glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, nullptr, GL_TRUE);
#endif

        int swap_interval = 0;

        if (wait_vsync) {
            swap_interval = 1;
        }

        glfwSwapInterval(swap_interval);
        glfwSetWindowUserPointer(glfw_window, this);
        glfwSetFramebufferSizeCallback(glfw_window, framebuffer_size_callback);
        framebuffer_size_callback(glfw_window, window_width, window_height);
        glfwSetWindowSizeLimits(glfw_window, window_width, window_height, GLFW_DONT_CARE, GLFW_DONT_CARE);
        glfwSetWindowAttrib(glfw_window, GLFW_RESIZABLE, this->window_resizable);

        if (!array::empty(window_icon)) {
            int width, height, channels;
            unsigned char *icon_data = stbi_load(c_str(window_icon), &width, &height, &channels, 4);
            if (icon_data == nullptr) {
                log_fatal("Could not load window icon from: %s", c_str(window_icon));
            }

            GLFWimage icon;
            icon.width = width;
            icon.height = height;
            icon.pixels = icon_data;

            glfwSetWindowIcon(glfw_window, 1, &icon);
            stbi_image_free(icon_data);
        }
    }

    input = MAKE_NEW(allocator, Input, allocator, glfw_window);

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
    MAKE_DELETE(allocator, Input, input);

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
    wait_buffer();

    glPushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, 0, -1, "render engine");
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);

    // Render game
    if (engine.engine_callbacks && engine.engine_callbacks->render) {
        glPushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, 0, -1, "render game");
        engine.engine_callbacks->render(engine, engine.game_object);
        glPopDebugGroup();
    }

    // imgui
    {
        glPushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, 0, -1, "render imgui");
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        if (engine.engine_callbacks && engine.engine_callbacks->render_imgui) {
            engine.engine_callbacks->render_imgui(engine, engine.game_object);
        }

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        glPopDebugGroup();
    }

    glPopDebugGroup();

    lock_buffer();

    glfwSwapBuffers(engine.glfw_window);
}

int run(Engine &engine) {
    assert(engine.engine_callbacks);

    float prev_frame_time = (float)glfwGetTime();

    int exit_code = 0;

    float current_frame_time = prev_frame_time;
    float delta_time = current_frame_time - prev_frame_time;

    while (true) {
#if defined(SUPERLUMINAL)
        char superluminal_event_data[256];
        snprintf(superluminal_event_data, 256, "Frame %" PRIu64 "", engine.frames);
        PerformanceAPI_BeginEvent("frame", superluminal_event_data, PERFORMANCEAPI_DEFAULT_COLOR);
#endif

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
            if (engine.engine_callbacks && engine.engine_callbacks->on_shutdown) {
                if (!engine.engine_callbacks->on_shutdown(engine, engine.game_object)) {
                    glfwSetWindowShouldClose(engine.glfw_window, GLFW_FALSE);
                }
            } else {
                terminate(engine);
            }
        }

        // Check terminate
        if (engine.terminating) {
            break;
        }

        render(engine);

        engine.window_resized = false;
        process_events(*engine.input);

        // Calculate frame times
        current_frame_time = (float)glfwGetTime();
        delta_time = current_frame_time - prev_frame_time;
        prev_frame_time = current_frame_time;

        // Handle anomalies
        if (delta_time < 0) {
            delta_time = 0;
        }

        if (!engine.wait_vsync && engine.fps_limit > 0) {
            double desired_frametime = 1.0 / engine.fps_limit;
            if (delta_time < desired_frametime) {
                double delay = desired_frametime - delta_time;
                glfwWaitEventsTimeout(delay);
            }
        }

        ++engine.frames;
#if defined(SUPERLUMINAL)
        PerformanceAPI_EndEvent();
#endif
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

void zoom_camera(Engine &engine, float camera_zoom) {
    glm::vec2 offset = {engine.camera_offset.x, engine.camera_offset.y};
    offset += glm::vec2{engine.window_rect.size.x / 2.0f, engine.window_rect.size.y / 2.0f};
    offset /= engine.camera_zoom;
    engine.camera_zoom = camera_zoom;
    offset *= camera_zoom;
    offset -= glm::vec2{engine.window_rect.size.x / 2.0f, engine.window_rect.size.y / 2.0f};
    move_camera(engine, (int32_t)floor(offset.x), (int32_t)floor(offset.y));
}

void terminate(Engine &engine) {
    engine.terminating = true;
}

} // namespace engine
