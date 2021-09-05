#if defined(_WIN32)
#include <fileapi.h>
#include <windows.h>
#elif defined(__unix__) || (defined(__APPLE__) && defined(__MACH__))
#include <sys/stat.h>
#endif

#include "engine/file.h"
#include "engine/log.h"
#include "engine/shader.h"

#include <GLFW/glfw3.h>
#include <array.h>
#include <glad/glad.h>
#include <memory.h>
#include <string_stream.h>
#include <temp_allocator.h>

#include <fstream>

namespace engine {

using namespace foundation;
using namespace array;
using namespace string_stream;

Shader::Shader(const char *geometry_source, const char *vertex_source, const char *fragment_source)
: program(0) {
    program = glCreateProgram();

    GLuint geometry_shader = 0;
    GLuint vertex_shader = 0;
    GLuint fragment_shader = 0;

    auto compile_shader = [program = program](const char *shader_program_source, GLenum shader_type) -> GLint {
        TempAllocator512 ta;

        GLuint shader = glCreateShader(shader_type);

        glShaderSource(shader, 1, &shader_program_source, nullptr);
        glCompileShader(shader);

        int status;
        glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
        if (status == GL_FALSE) {
            Buffer error_buffer(ta);

            int log_length;
            glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &log_length);
            if (log_length > 0) {
                resize(error_buffer, log_length);
                glGetShaderInfoLog(shader, log_length, nullptr, begin(error_buffer));
            }

            log_fatal("Error compiling shader: %s", c_str(error_buffer));
        }

        glAttachShader(program, shader);

        return shader;
    };

    if (geometry_source) {
        geometry_shader = compile_shader(geometry_source, GL_GEOMETRY_SHADER);
    }

    if (vertex_source) {
        vertex_shader = compile_shader(vertex_source, GL_VERTEX_SHADER);
    }

    if (fragment_source) {
        fragment_shader = compile_shader(fragment_source, GL_FRAGMENT_SHADER);
    }

    glLinkProgram(program);

    if (geometry_shader) {
        glDetachShader(program, geometry_shader);
        glDeleteShader(geometry_shader);
    }

    if (vertex_shader) {
        glDetachShader(program, vertex_shader);
        glDeleteShader(vertex_shader);
    }

    if (fragment_shader) {
        glDetachShader(program, fragment_shader);
        glDeleteShader(fragment_shader);
    }
}

Shader::~Shader() {
    if (program) {
        glDeleteProgram(program);
    }
}

} // namespace engine
