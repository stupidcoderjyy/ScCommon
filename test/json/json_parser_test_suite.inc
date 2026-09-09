// 每个 JSON 库的测试文件通过先定义以下宏、再包含本文件来复用同一套用例：
//   SC_JSON_TEST_SUITE_NAME   测试套件名（如 JsonParserNlohmann）
//   SC_JSON_TYPE              JSON 库类型（如 nlohmann::json）
//   SC_JSON_MAKE_JSON()       构造包含测试数据的 JSON 文档
//   SC_JSON_MAKE_JSON_LEAF()  构造根节点为整数的 JSON 文档
#pragma once
#include <gtest/gtest.h>

#include <cstdint>
#include <string>
#include <string_view>
#include <stdexcept>
#include <utility>
#include <vector>

#include "json/json_parser.h"

using namespace sc::json;

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

struct SC_JSON_CONCAT(ContextException_, SC_JSON_TEST_SUITE_NAME) : std::runtime_error {
    SC_JSON_CONCAT(ContextException_, SC_JSON_TEST_SUITE_NAME)(std::string request_id, std::string path)
        : std::runtime_error("JsonParser: parse error at '" + path + "'"),
          request_id_(std::move(request_id)),
          path_(std::move(path)) {}

    const std::string& RequestId() const noexcept { return request_id_; }
    const std::string& Path() const noexcept { return path_; }

private:
    std::string request_id_;
    std::string path_;
};

struct SC_JSON_CONCAT(ContextErrorHandler_, SC_JSON_TEST_SUITE_NAME) {
    explicit SC_JSON_CONCAT(ContextErrorHandler_, SC_JSON_TEST_SUITE_NAME)(std::string request_id)
        : request_id_(std::move(request_id)) {}

    void OnMissing(std::string_view path) const {
        throw SC_JSON_CONCAT(ContextException_, SC_JSON_TEST_SUITE_NAME)(
                request_id_, std::string(path));
    }

    void OnTypeMismatch(std::string_view path) const {
        throw SC_JSON_CONCAT(ContextException_, SC_JSON_TEST_SUITE_NAME)(
                request_id_, std::string(path));
    }

private:
    std::string request_id_;
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
struct sc::json::Extractor<SC_JSON_TYPE, SC_JSON_CONCAT(Point_, SC_JSON_TEST_SUITE_NAME)> {
    using ValueType = SC_JSON_CONCAT(Point_, SC_JSON_TEST_SUITE_NAME);

