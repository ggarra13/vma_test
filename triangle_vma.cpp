#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define VMA_IMPLEMENTATION
#include "vk_mem_alloc.h"

#include <vector>
#include <cstring>
#include <iostream>

// --- Shaders ---
static const uint32_t vert_spirv[] = {
    0x07230203,0x00010000,0x0008000A,0x0000001E,0x00000000,0x00020011,0x00000001,0x0006000B,
    0x00000001,0x4C534C47,0x64726F57,0x00000000,0x0003000E,0x00000000,0x00000001,0x0007000F,
    0x00000004,0x00000004,0x6E69616D,0x00000000,0x0000000B,0x0000000F,0x00030010,0x00000004,
    0x00000007,0x00030003,0x00000002,0x00000190,0x00040005,0x00000004,0x6E69616D,0x00000000,
    0x00040005,0x0000000B,0x6F6C6F63,0x00000072,0x00040005,0x0000000F,0x736F70A3,0x00000000,
    0x00030005,0x00000011,0x00000000,0x00050006,0x00000011,0x00000000,0x6F6C6F63,0x00000072,
    0x00040006,0x00000011,0x00000001,0x70797475,0x00000065,0x00030005,0x00000013,0x00000000,
    0x00040047,0x0000000B,0x0000001E,0x00000000,0x00040047,0x0000000F,0x0000001E,0x00000000,
    0x00040047,0x00000017,0x0000000B,0x00000000,0x00020013,0x00000002,0x00030021,0x00000003,
    0x00000002,0x00030016,0x00000006,0x00000020,0x00040017,0x00000007,0x00000006,0x00000003,
    0x00040020,0x00000008,0x00000007,0x00000007,0x00040017,0x00000009,0x00000006,0x00000004,
    0x00040020,0x0000000A,0x00000001,0x00000009,0x0004003B,0x0000000A,0x0000000B,0x00000001,
    0x00040020,0x0000000E,0x00000003,0x00000009,0x0004003B,0x0000000E,0x0000000F,0x00000003,
    0x00040015,0x00000016,0x00000020,0x00000000,0x00040020,0x00000017,0x00000001,0x00000016,
    0x0004002B,0x00000016,0x00000018,0x00000000,0x0004002B,0x00000016,0x00000019,0x00000001,
    0x00050036,0x00000002,0x00000004,0x000200F8,0x00000005,0x0004003D,0x00000009,0x00000010,
    0x0000000F,0x0004003D,0x00000009,0x00000012,0x0000000B,0x00050041,0x00000017,0x0000001A,
    0x0000000F,0x00000018,0x0004003D,0x00000016,0x0000001B,0x0000001A,0x00050050,0x00000007,
    0x0000001C,0x0000001B,0x00000019,0x00050041,0x00000017,0x0000001D,0x0000000F,0x00000019,
    0x0004003D,0x00000016,0x0000001E,0x0000001D,0x00050081,0x00000016,0x0000001F,0x0000001E,
    0x00000019,0x0004007F,0x00000016,0x00000020,0x0000001F,0x00050051,0x00000006,0x00000021,
    0x00000012,0x00000000,0x00050051,0x00000006,0x00000022,0x00000012,0x00000001,0x00050051,
    0x00000006,0x00000023,0x00000012,0x00000002,0x00070050,0x00000007,0x00000024,0x00000021,
    0x00000022,0x00000023,0x00000020,0x00050081,0x00000007,0x00000014,0x0000001C,0x00000024,
    0x0003003E,0x00000011,0x00000014,0x000100FD,0x00010038
};

static const uint32_t frag_spirv[] = {
    0x07230203,0x00010000,0x0008000A,0x00000012,0x00000000,0x00020011,0x00000001,0x0006000B,
    0x00000001,0x4C534C47,0x64726F57,0x00000000,0x0003000E,0x00000000,0x00000001,0x0006000F,
    0x00000005,0x00000004,0x6E69616D,0x00000000,0x00000009,0x0000000B,0x00030010,0x00000004,
    0x00000007,0x00030003,0x00000002,0x00000190,0x00040005,0x00000004,0x6E69616D,0x00000000,
    0x00040005,0x00000009,0x67617266,0x00000043,0x00040005,0x0000000B,0x6F6C6F63,0x00000072,
    0x00030005,0x0000000D,0x00000000,0x00040047,0x00000009,0x0000001E,0x00000000,0x00040047,
    0x0000000B,0x0000001E,0x00000000,0x00020013,0x00000002,0x00030021,0x00000003,0x00000002,
    0x00030016,0x00000006,0x00000020,0x00040017,0x00000007,0x00000006,0x00000004,0x00040020,
    0x00000008,0x00000003,0x00000007,0x0004003B,0x00000008,0x00000009,0x00000003,0x00040020,
    0x0000000A,0x00000001,0x00000007,0x0004003B,0x0000000A,0x0000000B,0x00000001,0x00050036,
    0x00000002,0x00000004,0x000200F8,0x00000005,0x0004003D,0x00000007,0x0000000C,0x0000000B,
    0x0003003E,0x00000009,0x0000000C,0x000100FD,0x00010038
};

