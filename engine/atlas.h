#pragma mark once

#include "engine/math.inl"

namespace foundation {
class Allocator;
template<typename T> struct Hash;
}

namespace engine {
using namespace foundation;

struct Texture;

struct Atlas {
    Atlas(Allocator &allocator, const char *atlas_filename);
    ~Atlas();

    Allocator &allocator;
    Hash<Rect> *frames;
    Texture *texture;
};

// Returns a pointer to a Rect which corresponds to the filename of the image in the atlas.
const Rect *atlas_rect(Atlas &atlas, const char *filename);

} // namespace engine
