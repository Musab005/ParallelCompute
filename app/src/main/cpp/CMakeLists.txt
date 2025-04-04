cmake_minimum_required(VERSION 3.10)

project("parallelcompute")

# Find Vulkan (required)
find_package(Vulkan REQUIRED)

add_library(
        # Name of your library
        nativevulkan

        # Type of library
        SHARED

        # Your C++ source files
        MyVulkanCompute.cpp
        # add more .cpp files if needed
)

target_link_libraries(
        nativevulkan
        Vulkan::Vulkan
        android
        log
)
