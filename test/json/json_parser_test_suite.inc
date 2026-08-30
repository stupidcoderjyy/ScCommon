// 每个 JSON 库的测试文件通过先定义以下宏、再包含本文件来复用同一套用例：
//   SC_JSON_TEST_SUITE_NAME   测试套件名（如 JsonParserNlohmann）
//   SC_JSON_TYPE              JSON 库类型（如 nlohmann::json）
//   SC_JSON_MAKE_JSON()       构造包含测试数据的 JSON 文档
//   SC_JSON_MAKE_JSON_LEAF()  构造根节点为整数的 JSON 文档
#pragma once
#include <gtest/gtest.h>

#include <string>
#include <string_view>
#include <vector>

#include "json/json_parser.h"

using namespace sc::common::json;

#define SC_JSON_CONCAT_IMPL(a, b) a##b
#define SC_JSON_CONCAT(a, b) SC_JSON_CONCAT_IMPL(a, b)

struct SC_JSON_CONCAT(Point_, SC_JSON_TEST_SUITE_NAME) {
    int x = 0;
    int y = 0;
};

struct SC_JSON_CONCAT(ErrorHandler_, SC_JSON_TEST_SUITE_NAME) {
    static inline std::vector<std::string> missing;
    static inline std::vector<std::string> mismatch;

    static void OnMissing(std::string_view path) { missing.emplace_back(path); }

    static void OnTypeMismatch(std::string_view path) { mismatch.emplace_back(path); }
};

struct SC_JSON_CONCAT(MemberHandler_, SC_JSON_TEST_SUITE_NAME) {
    int value = 0;

    void onInt(int v) { value = v; }

    void process(const SC_JSON_TYPE& json) {
        JsonParser parser(json);
        parser.Parse<JsonInt, Required>("a.b", [this](int v) { onInt(v); });
    }
};

// 自定义提取器示例：通过特化 Extractor<JsonType, T> 支持自定义对象映射。
template <typename Json>
using SC_JSON_CONCAT(PointExtractor_, SC_JSON_TEST_SUITE_NAME) =
        Extractor<Json, SC_JSON_CONCAT(Point_, SC_JSON_TEST_SUITE_NAME)>;

template <>
struct sc::common::json::Extractor<SC_JSON_TYPE, SC_JSON_CONCAT(Point_, SC_JSON_TEST_SUITE_NAME)> {
    using value_type = SC_JSON_CONCAT(Point_, SC_JSON_TEST_SUITE_NAME);

    static bool Extract(const SC_JSON_TYPE& node, value_type& out) {
        if (!JsonTraits<SC_JSON_TYPE>::IsObject(node) ||
            !JsonTraits<SC_JSON_TYPE>::HasKey(node, "x") ||
            !JsonTraits<SC_JSON_TYPE>::HasKey(node, "y")) {
            return false;
        }
        out.x = JsonTraits<SC_JSON_TYPE>::GetInt(JsonTraits<SC_JSON_TYPE>::GetChild(node, "x"));
        out.y = JsonTraits<SC_JSON_TYPE>::GetInt(JsonTraits<SC_JSON_TYPE>::GetChild(node, "y"));
        return true;
    }
};  // namespace sc::common::json

TEST(SC_JSON_TEST_SUITE_NAME, ParseNestedObject) {
    const SC_JSON_TYPE json = SC_JSON_MAKE_JSON();
    JsonParser parser(json);
    int value = 0;
    parser.Parse<JsonInt, Required>("a.b", [&](int v) { value = v; });
    EXPECT_EQ(42, value);
}

TEST(SC_JSON_TEST_SUITE_NAME, ParseNegativeInteger) {
    const SC_JSON_TYPE json = SC_JSON_MAKE_JSON();
    JsonParser parser(json);
    int value = 0;
    parser.Parse<JsonInt, Required>("neg", [&](int v) { value = v; });
    EXPECT_EQ(-7, value);
}

TEST(SC_JSON_TEST_SUITE_NAME, ParseString) {
    const SC_JSON_TYPE json = SC_JSON_MAKE_JSON();
    JsonParser parser(json);
    std::string value;
    parser.Parse<JsonString, Required>("s", [&](const std::string& v) { value = v; });
    EXPECT_EQ(std::string("hello"), value);
}

