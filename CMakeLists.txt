cmake_minimum_required(VERSION 3.29)
project(game)

set(CMAKE_CXX_STANDARD 17)
# تعيين متغير لـ Vulkan SDK
set(VULKAN_SDK "/Users/abdulkafi/VulkanSDK/macOS/")
set(GLFW_SDK "/Users/abdulkafi/glfw/")

add_executable(game src/main.cpp)


target_include_directories(game PRIVATE "${VULKAN_SDK}/include")

target_link_libraries(game "${VULKAN_SDK}/lib/libvulkan.dylib")
target_link_libraries(game "/usr/local/lib/libMoltenVK.dylib")

target_include_directories(game PRIVATE "${GLFW_SDK}/include/")
target_link_libraries(game "${GLFW_SDK}/lib/libglfw.3.dylib")
set(CMAKE_CXX_STANDARD_REQUIRED True)