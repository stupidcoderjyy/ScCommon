//
// Created by PC on 2026/8/30.
//

#include <json/json.h>

#include <sstream>
#include <stdexcept>
#include <string>

namespace {

Json::Value MakeJson() {
    Json::CharReaderBuilder builder;
    Json::Value value;
    const std::string text =
            R"({"a":{"b":42},"neg":-7,"list":[1,2,3],"s":"hello","d":3.14,"flag":true,"obj_list":[{"id":1},{"id":2}],"point":{"x":10,"y":20}})";
    std::istringstream stream(text);
    std::string errors;
    if (!Json::parseFromStream(builder, stream, &value, &errors)) {
        throw std::runtime_error(errors);
    }
    return value;
}

Json::Value MakeLeaf() { return {42}; }

}  // namespace

#define SC_JSON_TEST_SUITE_NAME JsonParserJsoncpp
#define SC_JSON_TYPE Json::Value
#define SC_JSON_MAKE_JSON() MakeJson()
#define SC_JSON_MAKE_JSON_LEAF() MakeLeaf()
#define SC_JSON_ARRAY_ELEMENT(arr, index) (arr)[index]

#include "json_parser_test_suite.inc"

#undef SC_JSON_ARRAY_ELEMENT
#undef SC_JSON_MAKE_JSON_LEAF
#undef SC_JSON_MAKE_JSON
#undef SC_JSON_TYPE
#undef SC_JSON_TEST_SUITE_NAME