TEST(SC_JSON_TEST_SUITE_NAME, ParseDouble) {
    const SC_JSON_TYPE json = SC_JSON_MAKE_JSON();
    JsonParser parser(json);
    double value = 0.0;
    parser.Parse<JsonDouble, Required>("d", [&](double v) { value = v; });
    EXPECT_DOUBLE_EQ(3.14, value);
}

TEST(SC_JSON_TEST_SUITE_NAME, ParseBool) {
    const SC_JSON_TYPE json = SC_JSON_MAKE_JSON();
    JsonParser parser(json);
    bool value = false;
    parser.Parse<JsonBool, Required>("flag", [&](bool v) { value = v; });
    EXPECT_TRUE(value);
}

TEST(SC_JSON_TEST_SUITE_NAME, ParseArrayIndex) {
    const SC_JSON_TYPE json = SC_JSON_MAKE_JSON();
    JsonParser parser(json);
    int value = 0;
    parser.Parse<JsonInt, Required>("list[1]", [&](int v) { value = v; });
    EXPECT_EQ(2, value);
}

TEST(SC_JSON_TEST_SUITE_NAME, ParseNestedArrayIndex) {
    const SC_JSON_TYPE json = SC_JSON_MAKE_JSON();
    JsonParser parser(json);
    int value = 0;
    parser.Parse<JsonInt, Required>("obj_list[1].id", [&](int v) { value = v; });
    EXPECT_EQ(2, value);
}

TEST(SC_JSON_TEST_SUITE_NAME, ParseEmptyPathUsesRoot) {
    const SC_JSON_TYPE json = SC_JSON_MAKE_JSON_LEAF();
    JsonParser parser(json);
    int value = 0;
    parser.Parse<JsonInt, Required>("", [&](int v) { value = v; });
    EXPECT_EQ(42, value);
}

TEST(SC_JSON_TEST_SUITE_NAME, ParseChainedCalls) {
    const SC_JSON_TYPE json = SC_JSON_MAKE_JSON();
    JsonParser parser(json);
    int intValue = 0;
    std::string stringValue;
    bool boolValue = false;
    JsonParser<SC_JSON_TYPE>& ref =
            parser.Parse<JsonInt, Required>("a.b", [&](int v) { intValue = v; })
                    .Parse<JsonString, Required>("s", [&](const std::string& v) { stringValue = v; })
                    .Parse<JsonBool, Required>("flag", [&](bool v) { boolValue = v; });
    EXPECT_EQ(&ref, &parser);
    EXPECT_EQ(42, intValue);
    EXPECT_EQ(std::string("hello"), stringValue);
    EXPECT_TRUE(boolValue);
}

TEST(SC_JSON_TEST_SUITE_NAME, OptionalMissingSkips) {
    const SC_JSON_TYPE json = SC_JSON_MAKE_JSON();
    JsonParser parser(json);
    bool called = false;
    parser.Parse<JsonInt, Optional>("no.such.path", [&](int) { called = true; });
    EXPECT_FALSE(called);
}

TEST(SC_JSON_TEST_SUITE_NAME, OptionalTypeMismatchSkips) {
    const SC_JSON_TYPE json = SC_JSON_MAKE_JSON();
    JsonParser parser(json);
    bool called = false;
    parser.Parse<JsonInt, Optional>("s", [&](int) { called = true; });
    EXPECT_FALSE(called);
}

TEST(SC_JSON_TEST_SUITE_NAME, OptionalArrayIndexOutOfRangeSkips) {
    const SC_JSON_TYPE json = SC_JSON_MAKE_JSON();
    JsonParser parser(json);
    bool called = false;
    parser.Parse<JsonInt, Optional>("list[9]", [&](int) { called = true; });
    EXPECT_FALSE(called);
}

TEST(SC_JSON_TEST_SUITE_NAME, RequiredMissingThrows) {
    const SC_JSON_TYPE json = SC_JSON_MAKE_JSON();
    JsonParser parser(json);
    EXPECT_THROW((parser.Parse<JsonInt, Required>("no.such.path", [](int) {})), std::runtime_error);
}

TEST(SC_JSON_TEST_SUITE_NAME, RequiredTypeMismatchThrows) {
    const SC_JSON_TYPE json = SC_JSON_MAKE_JSON();
    JsonParser parser(json);
    EXPECT_THROW((parser.Parse<JsonInt, Required>("s", [](int) {})), std::runtime_error);
}