struct Vertex {
    float pos[2];
    float color[3];
};

static const Vertex vertices[] = {
    {{ 0.0f, -0.5f }, {1.f, 0.f, 0.f}},
    {{ 0.5f,  0.5f }, {0.f, 1.f, 0.f}},
    {{-0.5f,  0.5f }, {0.f, 0.f, 1.f}},
};

int main() {
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    GLFWwindow* window = glfwCreateWindow(800, 600, "VMA Triangle Demo", nullptr, nullptr);

    // 1. Instance
    uint32_t extCount = 0;
    const char** exts = glfwGetRequiredInstanceExtensions(&extCount);
    VkInstanceCreateInfo instanceCI{ VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO };
    instanceCI.enabledExtensionCount = extCount;
    instanceCI.ppEnabledExtensionNames = exts;
    VkInstance instance;
    vkCreateInstance(&instanceCI, nullptr, &instance);

    // 2. Surface & Physical Device
    VkSurfaceKHR surface;
    glfwCreateWindowSurface(instance, window, nullptr, &surface);
    uint32_t gpuCount = 0;
    vkEnumeratePhysicalDevices(instance, &gpuCount, nullptr);
    std::vector<VkPhysicalDevice> gpus(gpuCount);
    vkEnumeratePhysicalDevices(instance, &gpuCount, gpus.data());
    VkPhysicalDevice physicalDevice = gpus[0];

    // 3. Device & Queue
    uint32_t queueFamily = 0; // Simple selection for brevity
    float priority = 1.0f;
    VkDeviceQueueCreateInfo qci{ VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO };
    qci.queueFamilyIndex = queueFamily;
    qci.queueCount = 1;
    qci.pQueuePriorities = &priority;

    const char* deviceExts[] = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };
    VkDeviceCreateInfo dci{ VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO };
    dci.queueCreateInfoCount = 1;
    dci.pQueueCreateInfos = &qci;
    dci.enabledExtensionCount = 1;
    dci.ppEnabledExtensionNames = deviceExts;
    VkDevice device;
    vkCreateDevice(physicalDevice, &dci, nullptr, &device);
    VkQueue queue;
    vkGetDeviceQueue(device, queueFamily, 0, &queue);

    // 4. VMA Allocator
    VmaVulkanFunctions vmaFuncs = {};
    vmaFuncs.vkGetInstanceProcAddr = &vkGetInstanceProcAddr;
    vmaFuncs.vkGetDeviceProcAddr = &vkGetDeviceProcAddr;

    VmaAllocatorCreateInfo allocatorCI = {};
    allocatorCI.physicalDevice = physicalDevice;
    allocatorCI.device = device;
    allocatorCI.instance = instance;
    allocatorCI.vulkanApiVersion = VK_API_VERSION_1_0;
    allocatorCI.pVulkanFunctions = &vmaFuncs;

    VmaAllocator allocator;
    vmaCreateAllocator(&allocatorCI, &allocator);

    // 5. Create Vertex Buffer with VMA
    VkBufferCreateInfo bufferInfo = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
    bufferInfo.size = sizeof(vertices);
    bufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;

    VmaAllocationCreateInfo allocInfo = {};
    allocInfo.usage = VMA_MEMORY_USAGE_AUTO;
    allocInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT;

    VkBuffer vertexBuffer;
    VmaAllocation allocation;
    VmaAllocationInfo resultAllocInfo;
    vmaCreateBuffer(allocator, &bufferInfo, &allocInfo, &vertexBuffer, &allocation, &resultAllocInfo);

    std::memcpy(resultAllocInfo.pMappedData, vertices, sizeof(vertices));

    // 6. Minimal Swapchain Setup
    VkSurfaceCapabilitiesKHR caps;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &caps);
    VkSwapchainCreateInfoKHR sci{ VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR };
    sci.surface = surface;
    sci.minImageCount = 2;
    sci.imageFormat = VK_FORMAT_B8G8R8A8_UNORM;
    sci.imageColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
    sci.imageExtent = caps.currentExtent;
    sci.imageArrayLayers = 1;
    sci.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    sci.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    sci.preTransform = caps.currentTransform;
    sci.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    sci.presentMode = VK_PRESENT_MODE_FIFO_KHR;

    VkSwapchainKHR swapchain;
    vkCreateSwapchainKHR(device, &sci, nullptr, &swapchain);

    // Cleanup logic
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        // (Rendering loop would go here)
    }

    vkDeviceWaitIdle(device);
    vmaDestroyBuffer(allocator, vertexBuffer, allocation);
    vmaDestroyAllocator(allocator);
    vkDestroySwapchainKHR(device, swapchain, nullptr);
    vkDestroyDevice(device, nullptr);
    vkDestroySurfaceKHR(instance, surface, nullptr);
    vkDestroyInstance(instance, nullptr);
    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}
