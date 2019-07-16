cmake_minimum_required(VERSION 3.0)
project(texpack)

set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MT")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /MTd")

find_package(PNG REQUIRED)
include_directories(${PNG_INCLUDE_DIRS})

find_package(ZLIB REQUIRED)
include_directories(${ZLIB_INCLUDE_DIRS})

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
target_link_libraries(texpack  PNG::PNG ZLIB::ZLIB)
