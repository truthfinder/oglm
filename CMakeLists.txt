cmake_minimum_required(VERSION 3.15)

project (oglm)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set(ROOT_DIR ${CMAKE_CURRENT_SOURCE_DIR})

set(CMAKE_CONFIGURATION_TYPES Debug Release RelWithDebInfo CACHE STRING INTERNAL FORCE)

set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /D_DEBUG")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /D_SECURE_SCL=0")
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} /D_SECURE_SCL=0")

set(QUAKE_DIR "h:/games/quake1")
set(QUAKE2_DIR "h:/games/quake2")

add_definitions(-DUNICODE -D_UNICODE -D_CRT_SECURE_NO_WARNINGS)

set(CMAKE_CXX_STANDARD 20) 
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -MP -arch:SSE2") # -Wall

include_directories("3rdparty/zlib")
include_directories("3rdparty/zlib/_build")
link_directories("3rdparty/zlib/_build")

set(CMAKE_CXX_STANDARD_LIBRARIES "kernel32.lib user32.lib gdi32.lib")

add_subdirectory(opengl32)
add_subdirectory(ogl)

add_dependencies(ogl opengl32)

set_property(DIRECTORY PROPERTY VS_STARTUP_PROJECT opengl32)
