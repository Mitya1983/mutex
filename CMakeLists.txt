cmake_minimum_required(VERSION 3.25)

project(ipc-lock LANGUAGES CXX DESCRIPTION "")

set(CMAKE_CXX_STANDARD 20)

option(BUILD_STATIC "" ON)
option(ENABLE_ASAN "Enables asan build" OFF)

if (PROJECT_IS_TOP_LEVEL)
    if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
        # using Clang
        if (CMAKE_BUILD_TYPE STREQUAL "Debug")
            set(CMAKE_CXX_FLAGS "-g -g3 -glldb -ggdb -ggdb3 -O0 -Wall -Wextra -Wpedantic -Wfloat-equal -Werror")
            if (${ENABLE_ASAN})
                set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address -shared-libasan")
            endif ()
        endif ()
        if (CMAKE_BUILD_TYPE STREQUAL "Release")
            set(CMAKE_CXX_FLAGS "-g0 -ggdb0 -glldb0 -O3")
        endif ()
    elseif (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        # using GCC
        if (CMAKE_BUILD_TYPE STREQUAL "Debug")
            set(CMAKE_CXX_FLAGS "-g -ggdb3 -O0 -Wall -Wextra -Wpedantic -Wfloat-equal -Werror")
            if (${ENABLE_ASAN})
                set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address")
            endif ()
        endif ()
        if (CMAKE_BUILD_TYPE STREQUAL "Release")
            set(CMAKE_CXX_FLAGS "-g0 -ggdb0 -O3")
        endif ()
    elseif (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
        # using Visual Studio C++
        if (CMAKE_BUILD_TYPE STREQUAL "Debug")
            set(CMAKE_CXX_FLAGS "/Zi /Od /JMC /WX")
        endif ()
        if (CMAKE_BUILD_TYPE STREQUAL "Release")
            set(CMAKE_CXX_FLAGS "/Ot /O2")
        endif ()
    else ()
        message(FATAL_ERROR "Compiler not supported")
    endif ()
endif ()
include_directories(
        inc/
)

file(GLOB INC_FILES CONFIGURE_DEPENDS
        ${PROJECT_SOURCE_DIR}/inc/*.hpp
)
file(GLOB SRC_FILES CONFIGURE_DEPENDS
        ${PROJECT_SOURCE_DIR}/src/*.cpp
)

set(LIB_VERSION 1.0.1)

if ("${CMAKE_INSTALL_PREFIX}" STREQUAL "")
    message(STATUS "CMAKE_INSTALL_PREFIX is not set. $ENV{HOME} directory will be used")
    set(INSTALL_PREFIX "$ENV{HOME}")
else ()
    set(INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")
endif ()

set(PROJECT_INSTALL_DIR ${INSTALL_PREFIX}/${PROJECT_NAME})
set(LIBRARY_INSTALL_DIR ${PROJECT_INSTALL_DIR}/${CMAKE_BUILD_TYPE})
message(STATUS "${PROJECT_NAME}: Library install directory is set to ${LIBRARY_INSTALL_DIR}")
set(INCLUDE_INSTALL_DIR ${LIBRARY_INSTALL_DIR}/inc)
message(STATUS "${PROJECT_NAME}: Include install directory is set to ${INCLUDE_INSTALL_DIR}")
set(CMAKE_CONFIG_INSTALL_DIR ${INSTALL_PREFIX}/cmake/${CMAKE_BUILD_TYPE})
message(STATUS "${PROJECT_NAME}: CMake config files install directory is set to ${CMAKE_CONFIG_INSTALL_DIR}")

if (BUILD_STATIC)
    message(STATUS "${PROJECT_NAME}: Will build static library")
    add_library(${PROJECT_NAME} STATIC)
else ()
    message(STATUS "${PROJECT_NAME}: Will build shared library")
    add_library(${PROJECT_NAME} SHARED)
endif ()

if (PROJECT_IS_TOP_LEVEL)
    if (CMAKE_BUILD_TYPE STREQUAL "Debug")
        target_compile_definitions(${PROJECT_NAME} PUBLIC TRISTAN_DEBUG)
    endif ()
endif ()

if (${ENABLE_ASAN} AND CMAKE_CXX_COMPILER_ID STREQUAL "Clang" AND CMAKE_BUILD_TYPE STREQUAL "Debug")
    string(REPLACE "." ";" CLANG_VERSION ${CMAKE_CXX_COMPILER_VERSION})
    list(GET CLANG_VERSION 0 CLANG_VERSION_MAJOR)
    set(PATH_TO_CLANG_DIRECTORY "/usr/lib/llvm-${CLANG_VERSION_MAJOR}/lib/clang/")

    execute_process(
            COMMAND find ${PATH_TO_CLANG_DIRECTORY} -name libclang_rt.asan-x86_64.so
            OUTPUT_VARIABLE PATH_TO_CLANG_LIB_DIRECTORY
    )
    string(STRIP ${PATH_TO_CLANG_LIB_DIRECTORY} PATH_TO_CLANG_LIB_DIRECTORY)
    target_link_directories(${PROJECT_NAME} PRIVATE
            ${PATH_TO_CLANG_LIB_DIRECTORY}
    )
    target_link_libraries(${PROJECT_NAME} PRIVATE
            -lclang_rt.asan-x86_64
    )
endif ()

target_sources(
        ${PROJECT_NAME}
        PUBLIC ${INC_FILES}
        PRIVATE ${SRC_FILES}
)

set_target_properties(
        ${PROJECT_NAME}
        PROPERTIES
        VERSION ${LIB_VERSION}
        OUTPUT_NAME ${PROJECT_NAME}
)
