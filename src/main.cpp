#include <iostream>
#include <csignal>
#define GLFW_INCLUDE_VULKAN
#include "GLFW/glfw3.h"
#include <cstdlib>
#include <vector>

#define EXPECT(ERROR, FORMAT, ...) \
if (ERROR) { \
    fprintf(stderr, "%s -> %s -> %d -> Error(%i):\n\t" FORMAT "\n", \
    __FILE__, __FUNCTION__, __LINE__, ERROR, ##__VA_ARGS__); \
    raise(SIGSEGV); \
}

void glfwErorrCallback(int error_code, const char *error_message) {
    EXPECT(error_code, "GLFW error: %s", error_message);
}

void exitCallback() {
    glfwTerminate();
}

void setupErrorHandling() {
    glfwSetErrorCallback(glfwErorrCallback);
    atexit(exitCallback);
}

struct State {
    const char *windowTitle;
    const char *applicationName;
    const char *engineName;
    int windowWidth, windowHeight;
    bool windowResizable;
    bool windowFullscreen;
    uint32_t frameBufferWidth, frameBufferHeight;
    bool recreateSwapChain;

    GLFWwindow *window = nullptr;                   // تم تعديل هذا السطر
    GLFWmonitor *windowMonitor = nullptr;           // تم تعديل هذا السطر
    VkAllocationCallbacks *allocator = nullptr;     // تم تعديل هذا السطر
    const std::vector<const char *> validationLayers{
        "VK_LAYER_KHRONOS_validation"
    };

    const std::vector<const char *> enabledExtensionNames{
        VK_KHR_SWAPCHAIN_EXTENSION_NAME,
        "VK_KHR_portability_subset"
    };

    VkInstance instance = VK_NULL_HANDLE;           // تم تعديل هذا السطر
    uint32_t app_version;
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;  // تم تعديل هذا السطر
    VkSurfaceKHR surface = VK_NULL_HANDLE;             // تم تعديل هذا السطر
    uint32_t queueFamilyIndex;
    VkDevice device = VK_NULL_HANDLE;                  // تم تعديل هذا السطر
    VkQueue queue = VK_NULL_HANDLE;                    // تم تعديل هذا السطر
    uint32_t swapchainImageCount;
    VkSwapchainKHR swapchain = VK_NULL_HANDLE;         // تم تعديل هذا السطر
    std::vector<VkImage> swapchainImages;
    VkExtent2D swapchainExtent;
    std::vector<VkImageView> swapchainImageViews;
};

void glfwFramebufferSizeCallback(GLFWwindow *window, int width, int height) {
    State *state = static_cast<State*>(glfwGetWindowUserPointer(window));
    state->windowWidth = width;
    state->windowHeight = height;
    state->recreateSwapChain = true;
    printf("Window size: %dx%d\n", width, height);
}

void createWindow(State *state) {
    if (!glfwInit()) {
        throw std::runtime_error("Failed to initialize GLFW");
    }
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, state->windowResizable);
    if (state->windowFullscreen) {
        state->windowMonitor = glfwGetPrimaryMonitor();
        const GLFWvidmode *monitor = glfwGetVideoMode(state->windowMonitor);
        state->windowWidth = monitor->width;
        state->windowHeight = monitor->height;
        std::cout << "Monitor: " << monitor->width << "x" << monitor->height << std::endl;
        state->windowMonitor = nullptr;
    }
    state->window = glfwCreateWindow(state->windowWidth, state->windowHeight, state->windowTitle, state->windowMonitor,
                                     NULL);
    glfwSetWindowUserPointer(state->window, state);
    glfwSetFramebufferSizeCallback(state->window, glfwFramebufferSizeCallback);
    int width, height;
    glfwGetFramebufferSize(state->window, &width, &height);
    glfwFramebufferSizeCallback(state->window, width, height);
    if (!state->window) {
        throw std::runtime_error("Failed to create GLFW window");
    }
}

