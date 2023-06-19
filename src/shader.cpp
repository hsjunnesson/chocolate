#include "engine/shader.h"
#include "engine/file.h"
#include "engine/log.h"

#pragma warning(push, 0)
#include <GLFW/glfw3.h>
#include <array.h>
#include <glad/glad.h>
#include <memory.h>
#include <string_stream.h>
#include <temp_allocator.h>

#include <fstream>
#pragma warning(pop)

namespace engine {

using namespace foundation;
using namespace array;
using namespace string_stream;

Shader::Shader(const char *geometry_source, const char *vertex_source, const char *fragment_source, const char *name)
: program(0) {
    program = glCreateProgram();

    GLuint geometry_shader = 0;
    GLuint vertex_shader = 0;
    GLuint fragment_shader = 0;

    auto compile_shader = [program = this->program](const char *shader_program_source, GLenum shader_type) -> GLint {
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

    TempAllocator512 ta;

    if (geometry_source) {
        geometry_shader = compile_shader(geometry_source, GL_GEOMETRY_SHADER);

        if (name) {
            Buffer name_buffer(ta);
            name_buffer << name;
            name_buffer << " Geometry Shader";
            glObjectLabel(GL_SHADER, geometry_shader, -1, c_str(name_buffer));
        }
    }

    if (vertex_source) {
        vertex_shader = compile_shader(vertex_source, GL_VERTEX_SHADER);

        if (name) {
            Buffer name_buffer(ta);
            name_buffer << name;
            name_buffer << " Vertex Shader";
            glObjectLabel(GL_SHADER, vertex_shader, -1, c_str(name_buffer));
        }
    }

    if (fragment_source) {
        fragment_shader = compile_shader(fragment_source, GL_FRAGMENT_SHADER);

        if (name) {
            Buffer name_buffer(ta);
            name_buffer << name;
            name_buffer << " Fragment Shader";
            glObjectLabel(GL_SHADER, fragment_shader, -1, c_str(name_buffer));
        }
    }

    glLinkProgram(program);

    if (name) {
        Buffer name_buffer(ta);
        name_buffer << name;
        name_buffer << " Program";
        glObjectLabel(GL_PROGRAM, program, -1, c_str(name_buffer));
    }

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

void link_shader(Shader &shader) {
}

} // namespace engine
