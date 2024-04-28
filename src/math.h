#pragma once
#include "types.h"

void mat4_set_all(Mat4* matrix, f32 value) {
    for(size_t index = 0; index < 16; ++index) {
        matrix->data[index] = value;
    }
}