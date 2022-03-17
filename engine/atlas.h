#pragma once

#include "math.inl"
#include <collection_types.h>

namespace foundation {
typedef Array<char> Buffer;
} // namespace foundation

namespace engine {

struct Texture;

struct Atlas {
    Atlas(foundation::Allocator &allocator, const char *atlas_filename);
    ~Atlas();

    foundation::Allocator &allocator;
    foundation::Array<foundation::Buffer *> *sprite_names;
    foundation::Hash<math::Rect> *frames;
    Texture *texture;
};

// Returns a pointer to a Rect which corresponds to the named sprite of the image in the atlas.
const math::Rect *atlas_rect(const Atlas &atlas, const char *sprite_name);

} // namespace engine
