enable_testing()
include(GoogleTest)

set(TEST_SRC_JSON_NLOHMANN json/json_parser_nlohmann_test.cpp)
set(TEST_SRC_JSON_RAPIDJSON json/json_parser_rapidjson_test.cpp)
set(TEST_SRC_JSON_JSONCPP json/json_parser_jsoncpp_test.cpp)
set(TEST_SRC_JSON_PATH json/json_path_test.cpp)

add_executable(sccommon_test_json_nlohmann ${TEST_SRC_JSON_NLOHMANN})
add_executable(sccommon_test_json_rapidjson ${TEST_SRC_JSON_RAPIDJSON})
add_executable(sccommon_test_json_jsoncpp ${TEST_SRC_JSON_JSONCPP})
add_executable(sccommon_test_json_path ${TEST_SRC_JSON_PATH})
add_executable(sccommon_test_all
    ${TEST_SRC_JSON_NLOHMANN}
    ${TEST_SRC_JSON_RAPIDJSON}
    ${TEST_SRC_JSON_JSONCPP}
    ${TEST_SRC_JSON_PATH}
)

set(TEST_TARGETS
    sccommon_test_json_nlohmann
    sccommon_test_json_rapidjson
    sccommon_test_json_jsoncpp
    sccommon_test_json_path
    sccommon_test_all
)
foreach (TARGET IN LISTS TEST_TARGETS)
    target_link_libraries(${TARGET} PRIVATE
        sc::json
        GTest::gtest_main
    )
    gtest_discover_tests(${TARGET} WORKING_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
endforeach ()