void createInstance(State *state) {
    if (!state->window) {
        throw std::runtime_error("GLFW window not initialized");
    }

    uint32_t requiredExtensionsCount;
    const char **requiredExtensions = glfwGetRequiredInstanceExtensions(&requiredExtensionsCount);

    std::vector<const char *> extensions(requiredExtensions, requiredExtensions + requiredExtensionsCount);

    extensions.push_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
    uint32_t layerCount;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
    std::vector<VkLayerProperties> availableLayers(layerCount);
    vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

    std::cout << "Available validation layers:" << std::endl;
    for (const auto &layer: availableLayers) {
        std::cout << "\t" << layer.layerName << std::endl;
    }

    VkApplicationInfo appInfo = {
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pApplicationName = state->applicationName,
        .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
        .pEngineName = state->engineName,
        .engineVersion = VK_MAKE_VERSION(1, 0, 0),
        .apiVersion = state->app_version,
    };

    VkInstanceCreateInfo instanceCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .pApplicationInfo = &appInfo,
        .enabledLayerCount = static_cast<uint32_t>(state->validationLayers.size()),
        .ppEnabledLayerNames = state->validationLayers.data(),
        .enabledExtensionCount = static_cast<uint32_t>(extensions.size()),
        .ppEnabledExtensionNames = extensions.data(),
    };
    instanceCreateInfo.flags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;

    EXPECT(vkCreateInstance(&instanceCreateInfo, state->allocator, &state->instance) != VK_SUCCESS,
           "Failed to create Vulkan instance");
    std::cout << "Instance created: " << reinterpret_cast<uintptr_t>(state->instance) << std::endl;
}

void logInfo() {
    uint32_t instanceApiVersion;
    vkEnumerateInstanceVersion(&instanceApiVersion);
    uint32_t apiVersionVariant = VK_API_VERSION_VARIANT(instanceApiVersion);
    uint32_t apiVersionMajor = VK_API_VERSION_MAJOR(instanceApiVersion);
    uint32_t apiVersionMinor = VK_API_VERSION_MINOR(instanceApiVersion);
    uint32_t apiVersionPatch = VK_API_VERSION_PATCH(instanceApiVersion);

    printf("VULKAN API %i.%i.%i.%i \n", apiVersionVariant, apiVersionMajor, apiVersionMinor, apiVersionPatch);
    printf("GLFW3 VERSION %s \n", glfwGetVersionString());
}

void selectPhysicalDevice(State *state) {
    uint32_t count;
    VkResult result = vkEnumeratePhysicalDevices(state->instance, &count, nullptr);
    EXPECT(result != VK_SUCCESS, "Couldn't enumerate physical devices");
    EXPECT(count == 0, "No physical devices found");

    std::cout << "Physical device count: " << count << std::endl;

    result = vkEnumeratePhysicalDevices(state->instance, &count, &state->physicalDevice);
    EXPECT(result != VK_SUCCESS, "Couldn't enumerate physical devices");
}

void createSurface(State *state) {
    EXPECT(glfwCreateWindowSurface(state->instance, state->window, state->allocator, &state->surface) != VK_SUCCESS,
           "Couldn't Create Surface");
    std::cout << "Surface created: " << reinterpret_cast<uintptr_t>(state->surface) << std::endl;
}

void selectQueueFamily(State *state) {
    uint32_t count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(state->physicalDevice, &count, nullptr);
    std::vector<VkQueueFamilyProperties> queueFamilies(count);
    vkGetPhysicalDeviceQueueFamilyProperties(state->physicalDevice, &count, queueFamilies.data());
    EXPECT(count < 0, "Couldn't find any queue families");
    std::cout << "Queue families Count: " << count << std::endl;

    for (uint32_t i = 0; i < count; i++) {
        const VkQueueFamilyProperties &queueFamily = queueFamilies[i];
        if ((queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) &&
            glfwGetPhysicalDevicePresentationSupport(state->instance, state->physicalDevice, i)) {
            state->queueFamilyIndex = i;
            break;
        }
    }
    std::cout << "Queue Family Index: " << state->queueFamilyIndex << std::endl;
}

