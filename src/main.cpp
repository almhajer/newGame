#include <iostream>
#include <signal.h>
#define GLFW_INCLUDE_VULKAN
#include "GLFW/glfw3.h"
#include <cstdlib>
#include <vector>
#define PANIC(ERROR, FORMAT, ...) \
if (ERROR) { \
fprintf(stderr, "%s -> %s -> %d -> Error(%i):\n\t" FORMAT "\n", \
__FILE__, __FUNCTION__, __LINE__, ERROR, ##__VA_ARGS__); \
raise(SIGSEGV); \
}

void glfwErorrCallback(int error_code, const char *error_message) {
    PANIC(error_code, "GLFW error: %s", error_message);
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
        state->windowWidth=monitor->width;
        state->windowHeight=monitor->height;
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

    PANIC(vkCreateInstance(&instanceCreateInfo, state->allocator, &state->instance) != VK_SUCCESS,
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
    PANIC(result != VK_SUCCESS, "Couldn't enumerate physical devices");
    PANIC(count == 0, "No physical devices found");

    std::cout << "Physical device count: " << count << std::endl;

    result = vkEnumeratePhysicalDevices(state->instance, &count, &state->physicalDevice);
    PANIC(result != VK_SUCCESS, "Couldn't enumerate physical devices");
}

void createSurface(State *state) {
    PANIC(glfwCreateWindowSurface(state->instance,state->window,state->allocator,&state->surface)!=VK_SUCCESS,
          "Couldn't Create Surface");
    std::cout << "Surface created: " << reinterpret_cast<uintptr_t>(state->surface) << std::endl;
}


void selectQueueFamily(State *state) {
    // الخطوة 1: الحصول على عدد عائلات الطوابير
    uint32_t count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(state->physicalDevice, &count, nullptr);
    std::vector<VkQueueFamilyProperties> queueFamilies(count);
    vkGetPhysicalDeviceQueueFamilyProperties(state->physicalDevice, &count, queueFamilies.data());
    PANIC(count<0, "Couldn't find any queue families");
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
    PANIC(result != VK_SUCCESS, "فشل في إنشاء الجهاز المنطقي");
    std::cout << "Device created: " << reinterpret_cast<uintptr_t>(state->device) << std::endl;
}


void getQueue(State *state) {
    vkGetDeviceQueue(state->device, state->queueFamilyIndex, 0, &state->queue);
    std::cout << "queue created: " << reinterpret_cast<uintptr_t>(state->queue) << std::endl;
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
