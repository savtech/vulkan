#include <stdlib.h>
#include <stdio.h>
#include <vector>
#include "vulkan_renderer.h"

VkResult create_renderer(VulkanRendererInitInfo* vulkan_renderer_init_info) {
    VkResult result = VK_ERROR_UNKNOWN;

    VulkanRenderer* renderer = vulkan_renderer_init_info->renderer;

    //BIG DOG memory leak incoming if we don't create some type of struct (RendererLoadData?) to keep track of meta data that can be mapped to the allocations we've made.
    //For example: RendererLoadData.swapchain_image_count (this is a bad example because we can query most of our structs for count information already. However, we are missing certain information, like the actual allocations within nested loops)
    //This could also be useful if we want to return debug statistics for physical device selected (+device stats), swapchain images allocated, etc.
    //Currently, if a function fails, we free the memory allocated from the function (for the most part, some are left dangling still)
    //Unfortunately we don't return to previous functions to free the memory allocated there. The RendererLoadData struct could provide useful variables that
    //we can reference to free applicable allocations.

    result = create_instance(vulkan_renderer_init_info);
    if(result != VK_SUCCESS) {
        printf("create_instance() failed.\n");
        return result;
    }

    result = create_win32_surface(vulkan_renderer_init_info);
    if(result != VK_SUCCESS) {
        printf("create_win32_surface failed.\n");
        return result;
    }

    result = choose_physical_device(renderer);
    if(result != VK_SUCCESS) {
        printf("choose_physical_device() failed.\n");
        return result;
    } else {
        VkPhysicalDeviceProperties properties;
        vkGetPhysicalDeviceProperties(renderer->devices.physical.device, &properties);
        printf("Selected device: %s\n", properties.deviceName);
    }

    result = query_queue_families(renderer);
    if(result != VK_SUCCESS) {
        printf("query_queue_families() failed.\n");
        for(size_t queue_family_index = 0; queue_family_index < renderer->devices.queue_families.count; ++queue_family_index) {
            free(renderer->devices.queue_families.families[queue_family_index].priorities);
        }
        free(renderer->devices.queue_families.families);
        return result;
    }

    result = create_logical_device(renderer);
    if(result != VK_SUCCESS) {
        printf("create_logical_device() failed.\n");
        return result;
    }

    //Let's revisit this and customize to our liking, this is basically defaults from the tutorial
    result = query_swapchain_support(renderer);
    if(result != VK_SUCCESS) {
        printf("query_swapchain_support() failed.\n");
        return result;
    }

    result = create_swapchain(renderer);
    if(result != VK_SUCCESS) {
        printf("create_swapchain() failed.\n");
        return result;
    }

    result = create_graphics_pipeline(renderer);
    if(result != VK_SUCCESS) {
        printf("create_graphics_pipeline() failed.\n");
        return result;
    }

    result = create_frame_buffers(renderer);
    if(result != VK_SUCCESS) {
        printf("create_frame_buffers() failed.\n");
        return result;
    }

    result = create_command_pools(renderer);
    if(result != VK_SUCCESS) {
        printf("create_command_pools() failed.\n");
        return result;
    }

    size_t semaphore_counts[] = { 2, 2 };
    size_t fence_counts[] = { 1, 1 };

    CommandBufferAllocationInfo graphics_queue_command_buffer_allocation_info = {
        .pool_type = QueueFamilies::Type::GRAPHICS,
        .buffer_count = renderer->swapchain.MAX_FRAMES_IN_FLIGHT,
        .semaphore_count = semaphore_counts,
        .fence_count = fence_counts
    };

    result = allocate_command_buffers(renderer, &graphics_queue_command_buffer_allocation_info);
    if(result != VK_SUCCESS) {
        printf("allocate_command_buffers() [QueueFamilies::Type::GRAPHICS] failed.\n");
        return result;
    }

    CommandBufferAllocationInfo transfer_queue_command_buffer_allocation_info = {
        .pool_type = QueueFamilies::Type::TRANSFER,
        .buffer_count = 1,
        .semaphore_count = 0,
        .fence_count = 0
    };

    result = allocate_command_buffers(renderer, &graphics_queue_command_buffer_allocation_info);
    if(result != VK_SUCCESS) {
        printf("allocate_command_buffers() [QueueFamilies::Type::TRANSFER] failed.\n");
        return result;
    }

    VkDeviceSize buffer_sizes = sizeof(Vertex) * 9;
    u32 shared_buffer_queue_family_indices[] = {
        static_cast<u32>(QueueFamilies::Type::GRAPHICS),
        static_cast<u32>(QueueFamilies::Type::TRANSFER)
    };

    //Staging Buffer
    {
        BufferAllocationInfo staging_buffer_info = {
            .buffer = &renderer->graphics_pipeline.staging_buffer,
            .usage_flags = VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            .memory_properties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            .size = buffer_sizes,
            .sharing_mode = VK_SHARING_MODE_CONCURRENT,
            .queue_families_indices_count = 2,
            .queue_family_indices = shared_buffer_queue_family_indices
        };

        result = create_buffer(renderer, &staging_buffer_info);
        if(result != VK_SUCCESS) {
            printf("create_buffer() failed. [Staging Buffer]\n");
            return result;
        }
    }

    //Vertex Buffer
    {
        BufferAllocationInfo vertex_buffer_info = {
            .buffer = &renderer->graphics_pipeline.vertex_buffer,
            .usage_flags = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
            .memory_properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            .size = buffer_sizes,
            .sharing_mode = VK_SHARING_MODE_CONCURRENT,
            .queue_families_indices_count = 2,
            .queue_family_indices = shared_buffer_queue_family_indices
        };

        result = create_buffer(renderer, &vertex_buffer_info);
        if(result != VK_SUCCESS) {
            printf("create_buffer() failed. [Vertex Buffer]\n");
            return result;
        }
    }

    record_staging_command_buffer(renderer, buffer_sizes);

    //Staging Queue Submit
    {
        VkSubmitInfo submit_info = {
            .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
            .pNext = nullptr,
            .waitSemaphoreCount = 0,
            .pWaitSemaphores = nullptr,
            .pWaitDstStageMask = 0,
            .commandBufferCount = 1,
            .pCommandBuffers = &renderer->devices.command_pools[static_cast<u32>(QueueFamilies::Type::TRANSFER)].command_buffers[0],
            .signalSemaphoreCount = 0,
            .pSignalSemaphores = nullptr
        };

        vkQueueSubmit(renderer->devices.queue_families.families[1].queues[0], 1, &submit_info, VK_NULL_HANDLE);
        vkQueueWaitIdle(renderer->devices.queue_families.families[1].queues[0]);
    }

    return result;
}

VkResult create_instance(VulkanRendererInitInfo* vulkan_renderer_init_info) {
    VkResult result = VK_ERROR_UNKNOWN;

    VkApplicationInfo application_info = {
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pNext = nullptr,
        .pApplicationName = vulkan_renderer_init_info->application_name,
        .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
        .pEngineName = "No Engine",
        .engineVersion = VK_MAKE_VERSION(1, 0, 0),
        .apiVersion = VK_MAKE_API_VERSION(0, 1, 3, 280)
    };

    // u32 instance_property_count = 0;
    // vkEnumerateInstanceExtensionProperties(nullptr, &instance_property_count, nullptr);
    // VkExtensionProperties* instance_properties = (VkExtensionProperties*)malloc(sizeof(VkExtensionProperties) * instance_property_count);

    // vkEnumerateInstanceExtensionProperties(nullptr, &instance_property_count, instance_properties);

    const char* extension_names[] = { "VK_KHR_surface", "VK_KHR_win32_surface" };

    // for(u32 property_index = 0; property_index < instance_property_count; ++property_index) {
    //     printf("Extension: %s\n", instance_properties[property_index].extensionName);
    // }

    VkInstanceCreateInfo instance_create_info = {
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .pApplicationInfo = &application_info,
        .enabledLayerCount = 0,
        .ppEnabledLayerNames = nullptr,
        .enabledExtensionCount = 2,
        .ppEnabledExtensionNames = extension_names
    };

    result = vkCreateInstance(&instance_create_info, nullptr, &vulkan_renderer_init_info->renderer->instance);
    if(result != VK_SUCCESS) {
        printf("vkCreateInstance() failed.\n");
        return result;
    }

    return result;
}

