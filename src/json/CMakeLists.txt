# 开发阶段要求存在所有json库
find_package(nlohmann_json REQUIRED)
find_package(JsonCpp REQUIRED)
find_package(RapidJSON REQUIRED)

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

add_format_target(${TARGET}
    ${CMAKE_CURRENT_SOURCE_DIR}/*.h
    ${CMAKE_CURRENT_SOURCE_DIR}/**/*.h
)
