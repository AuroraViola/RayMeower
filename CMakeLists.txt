cmake_minimum_required(VERSION 4.0)
project(RayMeower C)

set(CMAKE_C_STANDARD 23)

include_directories(third-party/Nuklear third-party/Nuklear/demo/sdl3_renderer)

# Compile shaders
set( ${CMAKE_CURRENT_SOURCE_DIR}/shaders)

set(SHADER_DIR ${CMAKE_CURRENT_SOURCE_DIR}/shaders)
file(GLOB SHADERS ${SHADER_DIR}/*.vert ${SHADER_DIR}/*.frag ${SHADER_DIR}/*.comp ${SHADER_DIR}/*.geom ${SHADER_DIR}/*.tesc ${SHADER_DIR}/*.tese ${SHADER_DIR}/*.mesh ${SHADER_DIR}/*.task ${SHADER_DIR}/*.rgen ${SHADER_DIR}/*.rchit ${SHADER_DIR}/*.rmiss)

find_package(Vulkan)

foreach(SHADER IN LISTS SHADERS)
    get_filename_component(FILENAME ${SHADER} NAME)
    add_custom_command(OUTPUT ${SHADER_DIR}/compiled/${FILENAME}.spv.h
            COMMAND glslang ${SHADER} -V -x -o ${SHADER_DIR}/compiled/${FILENAME}.spv.h
            DEPENDS ${SHADER}
            COMMENT "Compiling ${FILENAME}")
    list(APPEND SPV_SHADERS ${SHADER_DIR}/compiled/${FILENAME}.spv.h)
endForeach()

add_custom_target(shaders ALL DEPENDS ${SPV_SHADERS})


find_package(SDL3 REQUIRED CONFIG REQUIRED COMPONENTS SDL3-shared)
find_package(SDL3_image REQUIRED CONFIG REQUIRED)
find_package(OpenMP REQUIRED)
add_executable(RayMeower src/main.c
        src/MeowMath.h
        src/ObjParser.h
        src/bvh.h
        src/nkui.h
        src/vk.h
        ${SHADERS}
)
add_dependencies(RayMeower shaders)
target_link_libraries(RayMeower PRIVATE SDL3::SDL3 SDL3_image::SDL3_image m OpenMP::OpenMP_C vulkan)
