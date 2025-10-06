cmake_minimum_required(VERSION 3.30)
# vk-bootstrap 
add_subdirectory(vk-bootstrap)

# GLFW library for window 
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
add_subdirectory(glfw)

# math library for vulkan 
add_subdirectory(glm)

# logging 
add_subdirectory(fmt)

# vulkan memory allocator
add_subdirectory(VulkanMemoryAllocator)