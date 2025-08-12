cmake_minimum_required(VERSION 3.25)

project(mutex LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(BUILD_STATIC "Build static library" ON)
option(BUILD_DOCS "Build docs" OFF)
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
            set(CMAKE_CXX_FLAGS "-g0 -O3")
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
            set(CMAKE_CXX_FLAGS "/Zi /Od /JMC /WX /EHsc")
        endif ()
        if (CMAKE_BUILD_TYPE STREQUAL "Release")
            set(CMAKE_CXX_FLAGS "/Ot /O2 /EHsc")
        endif ()
    else ()
        message(FATAL_ERROR "Compiler not supported")
    endif ()
endif ()

include_directories(
        include/
        )

if (${PROJECT_IS_TOP_LEVEL})
    if (CMAKE_BUILD_TYPE STREQUAL "Debug")
        add_compile_definitions(DEBUG_BUILD)
    endif ()
endif ()

if (${BUILD_STATIC})
    message(STATUS "${PROJECT_NAME}: Will build static library")
    add_library(${PROJECT_NAME} STATIC
                source/mutex.cpp
                )
else ()
    message(STATUS "${PROJECT_NAME}: Will build shared library")
    add_library(${PROJECT_NAME} SHARED
                source/mutex.cpp
                )
endif ()

if (${PROJECT_IS_TOP_LEVEL})
    if (${ENABLE_ASAN} AND CMAKE_CXX_COMPILER_ID STREQUAL "Clang" AND CMAKE_BUILD_TYPE STREQUAL "Debug")
        string(REPLACE "." ";" CLANG_VERSION ${CMAKE_CXX_COMPILER_VERSION})
        list(GET CLANG_VERSION 0 CLANG_VERSION_MAJOR)
        set(PATH_TO_CLANG_DIRECTORY "/usr/lib/llvm-${CLANG_VERSION_MAJOR}/lib/clang/")

        execute_process(
                COMMAND find ${PATH_TO_CLANG_DIRECTORY} -name libclang_rt.asan-x86_64.so
                OUTPUT_VARIABLE PATH_TO_CLANG_LIB_ASAN
                )
        target_link_libraries(${PROJECT_NAME} PRIVATE
                              ${PATH_TO_CLANG_LIB_ASAN}
                              )
    endif ()
endif ()

if ("${CMAKE_INSTALL_PREFIX}" STREQUAL "")
    message(STATUS "${PROJECT_NAME}: CMAKE_INSTALL_PREFIX is not set. ${CMAKE_BINARY_DIR} directory will be used")
    set(CMAKE_INSTALL_PREFIX ${CMAKE_BINARY_DIR})
endif ()

string(TOLOWER ${CMAKE_BUILD_TYPE} BUILD_TYPE)
set(LIBRARY_DIR ${CMAKE_INSTALL_PREFIX}/${PROJECT_NAME}/lib/${BUILD_TYPE})
set(INCLUDE_DIR ${CMAKE_INSTALL_PREFIX}/${PROJECT_NAME}/include)
message(STATUS "${PROJECT_NAME}: Include install directory is set to ${INCLUDE_DIR}")
message(STATUS "${PROJECT_NAME}: Library install directory is set to ${LIBRARY_DIR}")

if (NOT ${PROJECT_IS_TOP_LEVEL})
    set(MUTEX_LIBRARY_DIR ${LIBRARY_DIR} PARENT_SCOPE)
    set(MUTEX_INCLUDE_DIR ${INCLUDE_DIR} PARENT_SCOPE)
endif ()

set(LIB_VERSION 1.0.0)

set_target_properties(
        ${PROJECT_NAME}
        PROPERTIES
        VERSION ${LIB_VERSION}
        OUTPUT_NAME ${PROJECT_NAME}
        PUBLIC_HEADER include/mutex.hpp
        )

install(TARGETS ${PROJECT_NAME}
        PUBLIC_HEADER DESTINATION ${INCLUDE_DIR}
        ARCHIVE DESTINATION ${LIBRARY_DIR}
        LIBRARY DESTINATION ${LIBRARY_DIR}
        )