VkResult create_win32_surface(VulkanRendererInitInfo* vulkan_renderer_init_info) {
    VkResult result = VK_ERROR_UNKNOWN;

    VkWin32SurfaceCreateInfoKHR win32_surface_create_info = {
        .sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR,
        .pNext = nullptr,
        .flags = 0,
        .hinstance = vulkan_renderer_init_info->window_instance,
        .hwnd = vulkan_renderer_init_info->window_handle
    };

    result = vkCreateWin32SurfaceKHR(vulkan_renderer_init_info->renderer->instance, &win32_surface_create_info, nullptr, &vulkan_renderer_init_info->renderer->surface);
    if(result != VK_SUCCESS) {
        printf("vkCreateWin32SurfaceKHR() failed.\n");
        return result;
    }

    return result;
}

VkResult choose_physical_device(VulkanRenderer* renderer) {
    VkResult result = VK_ERROR_UNKNOWN;

    size_t device_count = 0;
    result = vkEnumeratePhysicalDevices(renderer->instance, reinterpret_cast<u32*>(&device_count), nullptr);
    if(result != VK_SUCCESS) {
        printf("vkEnumeratePhysicalDevices() failed.\n");
        return result;
    }

    VkPhysicalDevice* physical_devices = (VkPhysicalDevice*)malloc(sizeof(VkPhysicalDevice) * device_count);
    result = vkEnumeratePhysicalDevices(renderer->instance, reinterpret_cast<u32*>(&device_count), physical_devices);
    if(result != VK_SUCCESS) {
        printf("vkEnumeratePhysicalDevices() failed.\n");
        return result;
    }

    //Will always return the last device found. Since we only have one, it's not an issue at the moment
    for(u32 device_index = 0; device_index < device_count; ++device_index) {
        renderer->devices.physical.device = physical_devices[device_index];
        vkGetPhysicalDeviceProperties(renderer->devices.physical.device, &renderer->devices.physical.properties);
        vkGetPhysicalDeviceMemoryProperties(renderer->devices.physical.device, &renderer->devices.physical.memory_properties);
        printf("Found Device: %s\n", renderer->devices.physical.properties.deviceName);
    }

    free(physical_devices);
    return result;
}

VkResult query_queue_families(VulkanRenderer* renderer) {
    VkResult result = VK_ERROR_UNKNOWN;

    size_t queue_family_count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(renderer->devices.physical.device, reinterpret_cast<u32*>(&queue_family_count), nullptr);

    VkQueueFamilyProperties* queue_family_properties_list = (VkQueueFamilyProperties*)malloc(sizeof(VkQueueFamilyProperties) * queue_family_count);
    vkGetPhysicalDeviceQueueFamilyProperties(renderer->devices.physical.device, reinterpret_cast<u32*>(&queue_family_count), queue_family_properties_list);

    renderer->devices.queue_families.families = (QueueFamily*)malloc(sizeof(QueueFamily) * queue_family_count);
    for(size_t queue_family_index = 0; queue_family_index < queue_family_count; ++queue_family_index) {
        QueueFamily queue_family = {
            .index = queue_family_index,
            .properties = queue_family_properties_list[queue_family_index],
        };

        queue_family.priorities = (float*)malloc(sizeof(float) * queue_family.properties.queueCount);
        for(size_t priority_index = 0; priority_index < queue_family.properties.queueCount; ++priority_index) {
            queue_family.priorities[priority_index] = 1.0f;
        }

        size_t graphics_index = static_cast<size_t>(QueueFamilies::Type::GRAPHICS);
        size_t transfer_index = static_cast<size_t>(QueueFamilies::Type::TRANSFER);
        if((queue_family_properties_list[queue_family_index].queueFlags & VK_QUEUE_GRAPHICS_BIT) &&
           !(renderer->devices.queue_families.populated_families & (1 << graphics_index))) {
            result = vkGetPhysicalDeviceSurfaceSupportKHR(renderer->devices.physical.device, static_cast<u32>(queue_family_index), renderer->surface, &queue_family.surface_support);
            if(result != VK_SUCCESS) {
                printf("vkGetPhysicalDeviceSurfaceSupportKHR() failed.\n");
                free(queue_family_properties_list);
                return result;
            }

            renderer->devices.queue_families.families[graphics_index] = queue_family;
            renderer->devices.queue_families.populated_families |= (1 << graphics_index);
        } else if(((queue_family_properties_list[queue_family_index].queueFlags & (VK_QUEUE_TRANSFER_BIT | VK_QUEUE_GRAPHICS_BIT)) == VK_QUEUE_TRANSFER_BIT) && !(renderer->devices.queue_families.populated_families & (1 << transfer_index))) {
            renderer->devices.queue_families.families[transfer_index] = queue_family;
            renderer->devices.queue_families.populated_families |= (1 << transfer_index);
        } else {
            continue;
        }

        ++renderer->devices.queue_families.count;
    }

    free(queue_family_properties_list);
    return result;
}

VkResult create_logical_device(VulkanRenderer* renderer) {
    VkResult result = VK_ERROR_UNKNOWN;

    std::vector<const char*> device_extensions = {};

    VkDeviceQueueCreateInfo* queue_create_infos = (VkDeviceQueueCreateInfo*)malloc(sizeof(VkDeviceQueueCreateInfo) * renderer->devices.queue_families.count);
    for(size_t queue_family_index = 0; queue_family_index < renderer->devices.queue_families.count; ++queue_family_index) {
        queue_create_infos[queue_family_index] = {
            .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .queueFamilyIndex = static_cast<u32>(renderer->devices.queue_families.families[queue_family_index].index),
            .queueCount = renderer->devices.queue_families.families[queue_family_index].properties.queueCount,
            .pQueuePriorities = renderer->devices.queue_families.families[queue_family_index].priorities
        };
    }

    size_t extension_properties_count = 0;
    vkEnumerateDeviceExtensionProperties(renderer->devices.physical.device, nullptr, reinterpret_cast<u32*>(&extension_properties_count), nullptr);

    VkExtensionProperties* extension_properties = (VkExtensionProperties*)malloc(sizeof(VkExtensionProperties) * extension_properties_count);
    vkEnumerateDeviceExtensionProperties(renderer->devices.physical.device, nullptr, reinterpret_cast<u32*>(&extension_properties_count), extension_properties);

    for(size_t extension_index = 0; extension_index < extension_properties_count; ++extension_index) {
        //printf("Extension property: %s\n", extension_properties[extension_index].extensionName);
        if(strcmp(extension_properties[extension_index].extensionName, VK_KHR_SWAPCHAIN_EXTENSION_NAME) == 0) {
            device_extensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
        }
        if(strcmp(extension_properties[extension_index].extensionName, VK_EXT_MEMORY_PRIORITY_EXTENSION_NAME) == 0) {
            device_extensions.push_back(VK_EXT_MEMORY_PRIORITY_EXTENSION_NAME);
        }
    }

    VkPhysicalDevicePageableDeviceLocalMemoryFeaturesEXT pageable_device_local_memory_feature_extension = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PAGEABLE_DEVICE_LOCAL_MEMORY_FEATURES_EXT,
        .pNext = nullptr
    };

    VkPhysicalDeviceFeatures2 physical_device_features = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2,
        .pNext = &pageable_device_local_memory_feature_extension
    };

    vkGetPhysicalDeviceFeatures2(renderer->devices.physical.device, &physical_device_features);

    if(pageable_device_local_memory_feature_extension.pageableDeviceLocalMemory == VK_TRUE) {
        device_extensions.push_back(VK_EXT_PAGEABLE_DEVICE_LOCAL_MEMORY_EXTENSION_NAME);
    }

    VkDeviceCreateInfo device_create_info = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .pNext = &physical_device_features,
        .flags = 0,
        .queueCreateInfoCount = static_cast<u32>(renderer->devices.queue_families.count),
        .pQueueCreateInfos = queue_create_infos,
        .enabledLayerCount = 0,
        .ppEnabledLayerNames = nullptr,
        .enabledExtensionCount = static_cast<u32>(device_extensions.size()),
        .ppEnabledExtensionNames = device_extensions.data(),
        .pEnabledFeatures = nullptr
    };

    result = vkCreateDevice(renderer->devices.physical.device, &device_create_info, nullptr, &renderer->devices.logical.device);
    if(result != VK_SUCCESS) {
        printf("vkCreateDevice() failed.\n");
        free(extension_properties);
        free(queue_create_infos);
        return result;
    }

    for(size_t queue_family_index = 0; queue_family_index < renderer->devices.queue_families.count; ++queue_family_index) {
        renderer->devices.queue_families.families[queue_family_index].queues = (VkQueue*)malloc(sizeof(VkQueue) * renderer->devices.queue_families.families[queue_family_index].properties.queueCount);
        for(size_t queue_index = 0; queue_index < renderer->devices.queue_families.families[queue_family_index].properties.queueCount; ++queue_index) {
            vkGetDeviceQueue(renderer->devices.logical.device, static_cast<u32>(renderer->devices.queue_families.families[queue_family_index].index), static_cast<u32>(queue_index), &renderer->devices.queue_families.families[queue_family_index].queues[queue_index]);
        }
    }

    free(extension_properties);
    free(queue_create_infos);
    return result;
}

