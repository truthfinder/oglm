cmake_minimum_required(VERSION 3.15)
project(opengl32)

add_definitions(-DOPENGL32_EXPORTS -D_USRDLL)

set(CMAKE_SHARED_LINKER_FLAGS_DEBUG "${CMAKE_SHARED_LINKER_FLAGS_DEBUG} /debug:fastlink")
set(CMAKE_SHARED_LINKER_FLAGS_RELWITHDEBINFO "${CMAKE_SHARED_LINKER_FLAGS_RELWITHDEBINFO} /debug:fastlink")
set(UTILS_DIR "../utils")

file(GLOB SOURCES "src/*.h" "src/*.cpp" "src/*.def")
file(GLOB_RECURSE UTILS_SOURCES "${UTILS_DIR}/*.h" "${UTILS_DIR}/*.c" "${UTILS_DIR}/*.cpp")

source_group("" FILES ${SOURCES})
source_group("utils" FILES ${UTILS_SOURCES})

include_directories(${UTILS_DIR})

set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)
add_library(opengl32 SHARED ${SOURCES} ${UTILS_SOURCES})

target_link_libraries(opengl32 debug zlibstaticd optimized zlibstatic)

set(LOCAL_COMMAND "${QUAKE_DIR}/glquake.exe")
set(LOCAL_COMMAND_ARGUMENTS "-window -width 640 -height 480")
configure_file(../cmake/template.vcxproj.user.in ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.vcxproj.user @ONLY)
set_target_properties(opengl32 PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${QUAKE_DIR}")
set_target_properties(opengl32 PROPERTIES LINK_FLAGS "/ignore:4099")

add_custom_command(TARGET opengl32
                   POST_BUILD
                   COMMAND echo f | xcopy "\"$(OutDir)$(TargetName)$(TargetExt)\"" "\"${QUAKE_DIR}/opengl32.dll\"" /Y /S)
