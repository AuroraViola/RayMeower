cmake_minimum_required(VERSION 4.0)
project(RayMeower C)

set(CMAKE_C_STANDARD 23)

include_directories(third-party/Nuklear third-party/Nuklear/demo/sdl3_renderer)

find_package(SDL3 REQUIRED CONFIG REQUIRED COMPONENTS SDL3-shared)
find_package(SDL3_image REQUIRED CONFIG REQUIRED)
find_package(OpenMP REQUIRED)
add_executable(RayMeower src/main.c
        src/MeowMath.h
        src/ObjParser.h
        src/bvh.h
        src/nkui.h
)
target_link_libraries(RayMeower PRIVATE SDL3::SDL3 SDL3_image::SDL3_image m OpenMP::OpenMP_C)
