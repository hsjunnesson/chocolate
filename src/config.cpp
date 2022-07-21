#include "engine/config.h"
#include "engine/log.h"

void *__config_malloc(size_t size);
void __config_free(void *ptr);

#pragma warning(push, 0)
#define INI_IMPLEMENTATION
#define INI_MALLOC(ctx, size) (__config_malloc(size))
#define INI_FREE(ctx, ptr) (__config_free(ptr))
#include "engine/ini.h"

#include <assert.h>
#include <memory.h>
#pragma warning(pop)

void *__config_malloc(size_t size) {
    foundation::Allocator &allocator = foundation::memory_globals::default_allocator();
    return allocator.allocate((uint32_t)size);
}

void __config_free(void *ptr) {
    if (ptr) {
        foundation::Allocator &allocator = foundation::memory_globals::default_allocator();
        allocator.deallocate(ptr);
    }
}

namespace engine {
namespace config {

const char *read_property(ini_t *ini, const char *section, const char *property) {
    assert(ini);
    assert(property);

    int section_index = INI_GLOBAL_SECTION;

    if (section) {
        section_index = ini_find_section(ini, section, 0);
    }

    if (section_index == INI_NOT_FOUND) {
        return nullptr;
    }

    int property_index = ini_find_property(ini, section_index, property, 0);

    if (property_index == INI_NOT_FOUND) {
        return nullptr;
    }

    return ini_property_value(ini, section_index, property_index);
}

} // namespace config
} // namespace engine
