cmake_minimum_required (VERSION 3.25)

# ============================================================
# Policy
# ============================================================
# CMP0091: "Honor standard library usage requirements when using MSVC toolset"
if(WIN32 AND POLICY CMP0091)
    cmake_policy(SET CMP0091 NEW)
endif()
# CMP0141: "Allow MSVC to use Edit and Continue when using the Program Database format for debug information"
if (WIN32 AND POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

# ============================================================
# Project
# ============================================================
set(ROOT_PROJECT_NAME "Bedrock")
project(${ROOT_PROJECT_NAME} LANGUAGES CXX)

set(CMAKE_CXX_STANDARD             23)
set(CMAKE_CXX_STANDARD_REQUIRED    ON)
set(CMAKE_CXX_EXTENSIONS          OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS  ON)

if(CMAKE_SIZEOF_VOID_P EQUAL 8)
  set(HOST_ARCH_AMD64 TRUE)
else()
  set(HOST_ARCH_AMD64 FALSE)
endif()

# ============================================================
# Target – Bedrock
# ============================================================
file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS "src/*.cc")

if (CMAKE_MSVC_RUNTIME_LIBRARY MATCHES "DLL$")
    add_library(${ROOT_PROJECT_NAME} SHARED ${SOURCES})
else ()
    add_library(${ROOT_PROJECT_NAME} STATIC ${SOURCES})
endif ()

target_include_directories(${ROOT_PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

# ============================================================
# Dependencies
# ============================================================
add_subdirectory(Common)
target_link_libraries(${ROOT_PROJECT_NAME} PUBLIC ${ROOT_PROJECT_NAME}::Common)
add_subdirectory(Encryption)
target_link_libraries(${ROOT_PROJECT_NAME} PUBLIC ${ROOT_PROJECT_NAME}::Encryption)
add_subdirectory(Networking)
target_link_libraries(${ROOT_PROJECT_NAME} PUBLIC ${ROOT_PROJECT_NAME}::Networking)

# ============================================================
# Compiler Options
# ============================================================
if (TARGET ${ROOT_PROJECT_NAME})
    target_precompile_headers(${ROOT_PROJECT_NAME} PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/include/pch.h"
    )
    if(MSVC)
        target_compile_options(${ROOT_PROJECT_NAME} PRIVATE
        /MP
        /W4              # 합리적인 모든 경고
        /WX              # 경고를 에러로 (Linux의 -Werror)
        /permissive-     # 표준 엄격 모드 (Microsoft 확장 거부)
        /w14242 /w14254 /w14263 /w14265 /w14287 /we4289
        /w14296 /w14311 /w14545 /w14546 /w14547 /w14549
        /w14555 /w14619 /w14640 /w14826 /w14905 /w14906
        /w14928
        /Zc:__cplusplus  # __cplusplus 매크로 정확히 보고
        /Zc:preprocessor # 표준 준수 전처리기
        )
    endif()
endif()

if(UNIX AND NOT APPLE AND CMAKE_BUILD_TYPE STREQUAL "Release")
    set_target_properties(${ROOT_PROJECT_NAME} PROPERTIES
        BUILD_WITH_INSTALL_RPATH TRUE
        INSTALL_RPATH            "$ORIGIN"
        SKIP_BUILD_RPATH         FALSE
        BUILD_RPATH              "$ORIGIN"
    )
endif()

if (NOT WIN32)
    target_compile_options(
        ${ROOT_PROJECT_NAME} PRIVATE
        -maes
        -msse2
        -mssse3
        -fno-exceptions -fno-rtti
    )
else()
    add_compile_options(/utf-8)
endif()

if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
    add_compile_options(-Wuseless-cast)
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
    add_compile_options(
        -Weverything
        -Wno-c++98-compat
        -Wno-c++98-compat-pedantic
        -Wno-unused-macros
        -Wno-padded
    )
endif ()

if(WIN32)
    target_link_libraries(${ROOT_PROJECT_NAME} PUBLIC ws2_32)
    target_link_libraries(${ROOT_PROJECT_NAME} PUBLIC crypt32)
    set_target_properties(${ROOT_PROJECT_NAME} PROPERTIES WINDOWS_EXPORT_ALL_SYMBOLS ON)
else()
    add_compile_options(
        -Werror
        -Wall
        -Wextra
        -Wpedantic
        -Wconversion
        -Wsign-conversion
        -Wshadow
        -Wundef
        -Wunreachable-code
        -Wstrict-aliasing
        -Wnull-dereference
        -Wdouble-promotion
        -Wformat=2
        -Wcast-qual
        -Wcast-align
    )
endif()

# ============================================================
# clang-tidy (Linux only)
# ============================================================
find_program(CLANG_TIDY_EXE NAMES clang-tidy)
if(CLANG_TIDY_EXE)
    set(CMAKE_CXX_CLANG_TIDY
        $<$<COMPILE_LANGUAGE:CXX>:clang-tidy;-checks=-*,google-readability-casting;-fix;-fix-errors>
    )
    if (PROJECT_IS_TOP_LEVEL AND CMAKE_EXPORT_COMPILE_COMMANDS AND NOT WIN32)
    execute_process(
        COMMAND ${CMAKE_COMMAND} -E create_symlink
                ${CMAKE_BINARY_DIR}/compile_commands.json
                ${CMAKE_BINARY_DIR}/../compile_commands.json
    )
    endif()
endif()

# ============================================================
# Tests (top-level only)
# ============================================================
if (${PROJECT_IS_TOP_LEVEL})
    set(BUILD_TESTING ON)

    include(CTest)
    enable_testing()

    add_subdirectory(test)

    add_custom_command(TARGET ${ROOT_PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "$<TARGET_FILE:${ROOT_PROJECT_NAME}>"
        "${CMAKE_BINARY_DIR}/test"
    COMMENT "Copying ${ROOT_PROJECT_NAME} DLL/so to test output directory"
    )
endif()

configure_file("${CMAKE_CURRENT_SOURCE_DIR}/.cmake/cmake_include/config.h.in" "${CMAKE_CURRENT_BINARY_DIR}/config.h" @ONLY)
target_include_directories(${ROOT_PROJECT_NAME} SYSTEM PUBLIC "${CMAKE_CURRENT_BINARY_DIR}")
