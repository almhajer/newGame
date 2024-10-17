#include <iostream>
#include <signal.h>
#define GLFW_INCLUDE_VULKAN
#include "GLFW/glfw3.h"

#include<cstdlib>


#define PANIC(ERROR, FORMAT, ...) \
if (ERROR) { \
fprintf(stderr, "%s -> %s -> %d -> Error(%d):\n\t" FORMAT "\n", \
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
    const char *window_title;
    int width, height;
    bool resizable;
    bool window_fullscreen;
    //الناافذة
    GLFWwindow *window;
    GLFWmonitor *window_monitor;
    VkAllocationCallbacks *allocator;
    VkInstance instance;
    uint32_t app_version;
};

void createWindow(State *state) {
    if (!glfwInit()) {
        std::runtime_error("Failed to initialize GLFW");
    }
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    if (state->window_fullscreen) {
        state->window_monitor = glfwGetPrimaryMonitor();
    }
    state->window = glfwCreateWindow(state->width, state->height, state->window_title, state->window_monitor,NULL);
}

void createInstance(State *state) {
    if (!state->window) {
        std::runtime_error("Failed to create GLFW window");
    }
    uint32_t requiredExtensionsCount;
    const char **RequiredExtensions = glfwGetRequiredInstanceExtensions(&requiredExtensionsCount);

    VkApplicationInfo appInfo = {
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pApplicationName = state->window_title,
        .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
        .pEngineName = state->window_title,
        .engineVersion = VK_MAKE_VERSION(1, 0, 0),
        .apiVersion = state->app_version,

    };
    VkInstanceCreateInfo instanceCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .pApplicationInfo = &appInfo,
        .enabledExtensionCount = requiredExtensionsCount,
        .ppEnabledExtensionNames = RequiredExtensions
    };

    PANIC(!vkCreateInstance(&instanceCreateInfo, state->allocator, &state->instance), "Collude Make Instance");
    std::cout << "Instance created: " << reinterpret_cast<uintptr_t>(state->instance) << std::endl;
}

void logInfo() {
    uint32_t instanceApiVersion;
    vkEnumerateInstanceVersion(&instanceApiVersion);
    uint32_t apiVersionVariant=VK_API_VERSION_VARIANT(instanceApiVersion);
    uint32_t apiVersionMajor=VK_API_VERSION_MAJOR(instanceApiVersion);
    uint32_t apiVersionMinor=VK_API_VERSION_MINOR(instanceApiVersion);
    uint32_t apiVersionPatch=VK_API_VERSION_PATCH(instanceApiVersion);

    printf("VULKAN API %i.%i.%i.%i \n" ,apiVersionVariant,apiVersionMajor,apiVersionMinor,apiVersionPatch);
    printf("GLFW3 VERSION %s \n" ,glfwGetVersionString());
}

void init(State *state) {
    logInfo();
    setupErrorHandling();
    createWindow(state);
    createInstance(state);
}

void loop(State *state) {
    while (!glfwWindowShouldClose(state->window)) {
        glfwPollEvents();
    }
}

void cleanup(State *state) {
    glfwDestroyWindow(state->window);
    vkDestroyInstance(state->instance, state->allocator);
    state->window = nullptr;
}

int main() {
    std::cout << "Hello, World!" << std::endl;
    State state = {
        .window_title = "Hello, World!",
        .width = 800,
        .height = 800,
        .resizable = false,
        .window_fullscreen = false,
        .app_version = VK_API_VERSION_1_3
    };
    init(&state);
    loop(&state);
    cleanup(&state);

    return EXIT_SUCCESS;
}
