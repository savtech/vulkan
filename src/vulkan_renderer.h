#pragma once
#define VK_USE_PLATFORM_WIN32_KHR
#include <vulkan/vulkan.h>
#include <vulkan/vk_enum_string_helper.h>
#include "types.h"

struct Vertex {
    Vec2 position;
    Vec3 color;
};

static constexpr Vertex triangle_vertices[] = {
    { { -1.0f, 0.0f }, { 1.0f, 0.0f, 0.0f } },
    { { -0.5f, -1.0f }, { 0.0f, 1.0f, 0.0f } },
    { { 0.0f, 0.0f }, { 0.0f, 0.0f, 10.0f } },

    { { 0.0f, 0.0f }, { 1.0f, 0.0f, 0.0f } },
    { { 0.5f, -1.0f }, { 0.0f, 1.0f, 0.0f } },
    { { 1.0f, 0.0f }, { 0.0f, 0.0f, 1.0f } },

    { { -0.5f, 1.0f }, { 1.0f, 0.0f, 0.0f } },
    { { 0.0f, 0.0f }, { 0.0f, 1.0f, 0.0f } },
    { { 0.5f, 1.0f }, { 0.0f, 0.0f, 1.0f } },
};

static constexpr Vertex triforce_vertices[] = {
    { { -0.5f, 0.0f }, { 1.0f, 0.0f, 0.0f } },
    { { 0.0f, -1.0f }, { 0.0f, 1.0f, 0.0f } },
    { { 0.5f, 0.0f }, { 0.0f, 0.0f, 1.0f } },

    { { -1.0f, 1.0f }, { 1.0f, 0.0f, 0.0f } },
    { { -0.5f, 0.0f }, { 0.0f, 1.0f, 0.0f } },
    { { 0.0f, 1.0f }, { 0.0f, 0.0f, 1.0f } },

    { { 0.0f, 1.0f }, { 1.0f, 0.0f, 0.0f } },
    { { 0.5f, 0.0f }, { 0.0f, 1.0f, 0.0f } },
    { { 1.0f, 1.0f }, { 0.0f, 0.0f, 1.0f } },
};

struct QueueFamily {
    size_t index = 0;
    VkQueue* queues = VK_NULL_HANDLE;
    float* priorities = nullptr;
    VkBool32 surface_support = VK_FALSE;
    VkQueueFamilyProperties properties = {};
};

struct QueueFamilies {
    enum class Type : size_t {
        GRAPHICS,
        TRANSFER,
        COUNT
    };
    size_t count = 0;
    QueueFamily* families = nullptr;
    u32 populated_families;
};

struct Synchronization {
    size_t semaphore_count = 0;
    VkSemaphore* semaphores = VK_NULL_HANDLE;
    size_t fence_count = 0;
    VkFence* fences = VK_NULL_HANDLE;
};

struct CommandPool {
    VkCommandPool pool = VK_NULL_HANDLE;
    size_t buffer_count = 0;
    VkCommandBuffer* command_buffers = VK_NULL_HANDLE;
    Synchronization* synchro = nullptr;
};

struct CommandBufferAllocationInfo {
    QueueFamilies::Type pool_type;
    size_t buffer_count = 0;
    size_t* semaphore_count = nullptr;
    size_t* fence_count = nullptr;
};

struct Swapchain {
    static constexpr size_t MAX_FRAMES_IN_FLIGHT = 2;

    struct SupportInfo {
        VkSurfaceCapabilitiesKHR capabilities = {};
        VkSurfaceFormatKHR* surface_formats = nullptr;
        VkPresentModeKHR* present_modes = nullptr;
    };

    struct Images {
        size_t count = 0;
        VkImage* images = VK_NULL_HANDLE;
        VkImageView* views = VK_NULL_HANDLE;
        VkFramebuffer* frame_buffers = VK_NULL_HANDLE;
    };

    SupportInfo support_info = {};
    VkSwapchainKHR swapchain = VK_NULL_HANDLE;
    VkPresentModeKHR present_mode = VK_PRESENT_MODE_FIFO_KHR;
    VkSurfaceFormatKHR surface_format = {};
    VkExtent2D extent = {};
    Images images = {};
    size_t current_frame_index = 0;
};

