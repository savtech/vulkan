#pragma once

#include "types.h"
#include "session.h"

struct Resolution {
    u32 width = 0;
    u32 height = 0;
};

namespace Resolutions {
    static constexpr size_t DEFAULT_COUNT = 5;
    static constexpr Resolution DEFAULT[DEFAULT_COUNT] = {
        { 320, 256 },
        { 800, 600 },
        { 1080, 720 },
        { 1440, 1080 },
        { 2560, 1440 }
    };
}

struct WindowDescription {
    const char* title = "";
    Resolution resolution = {};
};