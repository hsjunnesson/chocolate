#pragma once

#include <inttypes.h>

namespace engine {

struct Shader {
    Shader(const char *geometry_shader_file, const char *vertex_shader_file, const char *fragment_shader_file);
    ~Shader();

    uint32_t program;
};

} // namespace engine
