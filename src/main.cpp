#include <iostream>
#include <csignal>
#define GLFW_INCLUDE_VULKAN
#include "GLFW/glfw3.h"
#include <cstdlib>
#include <vector>
#define PANIC_IF(ERROR, FORMAT, ...) \
if (ERROR) { \
fprintf(stderr, "%s -> %s -> %d -> Error(%i):\n\t" FORMAT "\n", \
__FILE__, __FUNCTION__, __LINE__, ERROR, ##__VA_ARGS__); \
raise(SIGSEGV); \
}

void glfwErorrCallback(int error_code, const char *error_message) {
    PANIC_IF(error_code, "GLFW error: %s", error_message);
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
    GLFWwindow *window;
    GLFWmonitor *windowMonitor;
    VkAllocationCallbacks *allocator;
    const std::vector<const char *> validationLayers{
        "VK_LAYER_KHRONOS_validation"
    };
    // تعريف مصفوفة الامتدادات المطلوبة

    const std::vector<const char *> enabledExtensionNames{
        VK_KHR_SWAPCHAIN_EXTENSION_NAME,
        "VK_KHR_portability_subset"
    };

    VkInstance instance;
    uint32_t app_version;
    VkPhysicalDevice physicalDevice;
    VkSurfaceKHR surface;
    uint32_t queueFamilyIndex;
    VkDevice device;
    VkQueue queue;
    VkSwapchainKHR swapchain;
    VkExtent2D swapchainExtent;
};

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
        state->windowMonitor = nullptr; // هذا يضمن إنشاء نافذة في وضع النوافذ العادية
    }
    state->window = glfwCreateWindow(state->windowWidth, state->windowHeight, state->windowTitle, state->windowMonitor,
                                     NULL);

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

    // نسخ الامتدادات إلى std::vector لتسهيل التعديل عليها
    std::vector<const char *> extensions(requiredExtensions, requiredExtensions + requiredExtensionsCount);

    // إضافة الامتداد VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME
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

    PANIC_IF(vkCreateInstance(&instanceCreateInfo, state->allocator, &state->instance) != VK_SUCCESS,
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
    PANIC_IF(result != VK_SUCCESS, "Couldn't enumerate physical devices");
    PANIC_IF(count == 0, "No physical devices found");

    std::cout << "Physical device count: " << count << std::endl;

    result = vkEnumeratePhysicalDevices(state->instance, &count, &state->physicalDevice);
    PANIC_IF(result != VK_SUCCESS, "Couldn't enumerate physical devices");
}

void createSurface(State *state) {
    PANIC_IF(glfwCreateWindowSurface(state->instance,state->window,state->allocator,&state->surface)!=VK_SUCCESS,
             "Couldn't Create Surface");
    std::cout << "Surface created: " << reinterpret_cast<uintptr_t>(state->surface) << std::endl;
}


void selectQueueFamily(State *state) {
    // الخطوة 1: الحصول على عدد عائلات الطوابير
    uint32_t count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(state->physicalDevice, &count, nullptr);
    std::vector<VkQueueFamilyProperties> queueFamilies(count);
    vkGetPhysicalDeviceQueueFamilyProperties(state->physicalDevice, &count, queueFamilies.data());
    PANIC_IF(count<0, "Couldn't find any queue families");
    std::cout << "queue families Count: " << count << std::endl;

    // الخطوة 3: البحث عن عائلة الطابور المناسبة
    for (uint32_t i = 0; i < count; i++) {
        const VkQueueFamilyProperties &queueFamily = queueFamilies[i];
        // التحقق من دعم الرسومات والعرض التقديمي
        if ((queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) &&
            glfwGetPhysicalDevicePresentationSupport(state->instance, state->physicalDevice, i)) {
            state->queueFamilyIndex = i;
            break;
        }
    }
    std::cout << "queue Family numbers: " << reinterpret_cast<uint32_t>(state->queueFamilyIndex) << std::endl;
}


void createDevice(State *state) {
    float priorities = 1.0f;
    VkPhysicalDeviceFeatures deviceFeatures{};

    // إعداد معلومات إنشاء قائمة انتظار الجهاز
    VkDeviceQueueCreateInfo queueCreateInfo{};
    queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queueCreateInfo.pNext = nullptr;
    queueCreateInfo.flags = 0;
    queueCreateInfo.queueFamilyIndex = state->queueFamilyIndex;
    queueCreateInfo.queueCount = 1;
    queueCreateInfo.pQueuePriorities = &priorities;

    // إعداد معلومات إنشاء الجهاز المنطقي
    VkDeviceCreateInfo deviceCreateInfo{};
    deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    deviceCreateInfo.pNext = nullptr;
    deviceCreateInfo.flags = 0;
    deviceCreateInfo.queueCreateInfoCount = 1;
    deviceCreateInfo.pQueueCreateInfos = &queueCreateInfo;
    deviceCreateInfo.enabledExtensionCount = state->enabledExtensionNames.size();
    deviceCreateInfo.ppEnabledExtensionNames = state->enabledExtensionNames.data();
    deviceCreateInfo.pEnabledFeatures = &deviceFeatures;
    deviceCreateInfo.enabledLayerCount = state->validationLayers.size();
    deviceCreateInfo.ppEnabledLayerNames = state->validationLayers.data();

    // إنشاء الجهاز المنطقي
    VkResult result = vkCreateDevice(state->physicalDevice, &deviceCreateInfo, state->allocator, &state->device);
    PANIC_IF(result != VK_SUCCESS, "فشل في إنشاء الجهاز المنطقي");
    std::cout << "Device created: " << reinterpret_cast<uintptr_t>(state->device) << std::endl;
}


