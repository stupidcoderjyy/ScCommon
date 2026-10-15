add_benchmark_target(benchmark_json_nlohmann
    SOURCES benchmark_nlohmann.cpp
    LIBS sc::json
)

add_benchmark_target(benchmark_json_jsoncpp
    SOURCES benchmark_jsoncpp.cpp
    LIBS sc::json
)

add_benchmark_target(benchmark_json_rapidjson
    SOURCES benchmark_rapidjson.cpp
    LIBS sc::json
)
