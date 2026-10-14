cmake_minimum_required(VERSION 4.2)
project(ScCommon)

include(cmake/format.cmake)
include(cmake/add_test_target.cmake)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS_DEBUG  "-O0 -g")
set(CMAKE_CXX_FLAGS_RELEASE "-O2 -DNDEBUG")
list(PREPEND CMAKE_PREFIX_PATH "C:/msys64/mingw64")

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# 默认安装路径
if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    set(CMAKE_INSTALL_PREFIX "${LIB_PREFIX_PATH}" CACHE PATH "Install prefix" FORCE)
endif()

add_subdirectory(src)
add_subdirectory(thirdparty)
add_subdirectory(test)
add_subdirectory(obsidian)
