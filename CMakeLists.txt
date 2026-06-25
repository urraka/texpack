cmake_minimum_required(VERSION 3.15)

project(texpack LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(MSVC)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

find_package(PNG REQUIRED)
find_package(ZLIB REQUIRED)

set(SOURCES
    src/main.cpp
    src/packer.cpp
    src/bleeding.cpp
    src/png/png.cpp
    src/rbp/MaxRects.cpp
    src/help.h
    src/packer.h
    src/bleeding.h
    src/png/png.h
    src/rbp/MaxRects.h
)

add_executable(texpack ${SOURCES})

target_link_libraries(texpack PRIVATE
    PNG::PNG
    ZLIB::ZLIB
)