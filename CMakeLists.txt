cmake_minimum_required(VERSION 4.0)
project(rstd LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(ext)
find_package(Vulkan REQUIRED)
add_executable(${PROJECT_NAME} main.cpp)

target_link_libraries(${PROJECT_NAME} glfw)
target_link_libraries(${PROJECT_NAME} Vulkan::Vulkan)

find_program(SLANGC_EXECUTABLE slangc HINTS $ENV{VULKAN_SDK}/bin REQUIRED)
message(STATUS "SLANGC PATH = ${SLANGC_EXECUTABLE}")

add_custom_command(
    OUTPUT ${CMAKE_SOURCE_DIR}/shaders/slang.spv
    COMMAND ${SLANGC_EXECUTABLE}
            ${CMAKE_SOURCE_DIR}/shaders/triangle.slang
            -target spirv
            -profile spirv_1_4
            -emit-spirv-directly
            -fvk-use-entrypoint-name
            -entry vertMain -entry fragMain
            -o ${CMAKE_SOURCE_DIR}/shaders/slang.spv
    DEPENDS ${CMAKE_SOURCE_DIR}/shaders/triangle.slang
    COMMENT "Compiling slang shader"
    VERBATIM
)

add_custom_target(shaders
    DEPENDS ${CMAKE_SOURCE_DIR}/shaders/slang.spv
)

add_dependencies(rstd shaders)