void createDevice(State *state) {
    float priorities = 1.0f;
    VkPhysicalDeviceFeatures deviceFeatures{};

    VkDeviceQueueCreateInfo queueCreateInfo{};
    queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queueCreateInfo.pNext = nullptr;
    queueCreateInfo.flags = 0;
    queueCreateInfo.queueFamilyIndex = state->queueFamilyIndex;
    queueCreateInfo.queueCount = 1;
    queueCreateInfo.pQueuePriorities = &priorities;

    VkDeviceCreateInfo deviceCreateInfo{};
    deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    deviceCreateInfo.pNext = nullptr;
    deviceCreateInfo.flags = 0;
    deviceCreateInfo.queueCreateInfoCount = 1;
    deviceCreateInfo.pQueueCreateInfos = &queueCreateInfo;
    deviceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(state->enabledExtensionNames.size());
    deviceCreateInfo.ppEnabledExtensionNames = state->enabledExtensionNames.data();
    deviceCreateInfo.pEnabledFeatures = &deviceFeatures;
    deviceCreateInfo.enabledLayerCount = static_cast<uint32_t>(state->validationLayers.size());
    deviceCreateInfo.ppEnabledLayerNames = state->validationLayers.data();

    VkResult result = vkCreateDevice(state->physicalDevice, &deviceCreateInfo, state->allocator, &state->device);
    EXPECT(result != VK_SUCCESS, "فشل في إنشاء الجهاز المنطقي");
    std::cout << "Device created: " << reinterpret_cast<uintptr_t>(state->device) << std::endl;
}

void getQueue(State *state) {
    vkGetDeviceQueue(state->device, state->queueFamilyIndex, 0, &state->queue);
    std::cout << "Queue retrieved: " << reinterpret_cast<uintptr_t>(state->queue) << std::endl;
}

uint32_t clamp(uint32_t value, uint32_t min, uint32_t max) {
    if (value < min) {
        return min;
    } else if (value > max) {
        return max;
    }
    return value;
}

