#pragma once
#include "types.h"
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

struct ImageData {
    u8* pixels;
    u32 width;
    u32 height;
    u64 size;
};

bool load_image(const char* filename, ImageData* image_data) {
    i32 channels = 0;
    i32 width = 0;
    i32 height = 0;
    image_data->pixels = stbi_load(filename, &width, &height, &channels, 4);
    image_data->size = width * height * 4;
    image_data->width = width;
    image_data->height = height;
    return true;
}