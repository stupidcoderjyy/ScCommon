# 每个基准测试一个可执行文件，链接 benchmark 的主程序
function(add_benchmark_target name)
    set(options)
    set(one_value_args)
    set(multi_value_args SOURCES LIBS)
    cmake_parse_arguments(ARG
        "${options}" "${one_value_args}" "${multi_value_args}" ${ARGN})

    add_executable("${name}" ${ARG_SOURCES})
    target_link_libraries("${name}" PRIVATE
        ${ARG_LIBS}
        benchmark::benchmark
        benchmark::benchmark_main
    )
    add_format_target("${name}" ${ARG_SOURCES})
endfunction()

add_subdirectory(json)