VkResult query_swapchain_support(VulkanRenderer* renderer) {
    VkResult result = VK_ERROR_UNKNOWN;

    result = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(renderer->devices.physical.device, renderer->surface, &renderer->swapchain.support_info.capabilities);
    if(result != VK_SUCCESS) {
        printf("vkGetPhysicalDeviceSurfaceCapabilitiesKHR() failed.\n");
        return result;
    }

    size_t surface_format_count = 0;
    result = vkGetPhysicalDeviceSurfaceFormatsKHR(renderer->devices.physical.device, renderer->surface, reinterpret_cast<u32*>(&surface_format_count), nullptr);
    if(result != VK_SUCCESS) {
        printf("vkGetPhysicalDeviceSurfaceFormatsKHR() failed.\n");
        return result;
    }

    renderer->swapchain.support_info.surface_formats = (VkSurfaceFormatKHR*)malloc(sizeof(VkSurfaceFormatKHR) * surface_format_count);
    result = vkGetPhysicalDeviceSurfaceFormatsKHR(renderer->devices.physical.device, renderer->surface, reinterpret_cast<u32*>(&surface_format_count), renderer->swapchain.support_info.surface_formats);
    if(result != VK_SUCCESS) {
        printf("vkGetPhysicalDeviceSurfaceFormatsKHR() failed.\n");
        free(renderer->swapchain.support_info.surface_formats);
        return result;
    }

    //printf("Surface Formats supported:\n");
    for(size_t i = 0; i < surface_format_count; ++i) {
        VkSurfaceFormatKHR surface_format = renderer->swapchain.support_info.surface_formats[i];
        if(surface_format.format == VK_FORMAT_B8G8R8A8_SRGB && surface_format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            renderer->swapchain.surface_format = surface_format;
        }
        //printf("%s [%s]\n", string_VkFormat(surface_format.format), string_VkColorSpaceKHR(surface_format.colorSpace));
    }

    size_t present_mode_count = 0;
    result = vkGetPhysicalDeviceSurfacePresentModesKHR(renderer->devices.physical.device, renderer->surface, reinterpret_cast<u32*>(&present_mode_count), nullptr);
    if(result != VK_SUCCESS) {
        printf("vkGetPhysicalDeviceSurfacePresentModesKHR() failed.\n");
        free(renderer->swapchain.support_info.surface_formats);
        return result;
    }

    renderer->swapchain.support_info.present_modes = (VkPresentModeKHR*)malloc(sizeof(VkPresentModeKHR) * present_mode_count);
    result = vkGetPhysicalDeviceSurfacePresentModesKHR(renderer->devices.physical.device, renderer->surface, reinterpret_cast<u32*>(&present_mode_count), renderer->swapchain.support_info.present_modes);
    if(result != VK_SUCCESS) {
        printf("vkGetPhysicalDeviceSurfacePresentModesKHR() failed.\n");
        free(renderer->swapchain.support_info.surface_formats);
        free(renderer->swapchain.support_info.present_modes);
        return result;
    }

    //printf("Present Modes supported:\n");
    renderer->swapchain.present_mode = VK_PRESENT_MODE_FIFO_KHR;
    for(size_t i = 0; i < present_mode_count; ++i) {
        VkPresentModeKHR present_mode = renderer->swapchain.support_info.present_modes[i];

        //printf("%s\n", string_VkPresentModeKHR(present_mode));
        if(present_mode == VK_PRESENT_MODE_MAILBOX_KHR) {
            renderer->swapchain.present_mode = present_mode;
        }
    }
    //printf("Selected Present Mode: %s\n", string_VkPresentModeKHR(renderer->swapchain.present_mode));

    return result;
}

VkResult create_swapchain(VulkanRenderer* renderer) {
    VkResult result = VK_ERROR_UNKNOWN;

    renderer->swapchain.extent = renderer->swapchain.support_info.capabilities.currentExtent;

    VkSwapchainCreateInfoKHR swapchain_create_info = {
        .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .pNext = nullptr,
        .flags = 0,
        .surface = renderer->surface,
        .minImageCount = renderer->swapchain.support_info.capabilities.minImageCount + 1, //Adding +1 to the minimum image count allots us an extra frame to render to. Freeing us from waiting on device operations between frames.
        .imageFormat = renderer->swapchain.surface_format.format,
        .imageColorSpace = renderer->swapchain.surface_format.colorSpace,
        .imageExtent = renderer->swapchain.extent,
        .imageArrayLayers = 1,
        .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        .imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .queueFamilyIndexCount = 0,
        .pQueueFamilyIndices = nullptr,
        .preTransform = renderer->swapchain.support_info.capabilities.currentTransform,
        .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
        .presentMode = renderer->swapchain.present_mode,
        .clipped = VK_TRUE,
        .oldSwapchain = renderer->swapchain.swapchain
    };

    result = vkCreateSwapchainKHR(renderer->devices.logical.device, &swapchain_create_info, nullptr, &renderer->swapchain.swapchain);
    if(result != VK_SUCCESS) {
        printf("vkCreateSwapchainKHR() failed.\n");
        return result;
    }

    result = vkGetSwapchainImagesKHR(renderer->devices.logical.device, renderer->swapchain.swapchain, reinterpret_cast<u32*>(&renderer->swapchain.images.count), nullptr);
    if(result != VK_SUCCESS) {
        printf("vkGetSwapchainImagesKHR() failed.\n");
        return result;
    } else {
        //printf("Swapchain allocated %zd images\n", renderer->swapchain.images.count);
    }

    renderer->swapchain.images.images = (VkImage*)malloc(sizeof(VkImage) * renderer->swapchain.images.count);
    result = vkGetSwapchainImagesKHR(renderer->devices.logical.device, renderer->swapchain.swapchain, reinterpret_cast<u32*>(&renderer->swapchain.images.count), renderer->swapchain.images.images);
    if(result != VK_SUCCESS) {
        printf("vkGetSwapchainImagesKHR() failed.\n");
        free(renderer->swapchain.images.images);
        return result;
    }

    renderer->swapchain.images.views = (VkImageView*)malloc(sizeof(VkImageView) * renderer->swapchain.images.count);
    for(size_t image_index = 0; image_index < renderer->swapchain.images.count; ++image_index) {
        VkImageViewCreateInfo image_view_create_info = {
            .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .image = renderer->swapchain.images.images[image_index],
            .viewType = VK_IMAGE_VIEW_TYPE_2D,
            .format = renderer->swapchain.surface_format.format,
            .components = {
                .r = VK_COMPONENT_SWIZZLE_IDENTITY,
                .g = VK_COMPONENT_SWIZZLE_IDENTITY,
                .b = VK_COMPONENT_SWIZZLE_IDENTITY,
                .a = VK_COMPONENT_SWIZZLE_IDENTITY },
            .subresourceRange = { .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT, .baseMipLevel = 0, .levelCount = 1, .baseArrayLayer = 0, .layerCount = 1 }
        };

        result = vkCreateImageView(renderer->devices.logical.device, &image_view_create_info, nullptr, &renderer->swapchain.images.views[image_index]);
        if(result != VK_SUCCESS) {
            printf("vkCreateImageView() failed. Image View[%zd]\n", image_index);
            free(renderer->swapchain.images.images);
            free(renderer->swapchain.images.views);
            return result;
        }
    }

    return result;
}