struct Shader {
    VkShaderStageFlagBits type = VkShaderStageFlagBits::VK_SHADER_STAGE_ALL;
    char file_path[MAX_PATH] = "";
    size_t file_size = 0;
    void* data = nullptr;
};

struct ShaderData {
    size_t count = 0;
    Shader* shaders = nullptr;
    VkShaderModule* modules = VK_NULL_HANDLE;
};

struct Buffer {
    VkBuffer buffer = VK_NULL_HANDLE;
    VkDeviceMemory device_memory = VK_NULL_HANDLE;
};

struct BufferAllocationInfo {
    Buffer* buffer = nullptr;
    VkBufferUsageFlags usage_flags = 0;
    VkMemoryPropertyFlags memory_properties = 0;
    VkDeviceSize size = 0;
    VkSharingMode sharing_mode;
    u32 queue_families_indices_count = 0;
    u32* queue_family_indices = nullptr;
};

struct GraphicsPipeline {
    ShaderData shader_data = {};
    VkPipelineLayout layout = VK_NULL_HANDLE;
    VkPipeline pipeline = VK_NULL_HANDLE;
    VkRenderPass render_pass = VK_NULL_HANDLE;
    VkClearValue clear_color = { .color = { 0.0f, 0.0f, 0.0f, 0.0f } };
    Buffer vertex_buffer = {};
    Buffer staging_buffer = {};
};

struct Devices {
    struct Physical {
        VkPhysicalDevice device = VK_NULL_HANDLE;
        VkPhysicalDeviceProperties properties;
        VkPhysicalDeviceMemoryProperties memory_properties;
    };

    struct Logical {
        VkDevice device = VK_NULL_HANDLE;
    };

    Physical physical;
    Logical logical;
    QueueFamilies queue_families = {};
    CommandPool* command_pools = nullptr;
};

struct VulkanRenderer {
    VkInstance instance = VK_NULL_HANDLE;
    Devices devices = {};
    VkSurfaceKHR surface = VK_NULL_HANDLE;
    Swapchain swapchain = {};
    GraphicsPipeline graphics_pipeline = {};
    GraphicsPipeline point_pipeline = {};

    bool resizing = false;
    bool should_render = true;
    bool fixed_frame_mode = false;
    size_t frames_to_render = 10;
};

struct VulkanRendererInitInfo {
    VulkanRenderer* renderer = nullptr;
    const char* application_name = "";
    HWND window_handle = NULL;
    HINSTANCE window_instance = NULL;
};

VkResult create_renderer(VulkanRendererInitInfo* vulkan_renderer_init_info);
VkResult create_instance(VulkanRendererInitInfo* vulkan_renderer_init_info);
VkResult create_win32_surface(VulkanRendererInitInfo* vulkan_renderer_init_info);
VkResult choose_physical_device(VulkanRenderer* renderer);
VkResult query_queue_families(VulkanRenderer* renderer);
VkResult create_logical_device(VulkanRenderer* renderer);
VkResult query_swapchain_support(VulkanRenderer* renderer);
VkResult create_swapchain(VulkanRenderer* renderer);
VkResult load_shader_data(VulkanRenderer* renderer, size_t* shader_count, ShaderData* shader_data);
void destroy_shader_data(VulkanRenderer* renderer);
VkResult create_graphics_pipeline(VulkanRenderer* renderer);
VkResult create_frame_buffers(VulkanRenderer* renderer);
VkResult create_command_pools(VulkanRenderer* renderer);
VkResult allocate_command_buffers(VulkanRenderer* renderer, CommandBufferAllocationInfo* command_buffer_allocation_info);
VkResult record_command_buffer(VulkanRenderer* renderer, VkCommandBuffer buffer, size_t image_index);
VkResult draw_frame(VulkanRenderer* renderer);

VkResult create_buffer(VulkanRenderer* renderer, BufferAllocationInfo* buffer_allocation_info);
VkResult create_vertex_buffers(VulkanRenderer* renderer);

VkResult resize(VulkanRenderer* renderer);

VkResult create_point_pipeline(VulkanRenderer* renderer);

size_t get_queue_family_index(QueueFamilies::Type type);

VkResult record_staging_command_buffer(VulkanRenderer* renderer, VkDeviceSize size);