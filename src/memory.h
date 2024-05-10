#pragma once

#include <memory>
#include "types.h"
#include <stdio.h>

struct MemoryArena {
    size_t size = 0;
    size_t used = 0;
    void* memory = nullptr;
};

MemoryArena* memory_arena_create(size_t bytes) {
    MemoryArena* arena = (MemoryArena*)malloc(sizeof(MemoryArena));
    if(!arena) {
        printf("malloc() failed. [MemoryArena object allocation]\n");
        return nullptr;
    }

    arena->memory = malloc(bytes);
    if(!arena->memory) {
        printf("malloc() failed. [MemoryArena memory allocation]\n");
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

    void* memory = (u8*)arena->memory + arena->used;
    arena->used += bytes;
    return memory;
}

void memory_arena_free(MemoryArena* arena) {
    free(arena->memory);
    free(arena);
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