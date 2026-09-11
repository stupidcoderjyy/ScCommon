find_package(nlohmann_json REQUIRED)
find_package(JsonCpp REQUIRED)
find_package(RapidJSON REQUIRED)

set(TARGET sccommon_json)

# JsonParser 为 header-only 模板库，使用 INTERFACE 库暴露头文件与依赖。
# 后续手动安装其他 JSON 库（boost::json / rapidjson 等）时，在下方补充依赖
# 并在 json_parser.h 的自动适配区加入对应 traits 头文件即可。
add_library(${TARGET} INTERFACE)
add_library(sccommon::json ALIAS ${TARGET})
# 导出到安装包时使用 sccommon::json（构建期目标名保持 sccommon_json）
set_target_properties(${TARGET} PROPERTIES EXPORT_NAME json)

# 公共 include 根目录：构建期是 src（对应 #include "json/json_parser.h"），
# 安装后是 include/（对应 include/json/json_parser.h）。
get_filename_component(SCCOMMON_JSON_INCLUDE_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/.." ABSOLUTE)
target_include_directories(sccommon_json INTERFACE
        $<BUILD_INTERFACE:${SCCOMMON_JSON_INCLUDE_ROOT}>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)
# rapidjson 无 CMake 导入目标，仅提供头文件路径：
# 构建期直接使用；安装后由 ScCommonJsonConfig.cmake 通过 find_dependency(RapidJSON) 补充。
target_include_directories(sccommon_json INTERFACE
        $<BUILD_INTERFACE:${RAPIDJSON_INCLUDE_DIRS}>
)
target_link_libraries(sccommon_json INTERFACE
        nlohmann_json::nlohmann_json
        JsonCpp::JsonCpp
)

# ---- 安装与 find_package 支持 ----
include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

# CMAKE_INSTALL_PREFIX 未显式指定时，默认安装到 LIB_PREFIX_PATH
if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    set(CMAKE_INSTALL_PREFIX "${LIB_PREFIX_PATH}" CACHE PATH "Install prefix" FORCE)
endif()

set(SCCOMMON_JSON_VERSION 1.0.0)

# 头文件安装到 include/json/（含 traits/ 子目录）
install(FILES
        json_parser.h
        json_parser.hpp
        json_traits.h
        json_extractor.h
        json_path.h
        json_detail.h
        json_error.h
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/json
)
install(DIRECTORY traits view
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/json
        FILES_MATCHING PATTERN "*.h"
)

# 导出目标：安装后为 sccommon::json
install(TARGETS sccommon_json
        EXPORT ScCommonJsonTargets
)

configure_package_config_file(
        sccommonConfig.cmake.in
        ${CMAKE_CURRENT_BINARY_DIR}/sccommonConfig.cmake
        INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/sccommon
)
write_basic_package_version_file(
        ${CMAKE_CURRENT_BINARY_DIR}/sccommonConfigVersion.cmake
        VERSION ${SCCOMMON_JSON_VERSION}
        COMPATIBILITY SameMajorVersion
)

install(EXPORT ScCommonJsonTargets
        FILE sccommonTargets.cmake
        NAMESPACE sccommon::
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/sccommon
)
install(FILES
        ${CMAKE_CURRENT_BINARY_DIR}/sccommonConfig.cmake
        ${CMAKE_CURRENT_BINARY_DIR}/sccommonConfigVersion.cmake
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/sccommon
)

add_format_target(sccommon_json
    src/json/*.h
    src/json/*.hpp
)
