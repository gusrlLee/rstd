cmake_minimum_required(VERSION 3.30)
project(rstd)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(3rdparty)
find_package(Vulkan REQUIRED)

# set (CMAKE_RUNTIME_OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}/bin")
# set (CMAKE_LIBRARY_OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}/bin")

find_program(GLSL_VALIDATOR glslangValidator HINTS /usr/bin /usr/local/bin $ENV{VULKAN_SDK}/Bin/ $ENV{VULKAN_SDK}/Bin32/)


add_executable(${PROJECT_NAME} 
	types.hpp
	engine.hpp
	initializers.hpp

	main.cpp
	engine.cpp
)

target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_SOURCE_DIR}/3rdparty)

target_link_libraries(${PROJECT_NAME} Vulkan::Vulkan)
target_link_libraries(${PROJECT_NAME} vk-bootstrap::vk-bootstrap)
target_link_libraries(${PROJECT_NAME} glfw)
target_link_libraries(${PROJECT_NAME} glm::glm-header-only)
target_link_libraries(${PROJECT_NAME} GPUOpen::VulkanMemoryAllocator)
target_link_libraries(${PROJECT_NAME} fmt::fmt-header-only)