TEST(SC_JSON_TEST_SUITE_NAME, ArrayIndexOutOfRangeThrows) {
    const SC_JSON_TYPE json = SC_JSON_MAKE_JSON();
    JsonParser parser(json);
    EXPECT_THROW((parser.Parse<JsonInt, Required>("list[9]", [](int) {})), std::runtime_error);
}

TEST(SC_JSON_TEST_SUITE_NAME, TraversalTypeMismatchThrows) {
    const SC_JSON_TYPE json = SC_JSON_MAKE_JSON();
    JsonParser parser(json);
    EXPECT_THROW((parser.Parse<JsonInt, Required>("list.b", [](int) {})), std::runtime_error);
}

TEST(SC_JSON_TEST_SUITE_NAME, ArrayExtractor) {
    const SC_JSON_TYPE json = SC_JSON_MAKE_JSON();
    JsonParser parser(json);
    using ArrayType = JsonArray<SC_JSON_TYPE>::value_type;
    bool called = false;
    parser.Parse<JsonArray, Required>("list", [&](const ArrayType& arr) {
        called = true;
        ASSERT_EQ(3u, arr.size());
        int value = 0;
        JsonParser sub(SC_JSON_ARRAY_ELEMENT(arr, 1));
        sub.Parse<JsonInt, Required>("", [&](int v) { value = v; });
        EXPECT_EQ(2, value);
    });
    EXPECT_TRUE(called);
}

TEST(SC_JSON_TEST_SUITE_NAME, CustomExtractor) {
    using Point = SC_JSON_CONCAT(Point_, SC_JSON_TEST_SUITE_NAME);
    const SC_JSON_TYPE json = SC_JSON_MAKE_JSON();
    JsonParser parser(json);
    Point point;
    parser.Parse<SC_JSON_CONCAT(PointExtractor_, SC_JSON_TEST_SUITE_NAME), Required>(
            "point", [&](const Point& p) { point = p; });
    EXPECT_EQ(10, point.x);
    EXPECT_EQ(20, point.y);
}

TEST(SC_JSON_TEST_SUITE_NAME, MemberFunctionBinding) {
    using MemberHandler = SC_JSON_CONCAT(MemberHandler_, SC_JSON_TEST_SUITE_NAME);
    const SC_JSON_TYPE json = SC_JSON_MAKE_JSON();
    JsonParser parser(json);
    MemberHandler handler;
    parser.Parse<JsonInt, Required>(
            "a.b", [ObjectPtr = &handler]<typename T0>(T0 && PH1) { ObjectPtr->onInt(std::forward<T0>(PH1)); });
    EXPECT_EQ(42, handler.value);
}

TEST(SC_JSON_TEST_SUITE_NAME, LambdaCapturingThis) {
    using MemberHandler = SC_JSON_CONCAT(MemberHandler_, SC_JSON_TEST_SUITE_NAME);
    const SC_JSON_TYPE json = SC_JSON_MAKE_JSON();
    MemberHandler handler;
    handler.process(json);
    EXPECT_EQ(42, handler.value);
}

TEST(SC_JSON_TEST_SUITE_NAME, CustomErrorHandler) {
    using ErrorHandler = SC_JSON_CONCAT(ErrorHandler_, SC_JSON_TEST_SUITE_NAME);
    ErrorHandler::missing.clear();
    ErrorHandler::mismatch.clear();

    const SC_JSON_TYPE json = SC_JSON_MAKE_JSON();
    JsonParser<SC_JSON_TYPE, ErrorHandler> parser(json);
    bool called = false;
    parser.Parse<JsonInt, Required>("no.such.path", [&](int) { called = true; });
    EXPECT_FALSE(called);
    ASSERT_EQ(1u, ErrorHandler::missing.size());
    EXPECT_EQ(std::string("no.such.path"), ErrorHandler::missing.front());

    parser.Parse<JsonInt, Required>("s", [&](int) { called = true; });
    ASSERT_EQ(1u, ErrorHandler::mismatch.size());
    EXPECT_EQ(std::string("s"), ErrorHandler::mismatch.front());

    int value = 0;
    parser.Parse<JsonInt, Required>("a.b", [&](int v) { value = v; });
    EXPECT_EQ(42, value);
}
