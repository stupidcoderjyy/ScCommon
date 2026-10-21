# 开发阶段要求存在所有json库

set(TARGET sc_json)

# 纯头文件库
add_library(${TARGET} INTERFACE)
add_library(sc::json ALIAS ${TARGET})

target_include_directories(${TARGET} INTERFACE
    # 三方库
    ${PROJECT_SOURCE_DIR}/thirdparty/nlohmann/include
    ${PROJECT_SOURCE_DIR}/thirdparty/rapidjson/include
    # 项目测试使用的头文件路径
    ${CMAKE_CURRENT_SOURCE_DIR}/..
)

target_link_libraries(${TARGET} INTERFACE
    jsoncpp_static
    sc::reflect
    sc::common
    sc::compile_options
)

add_format_target(${TARGET}
    ${CMAKE_CURRENT_SOURCE_DIR}/*.h
    ${CMAKE_CURRENT_SOURCE_DIR}/**/*.h
)
