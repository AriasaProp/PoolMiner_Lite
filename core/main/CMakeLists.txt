cmake_minimum_required(VERSION 3.21.0)

project(core LANGUAGES C)
set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSION OFF)

file(GLOB_RECURSE Include_Dir LIST_DIRECTORIES true ".")
include_directories(${Include_Dir})
file(GLOB_RECURSE Src_Files "*.c")

add_library(${PROJECT_NAME} SHARED ${Src_Files})
set_target_properties(${PROJECT_NAME} PROPERTIES LINKER_LANGUAGE C)