VkResult load_shader_data(VulkanRenderer* renderer, size_t* shader_count, ShaderData* shader_data) {
    VkResult result = VK_ERROR_UNKNOWN;

    WIN32_FIND_DATAA file_data;
    HANDLE find;

    char shader_directory[MAX_PATH] = "shaders/compiled/";
    char search[MAX_PATH];
    strcpy_s(search, MAX_PATH, shader_directory);
    strcat_s(search, MAX_PATH, "*");

    find = FindFirstFileA(search, &file_data);
    if(find == INVALID_HANDLE_VALUE) {
        printf("No shaders found in ../shaders/compiled\n");
        return result;
    }

    if(shader_data) {
        shader_data->shaders = (Shader*)malloc(sizeof(Shader) * shader_data->count);
        shader_data->modules = (VkShaderModule*)malloc(sizeof(VkShaderModule) * shader_data->count);
    }

    size_t shader_index = 0;
    do {
        if(strcmp(file_data.cFileName, ".") == 0 || strcmp(file_data.cFileName, "..") == 0) {
            continue;
        }

        char file_name[MAX_PATH] = "";
        strcpy_s(file_name, MAX_PATH, file_data.cFileName);

        char* extension = nullptr;
        char shader_file_name[MAX_PATH] = "";
        strcpy_s(shader_file_name, MAX_PATH, strtok_s(file_name, ".", &extension));

        if(strcmp(extension, "spv") != 0) {
            printf("Ignoring file: %s [Non-SPIRV (No .spv extension)]\n", file_data.cFileName);
            continue;
        }

        if(shader_data == nullptr) {
            ++*shader_count;
            continue;
        }

        char* shader_type = nullptr;
        strtok_s(shader_file_name, "_", &shader_type);

        shader_data->shaders[shader_index] = {};
        strcpy_s(shader_data->shaders[shader_index].file_path, MAX_PATH, shader_directory);
        strcat_s(shader_data->shaders[shader_index].file_path, MAX_PATH, file_data.cFileName);

        if(strcmp(shader_type, "vert") == 0) {
            shader_data->shaders[shader_index].type = VK_SHADER_STAGE_VERTEX_BIT;
        }
        if(strcmp(shader_type, "frag") == 0) {
            shader_data->shaders[shader_index].type = VK_SHADER_STAGE_FRAGMENT_BIT;
        }

        FILE* shader_file = fopen(shader_data->shaders[shader_index].file_path, "rb");
        if(shader_file) {
            fseek(shader_file, 0, SEEK_END);

            shader_data->shaders[shader_index].file_size = ftell(shader_file);
            shader_data->shaders[shader_index].data = malloc(shader_data->shaders[shader_index].file_size);

            fseek(shader_file, 0, SEEK_SET);
            if(fread(shader_data->shaders[shader_index].data, sizeof(char), shader_data->shaders[shader_index].file_size, shader_file) == shader_data->shaders[shader_index].file_size) {
                VkShaderModuleCreateInfo shader_create_info = {
                    .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
                    .pNext = nullptr,
                    .flags = 0,
                    .codeSize = shader_data->shaders[shader_index].file_size,
                    .pCode = reinterpret_cast<const u32*>(shader_data->shaders[shader_index].data)
                };

                result = vkCreateShaderModule(renderer->devices.logical.device, &shader_create_info, nullptr, &shader_data->modules[shader_index]);
                if(result != VK_SUCCESS) {
                    printf("Failed to load %s\n", shader_data->shaders[shader_index].file_path);
                    //free shader data stuff
                    return result;
                } else {
                    printf("Loaded %s\n", shader_data->shaders[shader_index].file_path);
                }

            } else {
                printf("File %s not found.\n", shader_data->shaders[shader_index].file_path);
                //check shader_data null, delete if necessary
                continue;
            }

            ++shader_index;
        }
        fclose(shader_file);
    } while(FindNextFileA(find, &file_data) != 0);
    FindClose(find);

    if(shader_data == nullptr) {
        return VK_SUCCESS;
    }

    return result;
}

void destroy_shader_data(VulkanRenderer* renderer) {
}

