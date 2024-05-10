#pragma once
#define VK_USE_PLATFORM_WIN32_KHR
#include <vulkan/vulkan.h>
#include <vulkan/vk_enum_string_helper.h>
#include "types.h"
#include "math.h"
#include "memory.h"
#include "time.h"
#include "texture.h"

struct Vertex {
    Vec2 position;
    Vec3 color;
    Vec2 texture_coord;
};

struct UniformBufferObject {
    Mat4 model;
    Mat4 view;
    Mat4 projection;
};

// static constexpr Vertex triforce_vertices[] = {
//     { { -0.5f, 0.0f }, { 1.0f, 0.0f, 0.0f } },
//     { { 0.0f, -1.0f }, { 0.0f, 1.0f, 0.0f } },
//     { { 0.5f, 0.0f }, { 0.0f, 0.0f, 1.0f } },

//     { { -1.0f, 1.0f }, { 1.0f, 0.0f, 0.0f } },
//     { { -0.5f, 0.0f }, { 0.0f, 1.0f, 0.0f } },
//     { { 0.0f, 1.0f }, { 0.0f, 0.0f, 1.0f } },

//     { { 0.0f, 1.0f }, { 1.0f, 0.0f, 0.0f } },
//     { { 0.5f, 0.0f }, { 0.0f, 1.0f, 0.0f } },
//     { { 1.0f, 1.0f }, { 0.0f, 0.0f, 1.0f } }
// };

// static constexpr Vertex indexed_triforce_vertices[] = {
//     { { -1.0f, 1.0f }, { 1.0f, 0.0f, 0.0f } },
//     { { -0.5f, 0.0f }, { 0.5f, 0.5f, 0.0f } },
//     { { 0.0f, 1.0f }, { 0.5f, 0.0f, 0.5f } },
//     { { 0.0f, -1.0f }, { 0.0f, 1.0f, 0.0f } },
//     { { 0.5f, 0.0f }, { 0.0f, 0.5f, 0.5f } },
//     { { 1.0f, 1.0f }, { 0.0f, 0.0f, 1.0f } }
// };

// static constexpr u16 vertex_indices[] = {
//     0, 1, 2, 1, 3, 4, 2, 4, 5
// };

static constexpr Vertex quad_vertices[] = {
    { { -0.5f, -0.5f }, { 0.0f, 1.0f, 0.0f }, { 1.0f, 0.0f } },
    { { 0.5f, -0.5f }, { 1.0f, 0.0f, 0.0f }, { 0.0f, 0.0f } },
    { { 0.5f, 0.5f }, { 1.0f, 0.0f, 0.0f }, { 0.0f, 1.0f } },
    { { -0.5f, 0.5f }, { 0.0f, 1.0f, 0.0f }, { 1.0f, 1.0f } }
};

static constexpr u16 quad_indices[] = {
    0, 1, 2, 0, 2, 3
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

    static constexpr size_t MAX_QUEUE_FAMILIES = static_cast<size_t>(QueueFamilies::Type::COUNT);

    QueueFamily families[MAX_QUEUE_FAMILIES];
    u32 populated_families;
};

struct Synchronization {
    static constexpr size_t MAX_SEMAPHORES = 8;
    static constexpr size_t MAX_FENCES = 8;

    VkSemaphore semaphores[MAX_SEMAPHORES];
    VkFence fences[MAX_FENCES];
};

struct CommandBuffers {
    static constexpr size_t MAX_BUFFERS = 4;

    enum class Graphics : size_t {
        DRAW_FRAME,
        TRANSITION_IMAGE_LAYOUT,
        COUNT
    };

    enum class Transfer : size_t {
        BUFFER_TO_BUFFER,
        BUFFER_TO_IMAGE,
        COUNT
    };

    VkCommandBuffer buffer[MAX_BUFFERS];
    Synchronization synchro[MAX_BUFFERS];
};

struct CommandPool {
    VkCommandPool pool = VK_NULL_HANDLE;
    CommandBuffers* buffers = nullptr;
};

struct CommandBufferAllocationInfo {
    QueueFamilies::Type pool_type;
    CommandBuffers::Graphics graphics_buffer_type;
    CommandBuffers::Transfer transfer_buffer_type;
    size_t buffer_count = 0;
    size_t semaphore_count = 0;
    size_t fence_count = 0;
};

struct Swapchain {
    static constexpr size_t MIN_IMAGES = 3;
    static constexpr size_t MAX_FRAMES_IN_FLIGHT = 2;

