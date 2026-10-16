cmake_minimum_required(VERSION 4.2)
project(ScCommon)

include(cmake/format.cmake)
include(cmake/add_test_target.cmake)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS_DEBUG  "-O0 -g")
set(CMAKE_CXX_FLAGS_RELEASE "-O2 -DNDEBUG")
set(LIB_PREFIX_PATH "C:/msys64/mingw64")
list(PREPEND CMAKE_PREFIX_PATH ${LIB_PREFIX_PATH})

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# 版本号
set(SC_COMMON_VERSION "3.0")

if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    set(CMAKE_INSTALL_PREFIX "${LIB_PREFIX_PATH}/${SC_COMMON_VERSION}" CACHE PATH "Install prefix" FORCE)
else ()
    set(CMAKE_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}/${SC_COMMON_VERSION}" CACHE PATH "Install prefix" FORCE)
endif()

# 头文件安装：把 json 与 reflect 下的头文件复制到 CMAKE_INSTALL_PREFIX 下面
install(DIRECTORY
    ${CMAKE_CURRENT_SOURCE_DIR}/src/json
    ${CMAKE_CURRENT_SOURCE_DIR}/src/reflect
    DESTINATION include
    FILES_MATCHING
    PATTERN "*.h"
    PATTERN "*.hpp"
)

add_subdirectory(src)
add_subdirectory(thirdparty)
add_subdirectory(benchmark)
add_subdirectory(test)
add_subdirectory(obsidian)
