#if defined(_WIN32)
#include <windows.h>
#include <fileapi.h>
#elif defined (__unix__) || (defined (__APPLE__) && defined (__MACH__))
#include <sys/stat.h>
#endif

#include "engine/shader.h"
#include "engine/log.h"

#include <memory.h>
#include <array.h>
#include <string_stream.h>
#include <temp_allocator.h>
#include <GLFW/glfw3.h>
#include <glad/glad.h>

#include <fstream>

namespace engine {

using namespace foundation;
using namespace array;
using namespace string_stream;

void read_file(Buffer &buffer, const char *filename) {
    // https://wiki.sei.cmu.edu/confluence/display/c/FIO19-C.+Do+not+use+fseek%28%29+and+ftell%28%29+to+compute+the+size+of+a+regular+file
#if defined(_WIN32)
    LARGE_INTEGER file_size;

    HANDLE file = CreateFile(TEXT(filename), GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

    if (INVALID_HANDLE_VALUE == file) {
        log_fatal("Could not read file %s: invalid file handle", filename);
    }
    
    if (!GetFileSizeEx(file, &file_size)) {
        log_fatal("Could not read file %s: could not get size", filename);
    }

    resize(buffer, (uint32_t)file_size.QuadPart);

    if (!ReadFile(file, begin(buffer), (uint32_t)file_size.QuadPart, NULL, NULL)) {
        log_fatal("Could not read file %s", filename);
    }

    if (!CloseHandle(file)) {
        log_fatal("Could not close file after read %s", filename);
    }
#elif defined (__unix__) || (defined (__APPLE__) && defined (__MACH__))
    struct stat info;
    if (stat(filename, &info) != 0) {
        log_fatal("Could not read file %s", filename);
    }

    resize(buffer, info.st_size);

    FILE *file = fopen(filename, "rb");
    if (!file) {
        log_fatal("Could not read file %s", filename);
    }

    fread(begin(buffer), info.st_size, 1, file);

    if (ferror(file) != 0) {
        log_fatal("Could not read file %s", filename);
    }
    
    fclose(file);
#else
    log_fatal("Unsupported platform");
#endif
}

Shader::Shader(const char *geometry_shader_file, const char *vertex_shader_file, const char *fragment_shader_file)
: program(0) {
    program = glCreateProgram();

    GLuint geometry_shader = 0;
    GLuint vertex_shader = 0;
    GLuint fragment_shader = 0;

    auto compile_shader = [program = program](const char *file_name, GLenum shader_type) -> GLint {
        TempAllocator512 ta;

        Buffer shader_program_buffer(ta);
        read_file(shader_program_buffer, file_name);

        GLuint shader = glCreateShader(shader_type);

        const char *shader_program_source = c_str(shader_program_buffer);
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

            log_fatal("Error compiling shader %s: %s", file_name, c_str(error_buffer));
        }

        glAttachShader(program, shader);

        return shader;
    };

    if (geometry_shader_file) {
        geometry_shader = compile_shader(geometry_shader_file, GL_GEOMETRY_SHADER);
    }

    if (vertex_shader_file) {
        vertex_shader = compile_shader(vertex_shader_file, GL_VERTEX_SHADER);
    }

    if (fragment_shader_file) {
        fragment_shader = compile_shader(fragment_shader_file, GL_FRAGMENT_SHADER);
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
