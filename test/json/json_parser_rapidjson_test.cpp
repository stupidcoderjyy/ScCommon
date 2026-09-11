//
// Created by PC on 2026/8/30.
//

#include <rapidjson/document.h>

#include <stdexcept>
#include <string>

namespace {

// rapidjson::Value 的深拷贝内存归分配器管理，使用静态内存池保证
// 拷贝出的 Value 在整个测试期间有效。
rapidjson::MemoryPoolAllocator<>& TestPool() {
    static rapidjson::MemoryPoolAllocator<> pool;
    return pool;
}

rapidjson::Value MakeJson() {
    rapidjson::Document doc;
    doc.Parse(
            R"({"a":{"b":42},"neg":-7,"list":[1,2,3],"s":"hello","d":3.14,"flag":true,"obj_list":[{"id":1},{"id":2}],"point":{"x":10,"y":20},"big":9007199254740993,"big_neg":-9007199254740993})");
    if (doc.HasParseError()) {
        throw std::runtime_error("rapidjson parse error");
    }
    return {doc, TestPool()};
}

rapidjson::Value MakeLeaf() { return rapidjson::Value(42); }

}  // namespace

#define SC_JSON_TEST_SUITE_NAME JsonParserRapidjson
#define SC_JSON_TYPE rapidjson::Value
#define SC_JSON_MAKE_JSON() MakeJson()
#define SC_JSON_MAKE_JSON_LEAF() MakeLeaf()
#include "json_parser_test_suite.inc"

#undef SC_JSON_MAKE_JSON_LEAF
#undef SC_JSON_MAKE_JSON
#undef SC_JSON_TYPE
#undef SC_JSON_TEST_SUITE_NAME
