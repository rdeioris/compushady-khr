cmake_minimum_required(VERSION 3.17.2)

project(compushady-khr)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_CXX_STANDARD 17)

set(SPIRV_SKIP_EXECUTABLES ON)
set(ENABLE_GLSLANG_BINARIES OFF)

add_subdirectory(SPIRV-Headers)
add_subdirectory(SPIRV-Tools)
add_subdirectory(glslang)
add_subdirectory(SPIRV-Cross)

add_library(compushady_khr SHARED compushady_khr.cpp)
target_include_directories(compushady_khr PRIVATE glslang SPIRV-Cross)
target_link_libraries(compushady_khr PRIVATE glslang glslang-default-resource-limits SPIRV spirv-cross-hlsl spirv-cross-msl spirv-cross-glsl)
