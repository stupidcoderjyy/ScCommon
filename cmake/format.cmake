# 代码格式化
function(add_format_target TARGET)
    find_program(CLANG_FORMAT
            NAMES clang-format
            PATHS ${LIB_PREFIX_PATH}/bin
            NO_DEFAULT_PATH  # 仅搜索指定路径，不搜索系统默认路径
    )
    if(NOT CLANG_FORMAT)
        find_program(CLANG_FORMAT NAMES clang-format)
    endif()

    if(CLANG_FORMAT)
        file(GLOB_RECURSE ALL_SOURCE_FILES ${ARGN})
        add_custom_target(format_${TARGET}
                COMMAND ${CLANG_FORMAT} -i -style=file ${ALL_SOURCE_FILES}
                COMMENT "Running clang-format on all source files."
                WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
        )
        # INTERFACE 库没有构建步骤，跳过依赖挂接
        get_target_property(_format_target_type ${TARGET} TYPE)
        if(NOT _format_target_type STREQUAL "INTERFACE_LIBRARY")
            add_dependencies(${TARGET} format_${TARGET})
        endif()

        message(STATUS "clang-format found at ${CLANG_FORMAT}")
    else()
        message(WARNING "clang-format not found. The 'format' target will not be available.")
    endif()
endfunction()