void createSwapchain(State *state) {
    VkSurfaceCapabilitiesKHR surfaceCapabilities;
    EXPECT(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(state->physicalDevice, state->surface, &surfaceCapabilities),
           "Failed to get Surface Capabilities");

    uint32_t minImageCount = surfaceCapabilities.minImageCount;
    uint32_t imageCount = minImageCount + 1;
    if (surfaceCapabilities.maxImageCount > 0 && imageCount > surfaceCapabilities.maxImageCount) {
        imageCount = surfaceCapabilities.maxImageCount;
    }

    uint32_t formatCount;
    EXPECT(vkGetPhysicalDeviceSurfaceFormatsKHR(state->physicalDevice, state->surface, &formatCount, nullptr),
           "Couldn't get surface format");
    std::vector<VkSurfaceFormatKHR> formats(formatCount);

    EXPECT(vkGetPhysicalDeviceSurfaceFormatsKHR(state->physicalDevice, state->surface, &formatCount, formats.data()),
           "Couldn't get surface format");

    uint32_t formatIndex = 0;
    for (uint32_t i = 0; i < formatCount; i++) {
        VkSurfaceFormatKHR format = formats[i];
        if (format.colorSpace == VK_COLORSPACE_SRGB_NONLINEAR_KHR && format.format == VK_FORMAT_B8G8R8A8_SRGB) {
            formatIndex = i;
            break;
        }
    }
    VkSurfaceFormatKHR format = formats[formatIndex];

    VkPresentModeKHR presentMode = VK_PRESENT_MODE_FIFO_KHR;
    uint32_t presentModeCount;

    VkResult result = vkGetPhysicalDeviceSurfacePresentModesKHR(state->physicalDevice, state->surface,
                                                                &presentModeCount, nullptr);
    EXPECT(result != VK_SUCCESS || presentModeCount == 0, "Failed to get surface present modes");

    std::vector<VkPresentModeKHR> presentModes(presentModeCount);

    result = vkGetPhysicalDeviceSurfacePresentModesKHR(state->physicalDevice, state->surface, &presentModeCount,
                                                       presentModes.data());
    EXPECT(result != VK_SUCCESS, "Failed to get surface present modes");
    uint32_t presentModeIndex = UINT32_MAX;

    for (uint32_t i = 0; i < presentModeCount; ++i) {   // تم تعديل هذا السطر
        VkPresentModeKHR mode = presentModes[i];
        if (mode == VK_PRESENT_MODE_MAILBOX_KHR) {
            presentModeIndex = i;
            break;
        }
    }

    if (presentModeIndex != UINT32_MAX) {
        presentMode = presentModes[presentModeIndex];
    }

    std::cout << "Selected Present Mode: " << presentMode << std::endl;

    // تدمير الـ Swapchain والـ Image Views القديمة إذا كانت موجودة
    if (state->swapchain != VK_NULL_HANDLE) {          // تم تعديل هذا السطر
        // تدمير الـ Image Views القديمة
        for (auto &imageView : state->swapchainImageViews) {   // تم تعديل هذا السطر
            if (imageView != VK_NULL_HANDLE) {                 // تم تعديل هذا السطر
                vkDestroyImageView(state->device, imageView, state->allocator);
                imageView = VK_NULL_HANDLE;                    // تم تعديل هذا السطر
            }
        }
        state->swapchainImageViews.clear();                    // تم تعديل هذا السطر

        // تدمير الـ Swapchain القديمة
        vkDestroySwapchainKHR(state->device, state->swapchain, state->allocator);  // تم تعديل هذا السطر
        state->swapchain = VK_NULL_HANDLE;                                         // تم تعديل هذا السطر
    }

    VkSwapchainCreateInfoKHR createInfo = {
        .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .surface = state->surface,
        .queueFamilyIndexCount = 1,
        .pQueueFamilyIndices = &state->queueFamilyIndex,
        .clipped = VK_TRUE,
        .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
        .imageArrayLayers = 1,
        .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        .oldSwapchain = VK_NULL_HANDLE,          // تم تعديل هذا السطر
        .preTransform = surfaceCapabilities.currentTransform,
        .imageExtent = surfaceCapabilities.currentExtent,
        .imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .imageFormat = formats[formatIndex].format,
        .imageColorSpace = formats[formatIndex].colorSpace,
        .presentMode = presentMode,
        .minImageCount = clamp(3, surfaceCapabilities.minImageCount,
                               surfaceCapabilities.maxImageCount ? surfaceCapabilities.maxImageCount : UINT32_MAX),
    };

    result = vkCreateSwapchainKHR(state->device, &createInfo, state->allocator, &state->swapchain);
    EXPECT(result != VK_SUCCESS, "Failed to create swapchain");

    // الحصول على الصور من الـ Swapchain
    result = vkGetSwapchainImagesKHR(state->device, state->swapchain, &state->swapchainImageCount, nullptr);
    EXPECT(result != VK_SUCCESS, "Failed to get swapchain images count");

    state->swapchainImages.resize(state->swapchainImageCount);
    result = vkGetSwapchainImagesKHR(state->device, state->swapchain, &state->swapchainImageCount, state->swapchainImages.data());
    EXPECT(result != VK_SUCCESS, "Failed to get swapchain images");

    std::cout << "Get Swapchain Images KHR Count :" << (state->swapchainImages.size()) << std::endl;

    state->swapchainImageViews.resize(state->swapchainImageCount);
    for (uint32_t i = 0; i < state->swapchainImageCount; i++) {
        VkImageViewCreateInfo viewInfo{
            .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .format = format.format,
            .image = state->swapchainImages[i],
            .components = {
                .r = VK_COMPONENT_SWIZZLE_IDENTITY,
                .g = VK_COMPONENT_SWIZZLE_IDENTITY,
                .b = VK_COMPONENT_SWIZZLE_IDENTITY,
                .a = VK_COMPONENT_SWIZZLE_IDENTITY,
            },
            .subresourceRange = {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .baseMipLevel = 0,
                .levelCount = 1,
                .baseArrayLayer = 0,
                .layerCount = 1,
            },
            .viewType = VK_IMAGE_VIEW_TYPE_2D,
        };

        result = vkCreateImageView(state->device, &viewInfo, state->allocator, &state->swapchainImageViews[i]);
        EXPECT(result != VK_SUCCESS, "Failed to create image view %u", i);
    }
}

