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
            R"({"a":{"b":42},"neg":-7,"list":[1,2,3],"s":"hello","d":3.14,"flag":true,"nil":null,"zh":"\u4e2d\u6587","zh_raw":"中文","zh_key":{"\u952e":"\u503c"},"mix_res":[{"value":[1.5,2.5]}],"opt_list":[1,null,3],"opt_point":{"x":10,"y":null},"obj_list":[{"id":1},{"id":2}],"point":{"x":10,"y":20},"big":9007199254740993,"big_neg":-9007199254740993})";
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
#include "json_parser_test_suite.inc"

#undef SC_JSON_MAKE_JSON_LEAF
#undef SC_JSON_MAKE_JSON
#undef SC_JSON_TYPE
#undef SC_JSON_TEST_SUITE_NAME
