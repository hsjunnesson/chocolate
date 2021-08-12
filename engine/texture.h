#pragma once

#include <inttypes.h>

namespace foundation {
    class Allocator;
}

namespace engine {

struct Texture {
    Texture(foundation::Allocator &allocator, const char *texture_filename);

    int width;
    int height;
    uint32_t texture;
};

} // namespace engine
