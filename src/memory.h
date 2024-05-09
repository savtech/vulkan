#pragma once

#include <memory>
#include "types.h"
#include <stdio.h>

struct MemoryArena {
    size_t size = 0;
    size_t used = 0;
    void* data = nullptr;
};

MemoryArena* memory_arena_create(size_t bytes) {
    MemoryArena* arena = (MemoryArena*)malloc(sizeof(MemoryArena));
    if(!arena) {
        printf("malloc() failed. [MemoryArena object allocation]\n");
        return nullptr;
    }

    arena->data = malloc(bytes);
    if(!arena->data) {
        printf("malloc() failed. [MemoryArena data allocation]\n");
        free(arena);
        return nullptr;
    }

    arena->size = bytes;
    arena->used = 0;

    return arena;
}

void* memory_arena_allocate(MemoryArena* arena, size_t bytes) {
    if(arena->used + bytes > arena->size) {
        printf("memory_arena_allocate() failed. [Not enough space to allocate %zd bytes]", bytes);
        return nullptr;
    }

    void* memory = (u8*)arena->data + arena->used;
    arena->used += bytes;
    return memory;
}

size_t KB(size_t kilobytes) {
    return kilobytes * 1024;
}

size_t MB(size_t megabytes) {
    return megabytes * KB(1024);
}

size_t GB(size_t gigabytes) {
    return gigabytes * MB(1024);
}