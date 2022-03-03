#pragma once

#pragma warning(push, 0)
#include <inttypes.h>
#pragma warning(pop)

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
