#include "engine/texture.h"
#include "engine/log.h"

#include <GLFW/glfw3.h>
#include <algorithm>
#include <cstring>
#include <glad/glad.h>
#include <memory.h>

using namespace foundation;

namespace {

/*
void *__malloc(size_t size) {
    Allocator &allocator = memory_globals::default_allocator();
    return allocator.allocate(size);
}

void *__realloc(void *ptr, size_t new_size) {
    Allocator &allocator = memory_globals::default_allocator();

    if (!ptr) {
        return allocator.allocate(new_size);
    }

    void *new_ptr = allocator.allocate(new_size);
    size_t old_size = allocator.allocated_size(ptr);
    memcpy(new_ptr, ptr, std::min(old_size, new_size));
    allocator.deallocate(ptr); // TODO: This triggers asan

    return new_ptr;
}

void __free(void *ptr) {
    if (ptr) {
        Allocator &allocator = memory_globals::default_allocator();
        allocator.deallocate(ptr);
    }
}
*/

} // namespace

// TODO: fix memory management

// #define STBI_MALLOC(size) __malloc(size)
// #define STBI_REALLOC(ptr, new_size) __realloc(ptr, new_size)
// #define STBI_FREE(ptr) __free(ptr)

#include "engine/stb_image.h"

namespace engine {

Texture::Texture(Allocator &allocator, const char *texture_filename)
: width(0)
, height(0) {
    int channels = 0;
    unsigned char *data = stbi_load(texture_filename, &width, &height, &channels, 0);
    if (!data) {
        log_fatal("Couldn't load texture %s: %s", texture_filename, stbi_failure_reason());
    }

    unsigned char *padded_data = nullptr;

    if (channels != 4) {
        padded_data = (unsigned char *)allocator.allocate(width * height * 4);
        for (int x = 0; x < width; ++x) {
            for (int y = 0; y < height; ++y) {
                if (channels == 3) {
                    padded_data[(y * width + x) * 4 + 0] = data[(y * width + x) * 3 + 0];
                    padded_data[(y * width + x) * 4 + 1] = data[(y * width + x) * 3 + 1];
                    padded_data[(y * width + x) * 4 + 2] = data[(y * width + x) * 3 + 2];
                } else if (channels == 1) {
                    unsigned char val = data[(y * width + x) * 1 + 0];
                    padded_data[(y * width + x) * 4 + 0] = val;
                    padded_data[(y * width + x) * 4 + 1] = val;
                    padded_data[(y * width + x) * 4 + 2] = val;
                }
                padded_data[(y * width + x) * 4 + 3] = 255;
            }
        }
    }

    // normal texture
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, padded_data == nullptr ? data : padded_data);

    if (padded_data != nullptr) {
        allocator.deallocate(padded_data);
    }

    stbi_image_free(data);
}

} // namespace engine
