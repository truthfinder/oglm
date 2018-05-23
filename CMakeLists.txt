cmake_minimum_required(VERSION 3.10)

project (oglm)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

set(CMAKE_CONFIGURATION_TYPES Debug Release RelWithDebInfo CACHE TYPE INTERNAL FORCE)

set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /D_DEBUG")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /D_SECURE_SCL=0")
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} /D_SECURE_SCL=0")

set(QUAKE_DIR "z:/games/quake1")

add_definitions(-DUNICODE -D_UNICODE -D_CRT_SECURE_NO_WARNINGS)

set(CMAKE_CXX_STANDARD 17) 
set(CMAKE_CXX_STANDARD_REQUIRED on)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -MP -arch:SSE2") # -Wall

include_directories("3rdparty/zlib")
include_directories("3rdparty/zlib/_build")
link_directories("3rdparty/zlib/_build")

set(CMAKE_CXX_STANDARD_LIBRARIES "kernel32.lib user32.lib gdi32.lib")

add_subdirectory(opengl32)
add_subdirectory(ogl)

add_dependencies(ogl opengl32)

set_property(DIRECTORY PROPERTY VS_STARTUP_PROJECT opengl32)
