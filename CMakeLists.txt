cmake_minimum_required(VERSION 3.30)
project(rstd LANGUAGES CXX CUDA)
 
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CUDA_STANDARD 20)

add_subdirectory(ext)

add_executable(${PROJECT_NAME} 
camera.h
	main.cu
	hitable.h
	hitable_list.h
	material.h
	ray.h
	sphere.h
	vec3.h
)

target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_SOURCE_DIR}/ext)
target_link_libraries(${PROJECT_NAME} tinyobjloader)