VkResult create_graphics_pipeline(VulkanRenderer* renderer) {
    VkResult result = VK_ERROR_UNKNOWN;

    result = load_shader_data(renderer, &renderer->graphics_pipeline.shader_data.count, nullptr);
    if(renderer, renderer->graphics_pipeline.shader_data.count == 0) {
        return result;
    } else {
        //printf("Shader Count: %zd\n", renderer->graphics_pipeline.shader_data.count);
    }

    result = load_shader_data(renderer, &renderer->graphics_pipeline.shader_data.count, &renderer->graphics_pipeline.shader_data);
    if(result != VK_SUCCESS) {
        destroy_shader_data(renderer);
        return result;
    }

    VkPipelineShaderStageCreateInfo* pipeline_shader_stage_create_infos = (VkPipelineShaderStageCreateInfo*)malloc(sizeof(VkPipelineShaderStageCreateInfo) * renderer->graphics_pipeline.shader_data.count);
    for(size_t shader_index = 0; shader_index < renderer->graphics_pipeline.shader_data.count; ++shader_index) {
        pipeline_shader_stage_create_infos[shader_index] = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .stage = renderer->graphics_pipeline.shader_data.shaders[shader_index].type,
            .module = renderer->graphics_pipeline.shader_data.modules[shader_index],
            .pName = "main",
            .pSpecializationInfo = nullptr
        };
    }

    VkVertexInputBindingDescription vertex_binding_description = {
        .binding = 0,
        .stride = sizeof(Vertex),
        .inputRate = VK_VERTEX_INPUT_RATE_VERTEX
    };

    VkVertexInputAttributeDescription position_attribute_description = {
        .location = 0,
        .binding = 0,
        .format = VkFormat::VK_FORMAT_R32G32_SFLOAT,
        .offset = 0 //offsetof(Vertex, position)
    };

    VkVertexInputAttributeDescription color_attribute_description = {
        .location = 1,
        .binding = 0,
        .format = VkFormat::VK_FORMAT_R32G32B32_SFLOAT,
        .offset = 8 //offsetof(Vertex, color)
    };

    VkVertexInputBindingDescription vertex_input_binding_descriptions[] = {
        vertex_binding_description
    };

    VkVertexInputAttributeDescription vertex_input_attribute_descriptions[] = {
        position_attribute_description,
        color_attribute_description
    };

    VkPipelineVertexInputStateCreateInfo pipeline_vertex_input_state_create_info = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .vertexBindingDescriptionCount = 1,
        .pVertexBindingDescriptions = vertex_input_binding_descriptions,
        .vertexAttributeDescriptionCount = 2,
        .pVertexAttributeDescriptions = vertex_input_attribute_descriptions
    };

    VkPipelineInputAssemblyStateCreateInfo pipeline_input_assembly_state_create_info = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .topology = VkPrimitiveTopology::VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
        .primitiveRestartEnable = VK_FALSE
    };

    VkPipelineTessellationStateCreateInfo pipeline_tesselation_state_create_info = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .patchControlPoints = 0
    };

    //We're trying out the dynamic states, but this is the explicit version, maybe the source of our triangle not being scaled/centered on resize
    // VkViewport viewport = {
    //     .x = 0,
    //     .y = 0,
    //     .width = static_cast<f32>(renderer->swapchain.extent.width),
    //     .height = static_cast<f32>(renderer->swapchain.extent.height),
    //     .minDepth = 0.0f,
    //     .maxDepth = 1.0f
    // };

    // VkRect2D scissor = {
    //     .offset = { 0, 0 },
    //     .extent = renderer->swapchain.extent
    // };

    VkPipelineViewportStateCreateInfo pipeline_viewport_state_create_info = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .viewportCount = 1,
        .pViewports = nullptr, //&viewport,
        .scissorCount = 1,
        .pScissors = nullptr //&scissor
    };

    VkPipelineRasterizationStateCreateInfo pipeline_rasterization_state_create_info = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .depthClampEnable = VK_FALSE,
        .rasterizerDiscardEnable = VK_FALSE,
        .polygonMode = VkPolygonMode::VK_POLYGON_MODE_FILL,
        .cullMode = VkCullModeFlagBits::VK_CULL_MODE_BACK_BIT,
        .frontFace = VkFrontFace::VK_FRONT_FACE_CLOCKWISE,
        .depthBiasEnable = VK_FALSE,
        .depthBiasConstantFactor = 0.0f,
        .depthBiasClamp = 0.0f,
        .depthBiasSlopeFactor = 0.0f,
        .lineWidth = 1.0f
    };

    VkPipelineMultisampleStateCreateInfo pipeline_multisample_state_create_info = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .rasterizationSamples = VkSampleCountFlagBits::VK_SAMPLE_COUNT_1_BIT,
        .sampleShadingEnable = VK_FALSE,
        .minSampleShading = 1.0f,
        .pSampleMask = nullptr,
        .alphaToCoverageEnable = VK_FALSE,
        .alphaToOneEnable = VK_FALSE
    };

    VkPipelineDepthStencilStateCreateInfo pipeline_depth_stencil_state_create_info = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .depthTestEnable = VK_FALSE,
        .depthWriteEnable = VK_FALSE,
        .depthBoundsTestEnable = VK_FALSE,
        .stencilTestEnable = VK_FALSE,
        .front = {},
        .back = {},
        .minDepthBounds = 0.0f,
        .maxDepthBounds = 1.0f
    };

    //The tutorial only has one attachment, let's try that
    // VkPipelineColorBlendAttachmentState* pipeline_color_blend_attachment_states = (VkPipelineColorBlendAttachmentState*)malloc(sizeof(VkPipelineColorBlendAttachmentState) * renderer->swapchain.images.count);
    // for(size_t image_index = 0; image_index < renderer->swapchain.images.count; ++image_index) {
    //     pipeline_color_blend_attachment_states[image_index] = {
    //         .blendEnable = VK_FALSE,
    //         .srcColorBlendFactor = VkBlendFactor::VK_BLEND_FACTOR_ONE,
    //         .dstColorBlendFactor = VkBlendFactor::VK_BLEND_FACTOR_ZERO,
    //         .colorBlendOp = VkBlendOp::VK_BLEND_OP_ADD,
    //         .srcAlphaBlendFactor = VkBlendFactor::VK_BLEND_FACTOR_ONE,
    //         .dstAlphaBlendFactor = VkBlendFactor::VK_BLEND_FACTOR_ZERO,
    //         .alphaBlendOp = VkBlendOp::VK_BLEND_OP_ADD,
    //         .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT
    //     };
    // }

    VkPipelineColorBlendAttachmentState pipeline_color_blend_attachment_state = {
        .blendEnable = VK_FALSE,
        .srcColorBlendFactor = VkBlendFactor::VK_BLEND_FACTOR_ONE,
        .dstColorBlendFactor = VkBlendFactor::VK_BLEND_FACTOR_ZERO,
        .colorBlendOp = VkBlendOp::VK_BLEND_OP_ADD,
        .srcAlphaBlendFactor = VkBlendFactor::VK_BLEND_FACTOR_ONE,
        .dstAlphaBlendFactor = VkBlendFactor::VK_BLEND_FACTOR_ZERO,
        .alphaBlendOp = VkBlendOp::VK_BLEND_OP_ADD,
        .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT
    };

    VkPipelineColorBlendStateCreateInfo pipeline_color_blend_state_create_info = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0, //Do we want VK_PIPELINE_COLOR_BLEND_STATE_CREATE_RASTERIZATION_ORDER_ATTACHMENT_ACCESS_BIT_EXT to hook into the implicit synchro steps? Probably not
        .logicOpEnable = VK_FALSE,
        .logicOp = VkLogicOp::VK_LOGIC_OP_COPY,
        .attachmentCount = 1, //static_cast<u32>(renderer->swapchain.images.count),
        .pAttachments = &pipeline_color_blend_attachment_state,
        .blendConstants = { 0.0f, 0.0f, 0.0f, 0.0f }
    };

    //Here's our dynamic states, see above
    VkDynamicState dynamic_states[] = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR
    };

    VkPipelineDynamicStateCreateInfo pipeline_dynamic_state_create_info = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .dynamicStateCount = 2,
        .pDynamicStates = dynamic_states
    };

    VkPipelineLayoutCreateInfo pipeline_layout_create_info = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .setLayoutCount = 0,
        .pSetLayouts = nullptr,
        .pushConstantRangeCount = 0,
        .pPushConstantRanges = nullptr
    };

    result = vkCreatePipelineLayout(renderer->devices.logical.device, &pipeline_layout_create_info, nullptr, &renderer->graphics_pipeline.layout);
    if(result != VK_SUCCESS) {
        printf("vkCreatePipelineLayout() failed.\n");
        return result;
    }

    VkAttachmentDescription color_attachment = {
        .flags = 0,
        .format = renderer->swapchain.surface_format.format,
        .samples = VkSampleCountFlagBits::VK_SAMPLE_COUNT_1_BIT,
        .loadOp = VkAttachmentLoadOp::VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp = VkAttachmentStoreOp::VK_ATTACHMENT_STORE_OP_STORE,
        .stencilLoadOp = VkAttachmentLoadOp::VK_ATTACHMENT_LOAD_OP_DONT_CARE,
        .stencilStoreOp = VkAttachmentStoreOp::VK_ATTACHMENT_STORE_OP_DONT_CARE,
        .initialLayout = VkImageLayout::VK_IMAGE_LAYOUT_UNDEFINED,
        .finalLayout = VkImageLayout::VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
    };

    VkAttachmentReference color_attachment_reference = {
        .attachment = 0,
        .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
    };

    VkSubpassDescription subpass_description = {
        .flags = 0,
        .pipelineBindPoint = VkPipelineBindPoint::VK_PIPELINE_BIND_POINT_GRAPHICS,
        .inputAttachmentCount = 0,
        .pInputAttachments = VK_NULL_HANDLE,
        //use swapchain.images.count if we need an attachment per frame buffer
        .colorAttachmentCount = 1,
        //The index of the attachment in this array is directly referenced from the fragment shader
        //with the layout(location = 0) out vec4 outColor directive
        .pColorAttachments = &color_attachment_reference,
        .pResolveAttachments = VK_NULL_HANDLE,
        .pDepthStencilAttachment = VK_NULL_HANDLE,
        .preserveAttachmentCount = 0,
        .pPreserveAttachments = VK_NULL_HANDLE
    };

    VkSubpassDependency subpass_dependency = {
        .srcSubpass = VK_SUBPASS_EXTERNAL,
        .dstSubpass = 0,
        .srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        .dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        .srcAccessMask = 0,
        .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT
    };

    VkRenderPassCreateInfo render_pass_create_info = {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .attachmentCount = 1,
        .pAttachments = &color_attachment,
        .subpassCount = 1,
        .pSubpasses = &subpass_description,
        .dependencyCount = 1,
        .pDependencies = &subpass_dependency
    };

    result = vkCreateRenderPass(renderer->devices.logical.device, &render_pass_create_info, nullptr, &renderer->graphics_pipeline.render_pass);
    if(result != VK_SUCCESS) {
        printf("ckCreateRenderPass() failed.\n");
        return result;
    }

    VkGraphicsPipelineCreateInfo graphics_pipeline_create_info = {
        .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        .pNext = nullptr,
        //Consider the optional VK_PIPELINE_CREATE_CAPTURE_STATISTICS_BIT_KHR and VK_PIPELINE_CREATE_CAPTURE_INTERNAL_REPRESENTATIONS_BIT_KHR flags to deduce some debug info?
        .flags = 0,
        .stageCount = static_cast<u32>(renderer->graphics_pipeline.shader_data.count),
        .pStages = pipeline_shader_stage_create_infos,
        .pVertexInputState = &pipeline_vertex_input_state_create_info,
        .pInputAssemblyState = &pipeline_input_assembly_state_create_info,
        .pTessellationState = &pipeline_tesselation_state_create_info,
        .pViewportState = &pipeline_viewport_state_create_info,
        .pRasterizationState = &pipeline_rasterization_state_create_info,
        .pMultisampleState = &pipeline_multisample_state_create_info,
        .pDepthStencilState = &pipeline_depth_stencil_state_create_info,
        .pColorBlendState = &pipeline_color_blend_state_create_info,
        //VVV This was true before, we're trying dynamic states at the moment
        //Our pipeline is completely explicit, no dynamic state involved
        .pDynamicState = &pipeline_dynamic_state_create_info,
        .layout = renderer->graphics_pipeline.layout,
        .renderPass = renderer->graphics_pipeline.render_pass,
        .subpass = 0,
        .basePipelineHandle = VK_NULL_HANDLE,
        .basePipelineIndex = -1
    };

    result = vkCreateGraphicsPipelines(renderer->devices.logical.device, VK_NULL_HANDLE, 1, &graphics_pipeline_create_info, nullptr, &renderer->graphics_pipeline.pipeline);
    if(result != VK_SUCCESS) {
        printf("vkCreateGraphicsPipelines() failed.\n");
        return result;
    }

    //Destroy shader modules on success

    return result;
}

