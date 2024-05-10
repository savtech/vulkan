
#include <stdio.h>
#include "platform_win32.h"
#include "vulkan_renderer.cpp"

static size_t resolution_index = 2;

int WinMain(HINSTANCE instance, HINSTANCE prev_instance, LPSTR cmd_line, int show_cmd_line) {
    ApplicationWin32Vulkan application = {
        .window = {
            .description = {
                .title = "Vulkan Test",
                .resolution = Resolutions::DEFAULT[resolution_index] } },
        .renderer = {}
    };

    window_create(&application.window);
    console_create();

    VulkanRendererInitInfo vulkan_renderer_init_info = {
        .renderer = &application.renderer,
        .application_name = application.window.description.title,
        .window_handle = application.window.handle,
        .window_instance = application.window.instance
    };

    VkResult result = create_renderer(&vulkan_renderer_init_info);
    if(result != VK_SUCCESS) {
        printf("Renderer creation failed: %s\n", string_VkResult(result));
    } else {
        printf("Renderer created.\n");
        application.initialized = true;
    }

    if(application.initialized) {
        SetWindowLongPtr(application.window.handle, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(&application));

        Time::Stamp start_time = Time::Clock::now();
        Time::Duration accumulator = Time::Duration::zero();
        Time::Duration delta_time = Time::Milliseconds(10);

        application.session = {
            .start = start_time,
            .fps = {
                .measurement_start_time = start_time }
        };

        ShowWindow(application.window.handle, show_cmd_line);

        //MSG win32_message;
        while(!application.window.should_close) {
            // BOOL message_result = GetMessageA(&win32_message, application.window.handle, 0, 0);
            // if(message_result > 0) {
            //     TranslateMessage(&win32_message);
            //     DispatchMessageA(&win32_message);
            // } else {
            //     break;
            // }
            MSG win32_message;
            while(PeekMessage(&win32_message, application.window.handle, 0, 0, PM_REMOVE)) {
                TranslateMessage(&win32_message);
                DispatchMessage(&win32_message);
            }

            Time::Stamp now = Time::Clock::now();
            Time::Duration frame_time = now - start_time;

            start_time = now;
            accumulator += frame_time;

            while(accumulator >= delta_time) {
                application_update(&application, delta_time);
                accumulator -= delta_time;
            }

            application_render(&application, delta_time);
        }

        vkDeviceWaitIdle(application.renderer.devices.logical.device);
    }

    session_debug_print(&application.session);

    while(getchar()) {};

    vkFreeMemory(application.renderer.devices.logical.device, application.renderer.graphics_pipeline.vertex_buffer.device_memory, nullptr);

    return 0;
}

