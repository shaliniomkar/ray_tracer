cmake_minimum_required(VERSION 3.15)

project(ray_tracer LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(main main.cc)

add_compile_definitions(NOMINMAX)
add_compile_definitions(_CRT_SECURE_NO_WARNINGS)

add_compile_definitions(UNICODE=0 _UNICODE=0)