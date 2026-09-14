# 开发阶段要求存在所有json库
find_package(nlohmann_json REQUIRED)
find_package(JsonCpp REQUIRED)
find_package(RapidJSON REQUIRED)

# cmake安装
include(GNUInstallDirs)
# find_package支持
include(CMakePackageConfigHelpers)

set(TARGET sc_json)

# 纯头文件库
add_library(${TARGET} INTERFACE)
add_library(sc::json ALIAS ${TARGET})

target_include_directories(${TARGET} INTERFACE
    # 三方库
    $<BUILD_INTERFACE:${RAPIDJSON_INCLUDE_DIRS}>
    # 项目测试使用的头文件路径
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..>
    # 导出后的头文件路径
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

target_link_libraries(${TARGET} INTERFACE
    nlohmann_json::nlohmann_json
    JsonCpp::JsonCpp
)

# 覆盖导入目标名称
set_target_properties(${TARGET} PROPERTIES EXPORT_NAME json)

# 默认安装路径
if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    set(CMAKE_INSTALL_PREFIX "${LIB_PREFIX_PATH}" CACHE PATH "Install prefix" FORCE)
endif()

# 安装头文件
install(FILES
    json_view.h
    json_view.hpp
    json_access_mixin.h
    json_extractor.h
    json_path.h
    json_traits.h
    json_traits_mixin.h
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/json
)
install(DIRECTORY traits view util
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/json
)

set(PACKAGE_NAME sc)
set(CONFIG_FILE_NAME ${PACKAGE_NAME}-config.cmake)
set(TARGETS_FILE_NAME ${PACKAGE_NAME}-targets.cmake)
set(CONFIG_VERSION_FILE_NAME ${PACKAGE_NAME}-config-version.cmake)
set(CONFIG_INSTALL_DIR ${CMAKE_INSTALL_LIBDIR}/cmake/${PACKAGE_NAME})

# 生成包配置文件
configure_package_config_file(
    ${CONFIG_FILE_NAME}.in
    ${CMAKE_CURRENT_BINARY_DIR}/${CONFIG_FILE_NAME}
    INSTALL_DESTINATION ${CONFIG_INSTALL_DIR}
)

# 生成版本配置文件
set(VERSION 1.0.0)
write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/${CONFIG_VERSION_FILE_NAME}
    VERSION ${VERSION}
    COMPATIBILITY SameMajorVersion
)

# 安装包配置文件
install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/${CONFIG_FILE_NAME}
    ${CMAKE_CURRENT_BINARY_DIR}/${CONFIG_VERSION_FILE_NAME}
    DESTINATION ${CONFIG_INSTALL_DIR}
)

# 生成目标配置文件
install(TARGETS ${TARGET}
    EXPORT EXPORT_TARGET
)

install(EXPORT EXPORT_TARGET
    FILE ${TARGETS_FILE_NAME}
    NAMESPACE sc::
    DESTINATION ${CONFIG_INSTALL_DIR}
)

add_format_target(${TARGET}
    ${CMAKE_CURRENT_SOURCE_DIR}/*.h
    ${CMAKE_CURRENT_SOURCE_DIR}/**/*.h
)
