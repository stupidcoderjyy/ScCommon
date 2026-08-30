//
// Created by PC on 2026/8/30.
//

#include <nlohmann/json.hpp>

// 先包含目标 JSON 库头文件，使 json_parser.h 能自动适配 nlohmann::json。
#define SC_JSON_TEST_SUITE_NAME JsonParserNlohmann
#define SC_JSON_TYPE nlohmann::json
#define SC_JSON_MAKE_JSON() \
    SC_JSON_TYPE::parse(R"({"a":{"b":42},"neg":-7,"list":[1,2,3],"s":"hello","d":3.14,"flag":true,"obj_list":[{"id":1},{"id":2}],"point":{"x":10,"y":20}})")
#define SC_JSON_MAKE_JSON_LEAF() SC_JSON_TYPE::parse(R"(42)")
#define SC_JSON_ARRAY_ELEMENT(arr, index) (arr)[index]

#include "json_parser_test_suite.inc"

#undef SC_JSON_ARRAY_ELEMENT
#undef SC_JSON_MAKE_JSON_LEAF
#undef SC_JSON_MAKE_JSON
#undef SC_JSON_TYPE
#undef SC_JSON_TEST_SUITE_NAME
