cmake_minimum_required(VERSION 3.10)
project(game)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# تعيين مسارات Vulkan و GLFW
set(VULKAN_SDK "/Users/mac/VulkanSDK/macOS/")
set(GLFW_SDK "/Users/mac/glfw/")

# إضافة الملفات التنفيذية
add_executable(game src/main.cpp)

# تضمين مسارات الـ include بعد إنشاء الهدف
target_include_directories(game PRIVATE
        "${VULKAN_SDK}/include"
        "${GLFW_SDK}/include"
)

# ربط مكتبات Vulkan و GLFW بشكل مباشر
target_link_libraries(game
        "${VULKAN_SDK}/lib/libvulkan.dylib"
        "${VULKAN_SDK}/lib/libMoltenVK.dylib"
        "${GLFW_SDK}/lib/libglfw.3.dylib"

)
