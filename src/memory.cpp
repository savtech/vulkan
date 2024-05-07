#include "memory.h"
#include "types.h"
#include <memory>

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
    return KB(megabytes) * 1024;
}

size_t GB(size_t gigabytes) {
    return MB(gigabytes) * 1024;
}