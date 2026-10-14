add_test_target(test_json_nlohmann
    SOURCES json_parser_nlohmann_test.cpp
    LIBS sc::json)

add_test_target(test_json_rapidjson
    SOURCES json_parser_rapidjson_test.cpp
    LIBS sc::json)

add_test_target(test_json_jsoncpp
    SOURCES json_parser_jsoncpp_test.cpp
    LIBS sc::json)

add_test_target(test_json_path
    SOURCES json_path_test.cpp
    LIBS sc::json)

add_test_target(test_json_all
    SOURCES
    json_parser_jsoncpp_test.cpp
    json_parser_nlohmann_test.cpp
    json_parser_rapidjson_test.cpp
    json_path_test.cpp
    LIBS
    sc::json
)