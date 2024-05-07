#pragma once
#include "types.h"
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

struct ImageData {
    u8* pixels;
    i32 width;
    i32 height;
    u64 size;
};

bool load_image(const char* filename, ImageData* image_data) {
    i32 channels = 0;
    image_data->pixels = stbi_load(filename, &image_data->width, &image_data->height, &channels, 4);

    image_data->size = image_data->width * image_data->height * 4;

    return true;
}