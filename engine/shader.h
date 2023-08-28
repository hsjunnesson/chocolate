#pragma once

#include <inttypes.h>

namespace engine {

struct Shader {
    Shader(const char *geometry_source, const char *vertex_source, const char *fragment_source, const char *name = nullptr);
    ~Shader();

    uint32_t program;
};

} // namespace engine
