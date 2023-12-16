#pragma once

#include "api_types.h"

typedef struct ch_allocator_o ch_allocator_o;

typedef struct ch_allocator_i {
    struct ch_allocator_o *inst;

    void *(*alloc)(struct ch_allocator_i *a, uint64_t size);
    void (*free)(struct ch_allocator_i *a, void *ptr);
} ch_allocator_i;

struct ch_allocator_api {
    // malloc based default allocator.
    struct ch_allocator_i *system;
};
