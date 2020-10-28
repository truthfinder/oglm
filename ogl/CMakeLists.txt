cmake_minimum_required (VERSION 3.15)
project (ogl)

#set(CMAKE_EXE_LINKER_FLAGS_DEBUG "${CMAKE_EXE_LINKER_FLAGS_DEBUG} /debug:fastlink")
#set(CMAKE_EXE_LINKER_FLAGS_RELWITHDEBINFO "${CMAKE_EXE_LINKER_FLAGS_RELWITHDEBINFO} /debug:fastlink")
set(UTILS_DIR "${PROJECT_SOURCE_DIR}/../utils")

file(GLOB SOURCES "src/*.h" "src/*.cpp")
file(GLOB UTILS_SOURCES "${UTILS_DIR}/*.h" "${UTILS_DIR}/*.c" "${UTILS_DIR}/*.cpp")

source_group("" FILES ${SOURCES})
source_group("utils" FILES ${UTILS_SOURCES})

include_directories(${UTILS_DIR})

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${ROOT_DIR}/bin)
add_executable(ogl WIN32 ${SOURCES} ${UTILS_SOURCES})

#target_link_libraries(ogl opengl32)
target_link_libraries(ogl debug zlibstaticd optimized zlibstatic)

set_target_properties(ogl PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")
#set_target_properties(ogl PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${ROOT_DIR}/bin")
set_target_properties(ogl PROPERTIES LINK_FLAGS "/ignore:4099")
