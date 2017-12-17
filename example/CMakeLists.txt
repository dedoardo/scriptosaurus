cmake_minimum_required(VERSION 3.2 FATAL_ERROR)
project(Scriptosaurus_example VERSION 1.0 LANGUAGES C)
add_executable(Scriptosaurus_example ../scriptosaurus.h source.c)

set(compile_definitions "")
set(link_libraries "")
set(properties "")

if (WIN32)
    list(APPEND compile_definitions _CRT_SECURE_NO_WARNINGS)
    list(APPEND properties VS_DEBUGGER_WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
endif (WIN32)

if (UNIX)
    list(APPEND link_libraries pthread dl)
endif (UNIX)

target_compile_definitions(Scriptosaurus_example PUBLIC ${compile_definitions})
target_link_libraries(Scriptosaurus_example PUBLIC ${link_libraries})
set_target_properties(Scriptosaurus_example PROPERTIES ${properties})