    static bool Extract(const SC_JSON_TYPE& json, ValueType& out) {
        if (!JsonTraits<SC_JSON_TYPE>::IsObject(json) ||
            !JsonTraits<SC_JSON_TYPE>::HasKey(json, "x") ||
            !JsonTraits<SC_JSON_TYPE>::HasKey(json, "y")) {
            return false;
        }
        out.x = JsonTraits<SC_JSON_TYPE>::GetInt(JsonTraits<SC_JSON_TYPE>::GetChild(json, "x"));
        out.y = JsonTraits<SC_JSON_TYPE>::GetInt(JsonTraits<SC_JSON_TYPE>::GetChild(json, "y"));
        return true;
    }
};

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

TEST(SC_JSON_TEST_SUITE_NAME, ParseInt64) {
    const SC_JSON_TYPE json = SC_JSON_MAKE_JSON();
    JsonParser parser(json);
    std::int64_t value = 0;
    parser.Parse<JsonInt64, Required>("big", [&](std::int64_t v) { value = v; });
    EXPECT_EQ(9007199254740993LL, value);
}

TEST(SC_JSON_TEST_SUITE_NAME, ParseInt64Negative) {
    const SC_JSON_TYPE json = SC_JSON_MAKE_JSON();
    JsonParser parser(json);
    std::int64_t value = 0;
    parser.Parse<JsonInt64, Required>("big_neg", [&](std::int64_t v) { value = v; });
    EXPECT_EQ(-9007199254740993LL, value);
}

TEST(SC_JSON_TEST_SUITE_NAME, ParseInt64SmallInt) {
    const SC_JSON_TYPE json = SC_JSON_MAKE_JSON();
    JsonParser parser(json);
    std::int64_t value = 0;
    parser.Parse<JsonInt64, Required>("a.b", [&](std::int64_t v) { value = v; });
    EXPECT_EQ(42LL, value);
}

TEST(SC_JSON_TEST_SUITE_NAME, AcquireInt64) {
    const SC_JSON_TYPE json = SC_JSON_MAKE_JSON();
    JsonParser parser(json);
    std::int64_t value = 0;
    parser.Acquire<JsonInt64, Required>("big", value);
    EXPECT_EQ(9007199254740993LL, value);
}

TEST(SC_JSON_TEST_SUITE_NAME, Int64TypeMismatchThrows) {
    const SC_JSON_TYPE json = SC_JSON_MAKE_JSON();
    JsonParser parser(json);
    std::int64_t value = 0;
    EXPECT_THROW((parser.Acquire<JsonInt64, Required>("d", value)), std::runtime_error);
    EXPECT_THROW((parser.Acquire<JsonInt64, Required>("s", value)), std::runtime_error);
}

TEST(SC_JSON_TEST_SUITE_NAME, IntLosslessRangeChecks) {
    const SC_JSON_TYPE json = SC_JSON_MAKE_JSON();
    JsonParser parser(json);
    int value = 0;
    // 超出 int32 范围（上下界）的整数值不能无损转 int，必须按类型不匹配处理。
    EXPECT_THROW((parser.Acquire<JsonInt, Required>("big", value)), std::runtime_error);
    EXPECT_THROW((parser.Acquire<JsonInt, Required>("big_neg", value)), std::runtime_error);
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

TEST(SC_JSON_TEST_SUITE_NAME, OptionalMissingCallsCallbackWithDefault) {
    const SC_JSON_TYPE json = SC_JSON_MAKE_JSON();
    JsonParser parser(json);
    int value = -1;
    parser.Parse<JsonInt, Optional>("no.such.path", [&](int v) { value = v; });
    EXPECT_EQ(0, value);
}

TEST(SC_JSON_TEST_SUITE_NAME, OptionalTypeMismatchCallsCallbackWithDefault) {
    const SC_JSON_TYPE json = SC_JSON_MAKE_JSON();
    JsonParser parser(json);
    int value = -1;
    parser.Parse<JsonInt, Optional>("s", [&](int v) { value = v; });
    EXPECT_EQ(0, value);
}

TEST(SC_JSON_TEST_SUITE_NAME, OptionalArrayIndexOutOfRangeCallsCallbackWithDefault) {
    const SC_JSON_TYPE json = SC_JSON_MAKE_JSON();
    JsonParser parser(json);
    int value = -1;
    parser.Parse<JsonInt, Optional>("list[9]", [&](int v) { value = v; });
    EXPECT_EQ(0, value);
}

TEST(SC_JSON_TEST_SUITE_NAME, OptionalMissingUsesDefault) {
    const SC_JSON_TYPE json = SC_JSON_MAKE_JSON();
    JsonParser parser(json);
    int value = 0;
    parser.Parse<JsonInt>("no.such.path", [&](int v) { value = v; }, 42);
    EXPECT_EQ(42, value);
}

TEST(SC_JSON_TEST_SUITE_NAME, OptionalTypeMismatchUsesDefault) {
    const SC_JSON_TYPE json = SC_JSON_MAKE_JSON();
    JsonParser parser(json);
    int value = 0;
    parser.Parse<JsonInt>("s", [&](int v) { value = v; }, 7);
    EXPECT_EQ(7, value);
}

TEST(SC_JSON_TEST_SUITE_NAME, OptionalStringUsesDefault) {
    const SC_JSON_TYPE json = SC_JSON_MAKE_JSON();
    JsonParser parser(json);
    std::string value;
    parser.Parse<JsonString, Optional>("missing", [&](const std::string& v) { value = v; },
                                       std::string("unknown"));
    EXPECT_EQ(std::string("unknown"), value);
}

TEST(SC_JSON_TEST_SUITE_NAME, DefaultPolicyIsOptional) {
    const SC_JSON_TYPE json = SC_JSON_MAKE_JSON();
    JsonParser parser(json);
    int value = -1;
    parser.Parse<JsonInt>("no.such.path", [&](int v) { value = v; });
    EXPECT_EQ(0, value);
}

TEST(SC_JSON_TEST_SUITE_NAME, DefaultPolicyWithDefaultValue) {
    const SC_JSON_TYPE json = SC_JSON_MAKE_JSON();
    JsonParser parser(json);
    int value = 0;
    parser.Parse<JsonInt>("no.such.path", [&](int v) { value = v; }, 42);
    EXPECT_EQ(42, value);
}

TEST(SC_JSON_TEST_SUITE_NAME, DefaultValuePresentUsesActual) {
    const SC_JSON_TYPE json = SC_JSON_MAKE_JSON();
    JsonParser parser(json);
    int value = 0;
    parser.Parse<JsonInt>("a.b", [&](int v) { value = v; }, 0);
    EXPECT_EQ(42, value);
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
    using ArrayType = JsonArray<SC_JSON_TYPE>::ValueType;
    static_assert(std::is_same_v<ArrayType, std::vector<const SC_JSON_TYPE*>>,
                  "JsonArray must be a zero-copy pointer view");
    bool called = false;
    parser.Parse<JsonArray, Required>("list", [&](const ArrayType& arr) {
        called = true;
        ASSERT_EQ(3u, arr.size());
        const SC_JSON_TYPE& listNode = JsonTraits<SC_JSON_TYPE>::GetChild(json, "list");
        EXPECT_EQ(arr[1], &JsonTraits<SC_JSON_TYPE>::GetArrayElement(listNode, 1));
        int value = 0;
        JsonParser sub(SC_JSON_ARRAY_ELEMENT(arr, 1));
        sub.Parse<JsonInt, Required>("", [&](int v) { value = v; });
        EXPECT_EQ(2, value);
    });
    EXPECT_TRUE(called);
}

TEST(SC_JSON_TEST_SUITE_NAME, AcquireJsonArray) {
    const SC_JSON_TYPE json = SC_JSON_MAKE_JSON();
    JsonParser parser(json);
    std::vector<const SC_JSON_TYPE*> arr;
    parser.Acquire<JsonArray, Required>("list", arr);
    ASSERT_EQ(3u, arr.size());
    EXPECT_EQ(2, JsonTraits<SC_JSON_TYPE>::GetInt(*arr[1]));
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

TEST(SC_JSON_TEST_SUITE_NAME, StatefulErrorHandlerCarriesExternalData) {
    using Exception = SC_JSON_CONCAT(ContextException_, SC_JSON_TEST_SUITE_NAME);
    using Handler = SC_JSON_CONCAT(ContextErrorHandler_, SC_JSON_TEST_SUITE_NAME);

    const SC_JSON_TYPE json = SC_JSON_MAKE_JSON();
    JsonParser<SC_JSON_TYPE, Handler> parser(json, Handler("req-001"));

    EXPECT_THROW((parser.Parse<JsonInt, Required>("no.such.path", [](int) {})), Exception);

    try {
        parser.Parse<JsonInt, Required>("s", [](int) {});
        FAIL() << "expected ContextException";
    } catch (const Exception& e) {
        EXPECT_EQ(std::string("req-001"), e.RequestId());
        EXPECT_EQ(std::string("s"), e.Path());
    }
}

TEST(SC_JSON_TEST_SUITE_NAME, ParseJsonObject) {
    const SC_JSON_TYPE json = SC_JSON_MAKE_JSON();
    JsonParser parser(json);
    bool called = false;
    parser.Parse<JsonObject, Required>("a", [&](const SC_JSON_TYPE* obj) {
        called = true;
        ASSERT_NE(nullptr, obj);
        EXPECT_TRUE(JsonTraits<SC_JSON_TYPE>::IsObject(*obj));
        EXPECT_TRUE(JsonTraits<SC_JSON_TYPE>::HasKey(*obj, "b"));
        EXPECT_EQ(42, JsonTraits<SC_JSON_TYPE>::GetInt(
                          JsonTraits<SC_JSON_TYPE>::GetChild(*obj, "b")));
    });
    EXPECT_TRUE(called);
    EXPECT_THROW((parser.Parse<JsonObject, Required>("s", [](const SC_JSON_TYPE*) {})),
                 std::runtime_error);
}

TEST(SC_JSON_TEST_SUITE_NAME, AcquireJsonObject) {
    const SC_JSON_TYPE json = SC_JSON_MAKE_JSON();
    JsonParser parser(json);
    const SC_JSON_TYPE* obj = nullptr;
    parser.Acquire<JsonObject, Required>("a", obj);
    ASSERT_NE(nullptr, obj);
    EXPECT_TRUE(JsonTraits<SC_JSON_TYPE>::IsObject(*obj));
    EXPECT_TRUE(JsonTraits<SC_JSON_TYPE>::HasKey(*obj, "b"));
    EXPECT_EQ(42, JsonTraits<SC_JSON_TYPE>::GetInt(
                      JsonTraits<SC_JSON_TYPE>::GetChild(*obj, "b")));
}

TEST(SC_JSON_TEST_SUITE_NAME, AcquireAssignsValue) {
    const SC_JSON_TYPE json = SC_JSON_MAKE_JSON();
    JsonParser parser(json);
    int value = 0;
    parser.Acquire<JsonInt, Required>("a.b", value);
    EXPECT_EQ(42, value);
}

TEST(SC_JSON_TEST_SUITE_NAME, AcquireRequiredMissingThrows) {
    const SC_JSON_TYPE json = SC_JSON_MAKE_JSON();
    JsonParser parser(json);
    int value = 0;
    EXPECT_THROW((parser.Acquire<JsonInt, Required>("no.such.path", value)), std::runtime_error);
}

TEST(SC_JSON_TEST_SUITE_NAME, AcquireRequiredTypeMismatchThrows) {
    const SC_JSON_TYPE json = SC_JSON_MAKE_JSON();
    JsonParser parser(json);
    int value = 0;
    EXPECT_THROW((parser.Acquire<JsonInt, Required>("s", value)), std::runtime_error);
}

TEST(SC_JSON_TEST_SUITE_NAME, AcquireOptionalMissingAssignsDefault) {
    const SC_JSON_TYPE json = SC_JSON_MAKE_JSON();
    JsonParser parser(json);
    int value = 123;
    parser.Acquire<JsonInt>("no.such.path", value);
    EXPECT_EQ(0, value);
}

TEST(SC_JSON_TEST_SUITE_NAME, AcquireOptionalTypeMismatchAssignsDefault) {
    const SC_JSON_TYPE json = SC_JSON_MAKE_JSON();
    JsonParser parser(json);
    int value = 123;
    parser.Acquire<JsonInt>("s", value);
    EXPECT_EQ(0, value);
}

TEST(SC_JSON_TEST_SUITE_NAME, AcquireOptionalUsesDefaultValue) {
    const SC_JSON_TYPE json = SC_JSON_MAKE_JSON();
    JsonParser parser(json);
    int value = 0;
    parser.Acquire<JsonInt>("no.such.path", value, 42);
    EXPECT_EQ(42, value);
}

TEST(SC_JSON_TEST_SUITE_NAME, AcquireOptionalTypeMismatchUsesDefaultValue) {
    const SC_JSON_TYPE json = SC_JSON_MAKE_JSON();
    JsonParser parser(json);
    int value = 0;
    parser.Acquire<JsonInt>("s", value, 42);
    EXPECT_EQ(42, value);
}

TEST(SC_JSON_TEST_SUITE_NAME, AcquireDefaultValuePresentUsesActual) {
    const SC_JSON_TYPE json = SC_JSON_MAKE_JSON();
    JsonParser parser(json);
    int value = 0;
    parser.Acquire<JsonInt>("a.b", value, 7);
    EXPECT_EQ(42, value);
}

TEST(SC_JSON_TEST_SUITE_NAME, AcquireOptionalStringUsesDefaultValue) {
    const SC_JSON_TYPE json = SC_JSON_MAKE_JSON();
    JsonParser parser(json);
    std::string value;
    parser.Acquire<JsonString>("missing", value, std::string("unknown"));
    EXPECT_EQ(std::string("unknown"), value);
}

TEST(SC_JSON_TEST_SUITE_NAME, AcquireChained) {
    const SC_JSON_TYPE json = SC_JSON_MAKE_JSON();
    JsonParser parser(json);
    int intValue = 0;
    std::string stringValue;
    bool boolValue = false;
    parser.Acquire<JsonInt, Required>("a.b", intValue)
            .Acquire<JsonString, Required>("s", stringValue)
            .Acquire<JsonBool, Required>("flag", boolValue);
    EXPECT_EQ(42, intValue);
    EXPECT_EQ(std::string("hello"), stringValue);
    EXPECT_TRUE(boolValue);
}

TEST(SC_JSON_TEST_SUITE_NAME, AcquireUsesCustomErrorHandler) {
    using Exception = SC_JSON_CONCAT(ContextException_, SC_JSON_TEST_SUITE_NAME);
    using Handler = SC_JSON_CONCAT(ContextErrorHandler_, SC_JSON_TEST_SUITE_NAME);

    const SC_JSON_TYPE json = SC_JSON_MAKE_JSON();
    JsonParser<SC_JSON_TYPE, Handler> parser(json, Handler("req-002"));
    int value = 0;
    try {
        parser.Acquire<JsonInt, Required>("no.such.path", value);
        FAIL() << "expected ContextException";
    } catch (const Exception& e) {
        EXPECT_EQ(std::string("req-002"), e.RequestId());
        EXPECT_EQ(std::string("no.such.path"), e.Path());
    }
}
