cmake_minimum_required (VERSION 3.16)
set(ROOT_PROJECT_NAME "Bedrock")
project(${ROOT_PROJECT_NAME} LANGUAGES CXX)

include(CTest)
enable_testing()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_library(${ROOT_PROJECT_NAME} INTERFACE)

add_subdirectory(Common)
target_link_libraries(${ROOT_PROJECT_NAME} INTERFACE ${ROOT_PROJECT_NAME}::Common)
add_subdirectory(Networking)
target_link_libraries(${ROOT_PROJECT_NAME} INTERFACE ${ROOT_PROJECT_NAME}::Networking)

add_subdirectory(test)

add_compile_options(-Werror -Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion -Wshadow -Wundef -Wunreachable-code -Wstrict-aliasing -Wnull-dereference -Wdouble-promotion -Wformat=2 -Wcast-qual -Wcast-align)

set(CMAKE_CXX_CLANG_TIDY "clang-tidy;-checks=-*,google-readability-casting;-fix;-fix-errors;")

if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
    # Additional flags for GCC
    add_compile_options(-Wuseless-cast)
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
    # Additional flags for Clang
    add_compile_options(-Weverything -Wno-c++98-compat -Wno-c++98-compat-pedantic -Wno-unused-macros -Wno-padded)
endif ()

if (CMAKE_EXPORT_COMPILE_COMMANDS)
  execute_process(
    COMMAND ${CMAKE_COMMAND} -E create_symlink
            ${CMAKE_BINARY_DIR}/compile_commands.json
            ${CMAKE_BINARY_DIR}/../compile_commands.json
  )
endif()