cmake_minimum_required(VERSION 4.0)
project(RayMeower C)

set(CMAKE_C_STANDARD 23)

find_package(SDL3 REQUIRED CONFIG REQUIRED COMPONENTS SDL3-shared)
find_package(SDL3_image REQUIRED CONFIG REQUIRED)
find_package(OpenMP REQUIRED)
add_executable(RayMeower main.c
        MeowMath.h
        ObjParser.h
        bvh.h)
target_link_libraries(RayMeower PRIVATE SDL3::SDL3 SDL3_image::SDL3_image m OpenMP::OpenMP_C)
