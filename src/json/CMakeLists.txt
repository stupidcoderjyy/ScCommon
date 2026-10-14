# 开发阶段要求存在所有json库
find_package(nlohmann_json REQUIRED)
find_package(JsonCpp REQUIRED)
find_package(RapidJSON REQUIRED)

# cmake安装
include(GNUInstallDirs)

set(TARGET sc_json)

# 纯头文件库
add_library(${TARGET} INTERFACE)
add_library(sc::json ALIAS ${TARGET})

target_include_directories(${TARGET} INTERFACE
    # 三方库
    ${RAPIDJSON_INCLUDE_DIRS}
    # 项目测试使用的头文件路径
    ${CMAKE_CURRENT_SOURCE_DIR}/..
)

target_link_libraries(${TARGET} INTERFACE
    nlohmann_json::nlohmann_json
    JsonCpp::JsonCpp
    sc::reflect
)

set(SC_JSON_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}/json-${SC_JSON_VERSION}/include")

# 只安装库头文件：逐个显式列出，不安装测试代码，也不安装 CMakeLists.txt
install(FILES
    json_access_mixin.h
    json_extractor.h
    json_path.h
    json_traits.h
    json_traits_mixin.h
    json_view.h
    json_view.hpp
    DESTINATION ${SC_JSON_INSTALL_PREFIX}
)
install(FILES
    traits/jsoncpp.h
    traits/nlohmann.h
    traits/rapidjson.h
    DESTINATION ${SC_JSON_INSTALL_PREFIX}/traits
)
install(FILES
    view/json_array_view.h
    view/json_object_view.h
    DESTINATION ${SC_JSON_INSTALL_PREFIX}/view
)
install(FILES
    util/json_detail.h
    util/json_error.h
    util/json_path_util.h
    DESTINATION ${SC_JSON_INSTALL_PREFIX}/util
)

add_format_target(${TARGET}
    ${CMAKE_CURRENT_SOURCE_DIR}/*.h
    ${CMAKE_CURRENT_SOURCE_DIR}/**/*.h
)
