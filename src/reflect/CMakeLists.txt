set(TARGET sc_reflect)

# 纯头文件库
add_library(${TARGET} INTERFACE)
add_library(sc::reflect ALIAS ${TARGET})

target_include_directories(${TARGET} INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
)

add_format_target(${TARGET}
    ${CMAKE_CURRENT_SOURCE_DIR}/*.h
    ${CMAKE_CURRENT_SOURCE_DIR}/**/*.h
)
