function(add_test_target name)
    set(options)
    set(one_value_args)
    set(multi_value_args SOURCES LIBS)
    cmake_parse_arguments(ARG
        "${options}" "${one_value_args}" "${multi_value_args}" ${ARGN})

    list(LENGTH ARG_SOURCES src_count)
    add_executable("${name}" ${ARG_SOURCES})
    target_link_libraries("${name}" PRIVATE
        ${ARG_LIBS}
        GTest::gtest_main
    )
    gtest_discover_tests("${name}"
        WORKING_DIRECTORY "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}"
    )
endfunction()