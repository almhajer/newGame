cmake_minimum_required(VERSION 3.10)
project(game)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# البحث عن Vulkan
find_package(Vulkan REQUIRED)

# البحث عن GLFW
find_package(PkgConfig REQUIRED)
pkg_search_module(GLFW REQUIRED glfw3)

include_directories(${GLFW_INCLUDE_DIRS})
link_directories(${GLFW_LIBRARY_DIRS})

add_executable(game src/main.cpp)

target_link_libraries(game ${GLFW_LIBRARIES} Vulkan::Vulkan)