VkResult create_frame_buffers(VulkanRenderer* renderer) {
    VkResult result = VK_ERROR_UNKNOWN;

    renderer->swapchain.images.frame_buffers = (VkFramebuffer*)malloc(sizeof(VkFramebuffer) * renderer->swapchain.images.count);
    for(size_t image_index = 0; image_index < renderer->swapchain.images.count; ++image_index) {
        VkFramebufferCreateInfo frame_buffer_create_info = {
            .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .renderPass = renderer->graphics_pipeline.render_pass,
            .attachmentCount = 1,
            .pAttachments = &renderer->swapchain.images.views[image_index],
            .width = renderer->swapchain.extent.width,
            .height = renderer->swapchain.extent.height,
            .layers = 1
        };

        result = vkCreateFramebuffer(renderer->devices.logical.device, &frame_buffer_create_info, nullptr, &renderer->swapchain.images.frame_buffers[image_index]);
        if(result != VK_SUCCESS) {
            printf("vkCreateFramebuffer() failed. Frame Buffer[%zd]\n", image_index);
            free(renderer->swapchain.images.frame_buffers);
            return result;
        }
    }

    return result;
}

VkResult create_command_pools(VulkanRenderer* renderer) {
    VkResult result = VK_ERROR_UNKNOWN;

    size_t command_pool_count = static_cast<size_t>(QueueFamilies::Type::COUNT);

    renderer->devices.command_pools = (CommandPool*)malloc(sizeof(CommandPool) * command_pool_count);
    for(size_t command_pool_index = 0; command_pool_index < command_pool_count; ++command_pool_index) {
        VkCommandPoolCreateInfo command_pool_create_info = {
            .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
            .pNext = nullptr,
            .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
            //The following can maybe be a function call with a signature like: size_t get_queue_family_index(QueueFamilies::Type type) and we'll keep track of specific indices
            //for graphics, compute, etc. families in some array in the QueueFamilies struct. This will always return index 0 as we only have the GRAPHICS type in the enum currently
            .queueFamilyIndex = static_cast<u32>(renderer->devices.queue_families.families[command_pool_index].index)
        };

        result = vkCreateCommandPool(renderer->devices.logical.device, &command_pool_create_info, nullptr, &renderer->devices.command_pools[command_pool_index].pool);
        if(result != VK_SUCCESS) {
            printf("vkCreateCommandPool failed. Command Pool[%zd]\n", command_pool_index);
            free(renderer->devices.command_pools);
            return result;
        }
    }

    return result;
}

VkResult allocate_command_buffers(VulkanRenderer* renderer, CommandBufferAllocationInfo* command_buffer_allocation_info) {
    VkResult result = VK_ERROR_UNKNOWN;

    //The following can maybe be a function call with a signature like: size_t get_queue_family_index(QueueFamilies::Type type) and we'll keep track of specific indices
    //for graphics, compute, etc. families in some array in the QueueFamilies struct. This will always return index 0 as we only have the GRAPHICS type in the enum currently
    size_t command_pool_index = static_cast<size_t>(command_buffer_allocation_info->pool_type);

    renderer->devices.command_pools[command_pool_index].command_buffers = (VkCommandBuffer*)malloc(sizeof(VkCommandBuffer) * command_buffer_allocation_info->buffer_count);
    renderer->devices.command_pools[command_pool_index].synchro = (Synchronization*)malloc(sizeof(Synchronization) * command_buffer_allocation_info->buffer_count);
    renderer->devices.command_pools[command_pool_index].buffer_count = command_buffer_allocation_info->buffer_count;

    VkCommandBufferAllocateInfo command_buffer_allocate_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .pNext = nullptr,
        .commandPool = renderer->devices.command_pools[command_pool_index].pool,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = static_cast<u32>(command_buffer_allocation_info->buffer_count)
    };

    result = vkAllocateCommandBuffers(renderer->devices.logical.device, &command_buffer_allocate_info, renderer->devices.command_pools[command_pool_index].command_buffers);
    if(result == VK_SUCCESS) {
        for(size_t command_buffer_index = 0; command_buffer_index < command_buffer_allocation_info->buffer_count; ++command_buffer_index) {
            renderer->devices.command_pools->synchro[command_buffer_index].semaphores = (VkSemaphore*)malloc(sizeof(VkSemaphore) * command_buffer_allocation_info->semaphore_count[command_buffer_index]);
            renderer->devices.command_pools->synchro[command_buffer_index].fences = (VkFence*)malloc(sizeof(VkFence) * command_buffer_allocation_info->fence_count[command_buffer_index]);

            VkSemaphoreCreateInfo semaphore_create_info = {
                .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
                .pNext = nullptr,
                .flags = 0
            };

            VkFenceCreateInfo fence_create_info = {
                .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
                .pNext = nullptr,
                .flags = VK_FENCE_CREATE_SIGNALED_BIT //Lets us bypass fence waiting on first frame
            };

            for(size_t semaphore_index = 0; semaphore_index < command_buffer_allocation_info->semaphore_count[command_buffer_index]; ++semaphore_index) {
                result = vkCreateSemaphore(renderer->devices.logical.device, &semaphore_create_info, nullptr, &renderer->devices.command_pools[command_pool_index].synchro[command_buffer_index].semaphores[semaphore_index]);
                if(result == VK_SUCCESS) {
                    renderer->devices.command_pools[command_pool_index].synchro->semaphore_count++;
                } else {
                    printf("vkCreateSemaphore() failed. [Semaphore Index: %zd]\n", semaphore_index);
                    return result;
                }
            }

            for(size_t fence_index = 0; fence_index < command_buffer_allocation_info->fence_count[command_buffer_index]; ++fence_index) {
                result = vkCreateFence(renderer->devices.logical.device, &fence_create_info, nullptr, &renderer->devices.command_pools[command_pool_index].synchro[command_buffer_index].fences[fence_index]);
                if(result == VK_SUCCESS) {
                    renderer->devices.command_pools[command_pool_index].synchro->fence_count++;
                } else {
                    printf("vkCreateFence() failed. [Fence Index: %zd]\n", fence_index);
                    return result;
                }
            }
        }

    } else {
        printf("vkAllocateCommandBuffers failed. Command Pool[%zd] Buffer Count[%zd]\n", command_pool_index, command_buffer_allocation_info->buffer_count);
        return result;
    }

    return result;
}

