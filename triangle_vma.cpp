#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define VMA_IMPLEMENTATION 1
#include "vk_mem_alloc.h"

#include <vector>
#include <cassert>
#include <cstring>
#include <iostream>

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
    GLFWwindow* window = glfwCreateWindow(800, 600, "VMA Triangle", nullptr, nullptr);

    // --- Instance ---
    uint32_t extCount = 0;
    const char** exts = glfwGetRequiredInstanceExtensions(&extCount);

    VkApplicationInfo app{};
    app.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    app.apiVersion = VK_API_VERSION_1_1;

    VkInstanceCreateInfo ici{};
    ici.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    ici.pApplicationInfo = &app;
    ici.enabledExtensionCount = extCount;
    ici.ppEnabledExtensionNames = exts;

    VkInstance instance;
    vkCreateInstance(&ici, nullptr, &instance);

    // --- Surface ---
    VkSurfaceKHR surface;
    glfwCreateWindowSurface(instance, window, nullptr, &surface);

    // --- Physical device ---
    uint32_t gpuCount = 0;
    vkEnumeratePhysicalDevices(instance, &gpuCount, nullptr);
    std::vector<VkPhysicalDevice> gpus(gpuCount);
    vkEnumeratePhysicalDevices(instance, &gpuCount, gpus.data());
    VkPhysicalDevice physicalDevice = gpus[0];

    VkPhysicalDeviceProperties props{};
    vkGetPhysicalDeviceProperties(physicalDevice, &props);

    // --- Queue ---
    uint32_t queueFamily = 0;
    uint32_t qCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &qCount, nullptr);
    std::vector<VkQueueFamilyProperties> qprops(qCount);
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &qCount, qprops.data());

    for (uint32_t i = 0; i < qCount; i++) {
        VkBool32 present = VK_FALSE;
        vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, i, surface, &present);
        if ((qprops[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) && present) {
            queueFamily = i;
            break;
        }
    }

    float prio = 1.0f;
    VkDeviceQueueCreateInfo qci{};
    qci.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    qci.queueFamilyIndex = queueFamily;
    qci.queueCount = 1;
    qci.pQueuePriorities = &prio;

    VkDeviceCreateInfo dci{};
    dci.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    dci.queueCreateInfoCount = 1;
    dci.pQueueCreateInfos = &qci;

    VkDevice device;
    vkCreateDevice(physicalDevice, &dci, nullptr, &device);

    VkQueue queue;
    vkGetDeviceQueue(device, queueFamily, 0, &queue);

    // --- Swapchain (minimal) ---
    VkSurfaceCapabilitiesKHR caps{};
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &caps);

    VkSwapchainCreateInfoKHR sci{};
    sci.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
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
    sci.clipped = VK_TRUE;

    VkSwapchainKHR swapchain;
    vkCreateSwapchainKHR(device, &sci, nullptr, &swapchain);

    // --- VMA allocator ---
    VmaVulkanFunctions funcs{};
    funcs.vkGetPhysicalDeviceProperties = vkGetPhysicalDeviceProperties;
    funcs.vkGetPhysicalDeviceMemoryProperties = vkGetPhysicalDeviceMemoryProperties;
    funcs.vkAllocateMemory = vkAllocateMemory;
    funcs.vkFreeMemory = vkFreeMemory;
    funcs.vkMapMemory = vkMapMemory;
    funcs.vkUnmapMemory = vkUnmapMemory;
    funcs.vkFlushMappedMemoryRanges = vkFlushMappedMemoryRanges;
    funcs.vkBindBufferMemory = vkBindBufferMemory;
    funcs.vkGetBufferMemoryRequirements = vkGetBufferMemoryRequirements;
    funcs.vkCreateBuffer = vkCreateBuffer;
    funcs.vkDestroyBuffer = vkDestroyBuffer;

    VmaAllocatorCreateInfo aci{};
    aci.instance = instance;
    aci.physicalDevice = physicalDevice;
    aci.device = device;
    aci.vulkanApiVersion = VK_API_VERSION_1_1;
    aci.pVulkanFunctions = &funcs;

    VmaAllocator allocator;
    vmaCreateAllocator(&aci, &allocator);

    // --- Vertex buffer ---
    VkBufferCreateInfo bci{};
    bci.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bci.size = sizeof(vertices);
    bci.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;

    VmaAllocationCreateInfo alloc{};
    alloc.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;
    alloc.flags =
        VMA_ALLOCATION_CREATE_MAPPED_BIT |
        VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;

    VkBuffer vertexBuffer;
    VmaAllocation allocation;
    VmaAllocationInfo ainfo{};

    vmaCreateBuffer(allocator, &bci, &alloc,
                    &vertexBuffer, &allocation, &ainfo);

    std::memcpy(ainfo.pMappedData, vertices, sizeof(vertices));

    // --- Correct flush for Intel macOS ---
    VkDeviceSize atom = props.limits.nonCoherentAtomSize;
    VkDeviceSize size = sizeof(vertices);
    VkDeviceSize flushSize =
        (size + atom - 1) & ~(atom - 1);

    vmaFlushAllocation(allocator, allocation, 0, flushSize);

    // --- Main loop (just clear) ---
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
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
}
