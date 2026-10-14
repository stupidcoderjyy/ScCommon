# cmake安装
include(GNUInstallDirs)

set(TARGET sc_reflect)

# 纯头文件库
add_library(${TARGET} INTERFACE)
add_library(sc::reflect ALIAS ${TARGET})

target_include_directories(${TARGET} INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
)

set(SC_REFLECT_VERSION "1.0")
set(SC_REFLECT_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}/reflect-${SC_JSON_VERSION}/include")

# 只安装库头文件：逐个显式列出，不安装测试代码，也不安装 CMakeLists.txt
install(FILES
    pfr.h
    DESTINATION ${SC_REFLECT_INSTALL_PREFIX}
)

add_format_target(${TARGET}
    ${CMAKE_CURRENT_SOURCE_DIR}/*.h
    ${CMAKE_CURRENT_SOURCE_DIR}/**/*.h
)