VkResult record_command_buffer(VulkanRenderer* renderer, VkCommandBuffer command_buffer, size_t image_index) {
    VkResult result = VK_ERROR_UNKNOWN;

    VkCommandBufferBeginInfo command_buffer_begin_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .pNext = nullptr,
        .flags = 0,
        .pInheritanceInfo = nullptr
    };

    result = vkBeginCommandBuffer(command_buffer, &command_buffer_begin_info);
    if(result != VK_SUCCESS) {
        printf("vkBeginCommandBuffer() failed.\n");
        return result;
    }

    VkRenderPassBeginInfo render_pass_begin_info = {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
        .pNext = nullptr,
        .renderPass = renderer->graphics_pipeline.render_pass,
        .framebuffer = renderer->swapchain.images.frame_buffers[image_index],
        .renderArea = {
            .offset = { 0, 0 },
            .extent = renderer->swapchain.extent },
        .clearValueCount = 1,
        .pClearValues = &renderer->graphics_pipeline.clear_color
    };

    vkCmdBeginRenderPass(command_buffer, &render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, renderer->graphics_pipeline.pipeline);

    //Pipeline Dynamic State stuff VVV
    VkViewport viewport = {
        .x = 0.0f,
        .y = 0.0f,
        .width = static_cast<float>(renderer->swapchain.extent.width),
        .height = static_cast<float>(renderer->swapchain.extent.height),
        .minDepth = 0.0f,
        .maxDepth = 1.0f
    };
    vkCmdSetViewport(command_buffer, 0, 1, &viewport);

    VkRect2D scissor = {
        .offset = { 0, 0 },
        .extent = renderer->swapchain.extent
    };
    vkCmdSetScissor(command_buffer, 0, 1, &scissor);
    //End Pipeline Dynamic State stuff

    VkBuffer vertex_buffers[] = { renderer->graphics_pipeline.vertex_buffer.buffer };
    VkDeviceSize offsets[] = { 0 };
    vkCmdBindVertexBuffers(command_buffer, 0, 1, vertex_buffers, offsets);
    vkCmdDraw(command_buffer, 9, 1, 0, 0);

    vkCmdEndRenderPass(command_buffer);

    result = vkEndCommandBuffer(command_buffer);
    if(result != VK_SUCCESS) {
        printf("vkEndCommandBuffer() failed.\n");
        return result;
    }

    return result;
}

VkResult draw_frame(VulkanRenderer* renderer) {
    VkResult result = VK_ERROR_UNKNOWN;

    size_t frame_index = renderer->swapchain.current_frame_index;
    size_t command_pool_index = static_cast<size_t>(QueueFamilies::Type::GRAPHICS);
    CommandPool* command_pool = &renderer->devices.command_pools[command_pool_index];

    VkFence frame_in_flight_fence = command_pool->synchro[frame_index].fences[0];
    result = vkWaitForFences(renderer->devices.logical.device, 1, &frame_in_flight_fence, VK_TRUE, UINT64_MAX);
    if(result != VK_SUCCESS) {
        printf("vkWaitForFences() failed.\n");
        return result;
    }

    VkSemaphore image_available_semaphore = command_pool->synchro[frame_index].semaphores[0];
    size_t image_index = 0;
    result = vkAcquireNextImageKHR(renderer->devices.logical.device, renderer->swapchain.swapchain, UINT64_MAX, image_available_semaphore, VK_NULL_HANDLE, reinterpret_cast<u32*>(&image_index));
    if(result == VK_ERROR_OUT_OF_DATE_KHR) {
        VkResult resize_result = resize(renderer);
        if(resize_result != VK_SUCCESS) {
            printf("resize() failed. [%s]\n", string_VkResult(resize_result));
        }
        return result;
    } else if(result != VK_SUCCESS) {
        printf("vkAcquireNextImageKHR() failed.\n");
        return result;
    }

    result = vkResetFences(renderer->devices.logical.device, 1, &frame_in_flight_fence);
    if(result != VK_SUCCESS) {
        printf("vkResetFences() failed.\n");
        return result;
    }

    VkCommandBuffer command_buffer = command_pool->command_buffers[frame_index];
    result = vkResetCommandBuffer(command_buffer, 0);
    if(result != VK_SUCCESS) {
        printf("vkResetCommandBuffer() failed.\n");
        return result;
    }

    result = record_command_buffer(renderer, command_buffer, image_index);
    if(result != VK_SUCCESS) {
        printf("record_command_buffer() failed.\n");
        return result;
    }

    VkPipelineStageFlags wait_stages[] = {
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
    };

    VkSemaphore render_finished_semaphore = command_pool->synchro[frame_index].semaphores[1];
    VkSubmitInfo submit_info = {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &image_available_semaphore,
        .pWaitDstStageMask = wait_stages,
        .commandBufferCount = 1,
        .pCommandBuffers = &command_buffer,
        .signalSemaphoreCount = 1,
        .pSignalSemaphores = &render_finished_semaphore
    };

    //Another spot that we can make use of the get_queue_family_index(QueueFamilies::Type type) function
    VkQueue queue = renderer->devices.queue_families.families[0].queues[0];
    result = vkQueueSubmit(queue, 1, &submit_info, frame_in_flight_fence);
    if(result != VK_SUCCESS) {
        printf("vkQueueSubmit() failed.\n");
        return result;
    }

    VkPresentInfoKHR present_info = {
        .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &render_finished_semaphore,
        .swapchainCount = 1,
        .pSwapchains = &renderer->swapchain.swapchain,
        .pImageIndices = reinterpret_cast<const u32*>(&image_index),
        .pResults = nullptr
    };

    result = vkQueuePresentKHR(queue, &present_info);
    if(result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
        VkResult resize_result = resize(renderer);
        if(resize_result != VK_SUCCESS) {
            printf("resize() failed. [%s]\n", string_VkResult(resize_result));
        }
        return result;
    } else if(result != VK_SUCCESS) {
        printf("vkAcquireNextImageKHR() failed.\n");
        return result;
    }

    renderer->swapchain.current_frame_index = (renderer->swapchain.current_frame_index + 1) % renderer->swapchain.MAX_FRAMES_IN_FLIGHT;

    return result;
}