void getQueue(State *state) {
    vkGetDeviceQueue(state->device, state->queueFamilyIndex, 0, &state->queue);
    std::cout << "queue created: " << reinterpret_cast<uintptr_t>(state->queue) << std::endl;
}

void createSwapchain(State *state) {
    VkSurfaceCapabilitiesKHR surfaceCapabilities;
    PANIC_IF(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(state->physicalDevice, state->surface,&surfaceCapabilities),
             "Failed to get Surface Capabilities");
    uint32_t minImageCount = surfaceCapabilities.minImageCount;
    uint32_t imageCount = minImageCount + 1;
    if (surfaceCapabilities.maxImageCount > 0 && imageCount > surfaceCapabilities.maxImageCount) {
        imageCount = surfaceCapabilities.maxImageCount;
    }

    uint32_t formatCount;
    PANIC_IF(vkGetPhysicalDeviceSurfaceFormatsKHR(state->physicalDevice, state->surface, &formatCount, nullptr),
             "Couldn't get surafce format");
    std::vector<VkSurfaceFormatKHR> formats(formatCount);

    PANIC_IF(vkGetPhysicalDeviceSurfaceFormatsKHR(state->physicalDevice, state->surface, &formatCount, formats.data()),
             "Couldn't get surafce format");


    uint32_t formatIndex = 0;
    for (uint32_t i = 0; i < formatCount; i++) {
        VkSurfaceFormatKHR format = formats[i];
        if (format.colorSpace == VK_COLORSPACE_SRGB_NONLINEAR_KHR && format.format == VK_FORMAT_B8G8R8A8_SRGB) {
            formatIndex = i;
            break;
        }
    }
    VkSurfaceFormatKHR format = formats[formatIndex];
    // تعيين الوضع الافتراضي إلى VK_PRESENT_MODE_FIFO_KHR (مضمون التوفر)
    VkPresentModeKHR presentMode = VK_PRESENT_MODE_FIFO_KHR;
    uint32_t presentModeCount = 0;

    // الاستدعاء الأول للحصول على عدد أوضاع العرض التقديمي
    VkResult result = vkGetPhysicalDeviceSurfacePresentModesKHR(state->physicalDevice, state->surface,
                                                                &presentModeCount, nullptr);
    PANIC_IF(result != VK_SUCCESS || presentModeCount == 0, "Failed to get surface present modes");

    // تخصيص المتجه بأوضاع العرض التقديمي
    std::vector<VkPresentModeKHR> presentModes(presentModeCount);

    // الاستدعاء الثاني للحصول على الأوضاع الفعلية
    result = vkGetPhysicalDeviceSurfacePresentModesKHR(state->physicalDevice, state->surface, &presentModeCount,
                                                       presentModes.data());
    PANIC_IF(result != VK_SUCCESS, "Failed to get surface present modes");

    // البحث عن أفضل وضع عرض تقديمي متاح
    for (const auto &availablePresentMode: presentModes) {
        if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
            presentMode = VK_PRESENT_MODE_MAILBOX_KHR;
            break;
        } else if (availablePresentMode == VK_PRESENT_MODE_IMMEDIATE_KHR && presentMode !=
                   VK_PRESENT_MODE_MAILBOX_KHR) {
            presentMode = VK_PRESENT_MODE_IMMEDIATE_KHR;
        }
    }

    std::cout << "Selected Present Mode: " << presentMode << std::endl;

    VkSwapchainKHR swapchain;

    VkSwapchainCreateInfoKHR createInfo = {
        .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .surface = state->surface,
        .queueFamilyIndexCount = 1,
        .pQueueFamilyIndices = &state->queueFamilyIndex,
        .clipped = VK_TRUE,
        .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
        .imageArrayLayers = surfaceCapabilities.maxImageArrayLayers,
        .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        .oldSwapchain = state->swapchain,
        .preTransform = surfaceCapabilities.currentTransform,
        .imageExtent = surfaceCapabilities.currentExtent,
        .imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .imageFormat = formats[formatIndex].format,
        .imageColorSpace = formats[formatIndex].colorSpace,
        .presentMode = presentMode,
        .minImageCount = imageCount,

    };
    vkCreateSwapchainKHR(state->device, &createInfo, state->allocator, &state->swapchain);
    vkDestroySwapchainKHR(state->device, state->swapchain, state->allocator);
    std::cout << "Swapchain created: " << reinterpret_cast<uintptr_t>(state->swapchain) << std::endl;
    state->swapchain = swapchain;
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
    createSwapchain(state);
}

void loop(State *state) {
    while (!glfwWindowShouldClose(state->window)) {
        glfwPollEvents();
    }
}

void cleanup(State *state) {
    vkDestroyDevice(state->device, state->allocator);
    vkDestroySurfaceKHR(state->instance, state->surface, state->allocator);
    vkDestroyInstance(state->instance, state->allocator);
    glfwDestroyWindow(state->window);
    state->window = nullptr;
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