    struct SupportInfo {
        VkSurfaceCapabilitiesKHR capabilities = {};
        VkSurfaceFormatKHR* surface_formats = nullptr;
        VkPresentModeKHR* present_modes = nullptr;
    };

    struct Images {
        size_t count = 0;
        VkImage* images = nullptr;
        VkImageView* views = nullptr;
        VkFramebuffer* frame_buffers = nullptr;
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
    void* data = nullptr;
};

struct BufferAllocationInfo {
    Buffer* buffer = nullptr;
    VkBufferUsageFlags usage_flags = 0;
    VkMemoryPropertyFlags memory_properties = 0;
    VkDeviceSize size = 0;
    VkSharingMode sharing_mode;
    u32 queue_families_indices_count = 0;
    u32* queue_family_indices = nullptr;
    bool map_memory = false;
};

struct Texture {
    ImageData image_data;
    VkImage image;
    VkImageView image_view;
    VkDeviceMemory device_memory;
};

struct TextureAtlas {
    static constexpr size_t MAX_TEXTURES = 64;

    size_t next_texture_index = 0;
    Texture textures[MAX_TEXTURES];
    VkSampler sampler;
};

struct Transform {
    Mat4 translation = MAT4_IDENTITY;
    Mat4 rotation = MAT4_IDENTITY;
    Mat4 scale = MAT4_IDENTITY;
};

struct Sprite {
    Texture* texture = nullptr;
    Transform transform = {};
};

struct GraphicsPipeline {
    ShaderData shader_data = {};
    VkPipelineLayout layout = VK_NULL_HANDLE;
    VkPipeline pipeline = VK_NULL_HANDLE;
    VkDescriptorSetLayout descriptor_set_layout;
    VkDescriptorSet descriptor_sets[Swapchain::MAX_FRAMES_IN_FLIGHT];
    VkRenderPass render_pass = VK_NULL_HANDLE;
    VkClearValue clear_color = { .color = { 0.0f, 0.0f, 0.0f, 0.0f } };
    Buffer vertex_buffer = {};
    Buffer index_buffer = {};
    Buffer uniform_buffers[Swapchain::MAX_FRAMES_IN_FLIGHT];
    VkDescriptorPool descriptor_pool = VK_NULL_HANDLE;
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
};

struct VulkanRenderer {
    VkInstance instance = VK_NULL_HANDLE;
    Devices devices = {};
    VkSurfaceKHR surface = VK_NULL_HANDLE;
    Swapchain swapchain = {};
    GraphicsPipeline graphics_pipeline = {};
    GraphicsPipeline point_pipeline = {};
    QueueFamilies queue_families = {};
    CommandPool command_pools[QueueFamilies::MAX_QUEUE_FAMILIES];
    TextureAtlas texture_atlas = {};

    bool resizing = false;
    bool should_render = true;
    bool fixed_frame_mode = false;
    size_t frames_to_render = 10;

    MemoryArena* heap_data;
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
VkResult draw_frame(VulkanRenderer* renderer, Time::Duration delta_time);

VkResult create_buffer(VulkanRenderer* renderer, BufferAllocationInfo* buffer_allocation_info);
VkResult create_vertex_buffers(VulkanRenderer* renderer);

VkResult resize(VulkanRenderer* renderer);

VkResult create_point_pipeline(VulkanRenderer* renderer);

u32 get_queue_family_index(VulkanRenderer* renderer, QueueFamilies::Type type);

VkResult record_staging_command_buffer(VulkanRenderer* renderer, Buffer* buffer, VkDeviceSize size);

VkResult create_vertex_buffer(VulkanRenderer* renderer);
VkResult create_index_buffer(VulkanRenderer* renderer);
VkResult create_uniform_buffers(VulkanRenderer* renderer);

VkResult update_uniform_buffer(VulkanRenderer* renderer, size_t image_index, Time::Duration delta_time);

VkResult create_descriptor_pool(VulkanRenderer* renderer);
VkResult create_descriptor_sets(VulkanRenderer* renderer);

VkResult create_texture_atlas(VulkanRenderer* renderer);

VkResult load_texture(VulkanRenderer* renderer, const char* filename);

size_t find_memory_type_index(VulkanRenderer* renderer, u32 memory_type_bits, VkMemoryPropertyFlags memory_property_flags);

VkResult transition_image_layout(VulkanRenderer* renderer, VkImage image, VkFormat format, VkImageLayout old_layout, VkImageLayout new_layout);

Sprite* create_sprite(VulkanRenderer* renderer, size_t texture_id);
VkResult update_sprites(VulkanRenderer* renderer, Sprite* sprites[]);