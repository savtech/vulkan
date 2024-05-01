#pragma once

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include "platform_shared.h"
#include "vulkan_renderer.h"

struct WindowWin32 {
    WindowDescription description = {};
    HINSTANCE instance = nullptr;
    HWND handle = nullptr;
    bool should_close = false;
    //bool resizing = false;
};

struct ApplicationWin32Vulkan {
    WindowWin32 window = {};
    VulkanRenderer renderer = {};
    bool initialized = false;
    Session session;
};

void window_create(WindowWin32* window);
LRESULT window_callback(HWND handle, UINT message, WPARAM w_param, LPARAM l_param);
void console_create();
void application_update(ApplicationWin32Vulkan* application, Time::Duration delta_time);
void application_render(ApplicationWin32Vulkan* application, Time::Duration delta_time);