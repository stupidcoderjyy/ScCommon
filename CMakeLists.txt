cmake_minimum_required(VERSION 4.2)
project(ScCommon)

if(POLICY CMP0207)
    # 依赖扫描时先规范化路径再匹配正则，确保排除系统 DLL
    cmake_policy(SET CMP0207 NEW)
endif()

include(cmake/format.cmake)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS_DEBUG  "-O0 -g")
set(CMAKE_CXX_FLAGS_RELEASE "-O2 -DNDEBUG")
set(LIB_PREFIX_PATH "C:/msys64/mingw64")
list(PREPEND CMAKE_PREFIX_PATH ${LIB_PREFIX_PATH})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

add_subdirectory(src)
add_subdirectory(thirdparty)
add_subdirectory(test)