void init(State *state) {
    logInfo();
    setupErrorHandling();
    createWindow(state);
    createInstance(state);
    selectPhysicalDevice(state);
    createSurface(state);
    selectQueueFamily(state);
    createDevice(state);
    getQueue(state);
    createSwapchain(state);          // تم إضافة هذا السطر للتأكد من إنشاء الـ Swapchain عند التهيئة
}

void loop(State *state) {
    while (!glfwWindowShouldClose(state->window)) {
        glfwPollEvents();
        if (state->recreateSwapChain) {
            state->recreateSwapChain = false;
            createSwapchain(state);
        }
    }
}

void cleanup(State *state) {
    // تدمير الـ Image Views
    for (auto &imageView : state->swapchainImageViews) {    // تم تعديل هذا السطر
        if (imageView != VK_NULL_HANDLE) {                  // تم تعديل هذا السطر
            vkDestroyImageView(state->device, imageView, state->allocator);
            imageView = VK_NULL_HANDLE;                     // تم تعديل هذا السطر
        }
    }
    state->swapchainImageViews.clear();                     // تم تعديل هذا السطر

    // تدمير الـ Swapchain
    if (state->swapchain != VK_NULL_HANDLE) {               // تم تعديل هذا السطر
        vkDestroySwapchainKHR(state->device, state->swapchain, state->allocator);
        state->swapchain = VK_NULL_HANDLE;                  // تم تعديل هذا السطر
    }

    // تدمير الـ Surface
    if (state->surface != VK_NULL_HANDLE) {                 // تم تعديل هذا السطر
        vkDestroySurfaceKHR(state->instance, state->surface, state->allocator);
        state->surface = VK_NULL_HANDLE;                    // تم تعديل هذا السطر
    }

    // تدمير الـ Device
    if (state->device != VK_NULL_HANDLE) {                  // تم تعديل هذا السطر
        vkDestroyDevice(state->device, state->allocator);
        state->device = VK_NULL_HANDLE;                     // تم تعديل هذا السطر
    }

    // تدمير الـ Instance
    if (state->instance != VK_NULL_HANDLE) {                // تم تعديل هذا السطر
        vkDestroyInstance(state->instance, state->allocator);
        state->instance = VK_NULL_HANDLE;                   // تم تعديل هذا السطر
    }

    // تدمير نافذة GLFW
    if (state->window != nullptr) {                         // تم تعديل هذا السطر
        glfwDestroyWindow(state->window);
        state->window = nullptr;                            // تم تعديل هذا السطر
    }

    // إنهاء GLFW
    glfwTerminate();
}

int main() {
    std::cout << "Hello, World!" << std::endl;
    State state = {
        .windowTitle = "بسم الله الرحمن الرحيم",
        .applicationName = "عبدالكافي",
        .engineName = "BK",
        .windowWidth = 800,
        .windowHeight = 600,
        .windowResizable = true,
        .windowFullscreen = false,
        .app_version = VK_API_VERSION_1_3,
    };
    init(&state);
    loop(&state);
    cleanup(&state);

    return EXIT_SUCCESS;
}