VkResult resize(VulkanRenderer* renderer) {
    VkResult result = VK_ERROR_UNKNOWN;

    vkDeviceWaitIdle(renderer->devices.logical.device);

    result = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(renderer->devices.physical.device, renderer->surface, &renderer->swapchain.support_info.capabilities);
    if(result != VK_SUCCESS) {
        printf("vkGetPhysicalDeviceSurfaceCapabilitiesKHR() failed.\n");
        return result;
    }

    result = create_swapchain(renderer);
    if(result != VK_SUCCESS) {
        printf("create_swapchain() failed.\n");
        return result;
    }

    result = create_frame_buffers(renderer);
    if(result != VK_SUCCESS) {
        printf("create_frame_buffers() failed.\n");
        return result;
    }

    return result;
}

VkResult create_point_pipeline(VulkanRenderer* renderer) {
    VkResult result = VK_ERROR_UNKNOWN;

    size_t shader_count = 0;
    result = load_shader_data(renderer, &shader_count, nullptr);
    if(result != VK_SUCCESS || shader_count == 0) {
        printf("load_shader_data failed. [Shader Count: %zd]\n", shader_count);
        return result;
    }

    renderer->point_pipeline.shader_data.modules = (VkShaderModule*)malloc(sizeof(VkShaderModule) * shader_count);
    result = load_shader_data(renderer, &shader_count, &renderer->point_pipeline.shader_data);
    if(result != VK_SUCCESS || shader_count == 0) {
        printf("load_shader_data failed. [Shader Count: %zd]\n", shader_count);
        return result;
    }

    VkPipelineShaderStageCreateInfo* shader_stage_create_infos = (VkPipelineShaderStageCreateInfo*)malloc(sizeof(VkPipelineShaderStageCreateInfo) * shader_count);
    for(size_t shader_index = 0; shader_index < shader_count; ++shader_index) {
        shader_stage_create_infos[shader_index] = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .stage = renderer->point_pipeline.shader_data.shaders[shader_index].type,
            .module = renderer->point_pipeline.shader_data.modules[shader_index],
            .pName = "main",
            .pSpecializationInfo = VK_NULL_HANDLE
        };
    }

    VkVertexInputBindingDescription vertex_input_binding_description = {
        .binding = 0,
        .stride = sizeof(Vec3),
        .inputRate = VK_VERTEX_INPUT_RATE_VERTEX
    };

    VkPipelineVertexInputStateCreateInfo vertex_input_state = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .vertexBindingDescriptionCount = 1,
        .pVertexBindingDescriptions = &vertex_input_binding_description,
        .vertexAttributeDescriptionCount = 0,
        .pVertexAttributeDescriptions = nullptr
    };

    VkPipelineInputAssemblyStateCreateInfo input_assembly_state = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .topology = VkPrimitiveTopology::VK_PRIMITIVE_TOPOLOGY_POINT_LIST,
        .primitiveRestartEnable = VK_FALSE
    };

    //We left off here

    VkPipelineViewportStateCreateInfo viewport_state = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        //.viewportCount
    };

    VkGraphicsPipelineCreateInfo pipeline_create_info = {
        .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .stageCount = static_cast<u32>(shader_count),
        .pStages = shader_stage_create_infos,
        .pVertexInputState = &vertex_input_state,
        .pInputAssemblyState = &input_assembly_state,
        .pTessellationState = nullptr,
        .pViewportState = &viewport_state

    };

    result = vkCreateGraphicsPipelines(renderer->devices.logical.device, VK_NULL_HANDLE, 1, &pipeline_create_info, nullptr, &renderer->point_pipeline.pipeline);
    if(result != VK_SUCCESS) {
        printf("vkCreate");
    }

    return result;
}

VkResult create_buffer(VulkanRenderer* renderer, BufferAllocationInfo* buffer_allocation_info) {
    VkResult result;

    VkBufferCreateInfo buffer_create_info = {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .size = buffer_allocation_info->size,
        .usage = buffer_allocation_info->usage_flags,
        .sharingMode = buffer_allocation_info->sharing_mode,
        .queueFamilyIndexCount = buffer_allocation_info->queue_families_indices_count,
        .pQueueFamilyIndices = buffer_allocation_info->queue_family_indices
    };

    result = vkCreateBuffer(renderer->devices.logical.device, &buffer_create_info, nullptr, &buffer_allocation_info->buffer->buffer);
    if(result != VK_SUCCESS) {
        printf("vkCreateBuffer() failed.\n");
        return result;
    }

    VkMemoryRequirements buffer_memory_requirements;
    vkGetBufferMemoryRequirements(renderer->devices.logical.device, buffer_allocation_info->buffer->buffer, &buffer_memory_requirements);

    for(size_t memory_type_index = 0; memory_type_index < renderer->devices.physical.memory_properties.memoryTypeCount; ++memory_type_index) {
        if(
            (buffer_memory_requirements.memoryTypeBits & (1 << memory_type_index)) &&
            (buffer_allocation_info->memory_properties) & renderer->devices.physical.memory_properties.memoryTypes[memory_type_index].propertyFlags) {
            VkMemoryAllocateInfo allocate_info = {
                .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
                .pNext = nullptr,
                .allocationSize = buffer_memory_requirements.size,
                .memoryTypeIndex = (u32)memory_type_index
            };

            result = vkAllocateMemory(renderer->devices.logical.device, &allocate_info, nullptr, &buffer_allocation_info->buffer->device_memory);
            if(result != VK_SUCCESS) {
                printf("vkAllcoateMemory() failed.\n");
                return result;
            }
            break;
        }
    }

    return result;
}

VkResult record_staging_command_buffer(VulkanRenderer* renderer, VkDeviceSize size) {
    VkResult result = VK_ERROR_UNKNOWN;

    size_t command_pool_index = static_cast<size_t>(QueueFamilies::Type::TRANSFER);
    VkCommandBuffer command_buffer = renderer->devices.command_pools[command_pool_index].command_buffers[0];

    VkCommandBufferBeginInfo command_buffer_begin_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .pNext = nullptr,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
        .pInheritanceInfo = nullptr
    };

    vkBeginCommandBuffer(command_buffer, &command_buffer_begin_info);

    VkBufferCopy buffer_region = {
        .srcOffset = 0,
        .dstOffset = 0,
        .size = size
    };

    vkCmdCopyBuffer(command_buffer, renderer->graphics_pipeline.staging_buffer.buffer, renderer->graphics_pipeline.vertex_buffer.buffer, 1, &buffer_region);

    vkEndCommandBuffer(command_buffer);

    return result;
}

size_t
get_queue_family_index(QueueFamilies::Type type) {
    return 0;
}

void map_memory() {
    // result = vkBindBufferMemory(renderer->devices.logical.device, renderer->graphics_pipeline.vertex_buffer.buffer, renderer->graphics_pipeline.vertex_buffer.device_memory, 0);
    // if(result != VK_SUCCESS) {
    //     printf("vkBindBufferMemory() failed.\n");
    //     return result;
    // }

    // void* data;
    // result = vkMapMemory(renderer->devices.logical.device, renderer->graphics_pipeline.vertex_buffer.device_memory, 0, vertex_buffer_info.size, 0, &data);
    // if(result != VK_SUCCESS) {
    //     printf("vkMapMemory() failed.\n");
    //     return result;
    // }

    // memcpy(data, triangle_vertices, vertex_buffer_info.size);

    // vkUnmapMemory(renderer->devices.logical.device, renderer->graphics_pipeline.vertex_buffer.device_memory);
}