LRESULT window_callback(HWND handle, UINT message, WPARAM wparam, LPARAM lparam) {
    ApplicationWin32Vulkan* application = reinterpret_cast<ApplicationWin32Vulkan*>(GetWindowLongPtr(handle, GWLP_USERDATA));

    switch(message) {
        case WM_DESTROY: {
            application->window.should_close = true;
            return 0;
        } break;
        case WM_CLOSE: {
            application->window.should_close = true;
            return 0;
        } break;
        case WM_SIZING: {
            //printf("WM_SIZING()\n");
            application->renderer.resizing = true;
            VkResult vkresult = resize(&application->renderer);
            if(vkresult == VK_SUCCESS) {
                application->renderer.resizing = false;
            } else {
                printf("resize() failed.\nError: %s", string_VkResult(vkresult));
            }
            return 0;
        } break;
        case WM_KEYUP: {
            char keyName[8] = "";
            if(GetKeyNameTextA((LONG)lparam, keyName, 32) != 0) {
                //printf("WM_KEYUP: %s\n", keyName);
                if(strcmp(keyName, "R") == 0) {
                    application->renderer.resizing = true;
                    if(++resolution_index >= Resolutions::DEFAULT_COUNT) {
                        resolution_index = 0;
                    }
                    Resolution next_resolution = Resolutions::DEFAULT[resolution_index];
                    RECT current_window_dimensions;
                    GetWindowRect(application->window.handle, &current_window_dimensions);
                    RECT new_window_dimensions = {
                        .left = current_window_dimensions.left,
                        .top = current_window_dimensions.top,
                        .right = current_window_dimensions.left + (LONG)next_resolution.width,
                        .bottom = current_window_dimensions.top + (LONG)next_resolution.height
                    };
                    SetWindowPos(application->window.handle, NULL, new_window_dimensions.left, new_window_dimensions.top, next_resolution.width, next_resolution.height, SWP_NOMOVE | SWP_NOZORDER);
                    VkResult vkresult = resize(&application->renderer);
                    if(vkresult == VK_SUCCESS) {
                        application->renderer.resizing = false;
                    } else {
                        printf("resize() failed.\nError: %s", string_VkResult(vkresult));
                    }
                }
                if(strcmp(keyName, "F") == 0) {
                    if(application->session.display_fps) {
                        SetWindowTextA(handle, application->window.description.title);
                    }

                    application->session.display_fps = !application->session.display_fps;
                }
            } else {
                printf("WM_KEYUP: Unknown Key\n");
            }
            return 0;
        } break;
        default: {
            return DefWindowProcA(handle, message, wparam, lparam);
        } break;
    }
}

void window_create(WindowWin32* window) {
    window->instance = GetModuleHandle(NULL);
    char class_name[256];
    strcpy_s(class_name, 256, window->description.title);
    strcat_s(class_name, 256, "_win_class");

    WNDCLASSA window_class = {
        .style = 0,
        .lpfnWndProc = window_callback,
        .cbClsExtra = 0,
        .cbWndExtra = 0,
        .hInstance = window->instance,
        .hIcon = NULL,
        .hCursor = NULL,
        .hbrBackground = NULL,
        .lpszMenuName = NULL,
        .lpszClassName = class_name
    };

    RegisterClassA(&window_class);

    RECT client_area = {
        .left = 0,
        .top = 0,
        .right = (LONG)window->description.resolution.width,
        .bottom = (LONG)window->description.resolution.height
    };

    DWORD style = WS_OVERLAPPEDWINDOW; // ^ (WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX);

    AdjustWindowRect(&client_area, style, false);
    u32 x = (u32)(window->description.resolution.width * 0.5);
    u32 y = (u32)(window->description.resolution.height * 0.5) - client_area.top;

    window->handle = CreateWindowA(class_name, window->description.title, style, x, y, client_area.right - client_area.left, client_area.bottom - client_area.top, NULL, NULL, window->instance, NULL);
}

void console_create() {
    FILE* file_stream;
    AllocConsole();
    freopen_s(&file_stream, "CONIN$", "r", stdin);
    freopen_s(&file_stream, "CONOUT$", "w", stdout);
    freopen_s(&file_stream, "CONOUT$", "w", stderr);
}

void application_update(ApplicationWin32Vulkan* application, Time::Duration delta_time) {
    session_update(&application->session, delta_time);
}

void application_render(ApplicationWin32Vulkan* application, Time::Duration delta_time) {
    if(!application->renderer.resizing) {
        if(application->renderer.fixed_frame_mode) {
            if(--application->renderer.frames_to_render == 0) {
                application->renderer.should_render = false;
            }
        }

        session_render(&application->session);

        if(application->session.display_fps) {
            char fps[16];
            sprintf_s(fps, "FPS: %.2f", application->session.fps.last_measurement);
            //SetConsoleTitleA(title);
            SetWindowTextA(application->window.handle, fps);
        }

        VkResult result = draw_frame(&application->renderer, delta_time);
        if(result != VK_SUCCESS && result != VK_ERROR_OUT_OF_DATE_KHR) {
            printf("draw_frame() failed: %s\n", string_VkResult(result));
        }
    }
}