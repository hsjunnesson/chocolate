#include "allocator.h"

#include "memory.h"
#include "log.h"
#include <cassert>

struct ch_allocator_o {
    foundation::Allocator *allocator;
};

static struct ch_allocator_o default_allocator;

static void *allocator_alloc(struct ch_allocator_i *a, uint64_t size) {
    return a->inst->allocator->allocate(size);
}

static void allocator_dealloc(struct ch_allocator_i *a, void *ptr) {
    a->inst->allocator.deallocate(ptr);
}

static ch_allocator_i system_allocator = {.init = default_allocator, .alloc = allocator_alloc, .dealloc = allocator_dealloc};

void load_allocator() {
    static bool initialized = false;

    if (initialized) {
        ch_log_fatal("load_allocator ran twice");
    }

    foundation::memory_globals::init();
    default_allocator.allocator = &foundation::memory_globals::default_allocator();

    initialized = true;
}
