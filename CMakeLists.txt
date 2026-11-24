cmake_minimum_required(VERSION 3.20)
project(GreenTriangle LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(GreenTriangle src/main.cpp)

target_link_libraries(GreenTriangle PRIVATE opengl32)

if(MSVC)
    target_compile_options(GreenTriangle PRIVATE /utf-8 /W4)
    target_compile_definitions(GreenTriangle PRIVATE NOMINMAX WIN32_LEAN_AND_MEAN)
endif()
