// 每个 JSON 库的测试文件通过先定义以下宏、再包含本文件来复用同一套用例：
//   SC_JSON_TEST_SUITE_NAME   测试套件名（如 JsonParserNlohmann）
//   SC_JSON_TYPE              JSON 库类型（如 nlohmann::json）
//   SC_JSON_MAKE_JSON()       构造包含测试数据的 JSON 文档
//   SC_JSON_MAKE_JSON_LEAF()  构造根节点为整数的 JSON 文档
#pragma once
#include <gtest/gtest.h>

#include <exception>
#include <cstdint>
#include <map>
#include <ranges>
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
    static inline std::vector<std::string> resolve_errors;
    static inline std::vector<std::string> mismatch;

    static void OnPathResolveError(const std::exception& e) {
        resolve_errors.emplace_back(e.what());
    }

    static void OnTypeMismatch(std::string_view path) { mismatch.emplace_back(path); }
};

struct SC_JSON_CONCAT(ContextException_, SC_JSON_TEST_SUITE_NAME) : std::runtime_error {
    SC_JSON_CONCAT(ContextException_, SC_JSON_TEST_SUITE_NAME)(std::string request_id,
                                                               std::string message)
        : std::runtime_error(message),
          request_id_(std::move(request_id)),
          message_(std::move(message)) {}

    const std::string& RequestId() const noexcept { return request_id_; }
    const std::string& Message() const noexcept { return message_; }

private:
    std::string request_id_;
    std::string message_;
};

struct SC_JSON_CONCAT(ContextErrorHandler_, SC_JSON_TEST_SUITE_NAME) {
    explicit SC_JSON_CONCAT(ContextErrorHandler_, SC_JSON_TEST_SUITE_NAME)(std::string request_id)
        : request_id_(std::move(request_id)) {}

    void OnPathResolveError(const std::exception& e) const {
        throw SC_JSON_CONCAT(ContextException_, SC_JSON_TEST_SUITE_NAME)(
                request_id_, e.what());
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
        parser.Acquire<JsonInt, Required>("a.b"_path, value);
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
        if (!JsonTraits<SC_JSON_TYPE>::IsObject(json)) {
            return false;
        }
        const SC_JSON_TYPE* x = JsonTraits<SC_JSON_TYPE>::TryGetChild(json, "x");
        const SC_JSON_TYPE* y = JsonTraits<SC_JSON_TYPE>::TryGetChild(json, "y");
        if (!x || !y) {
            return false;
        }
        out.x = JsonTraits<SC_JSON_TYPE>::GetInt(*x);
        out.y = JsonTraits<SC_JSON_TYPE>::GetInt(*y);
        return true;
    }
};

TEST(SC_JSON_TEST_SUITE_NAME, AcquireNestedObject) {
    const SC_JSON_TYPE json = SC_JSON_MAKE_JSON();
    JsonParser parser(json);
    int value = 0;
    parser.Acquire<JsonInt, Required>("a.b"_path, value);
    EXPECT_EQ(42, value);
}

TEST(SC_JSON_TEST_SUITE_NAME, AcquireNegativeInteger) {
    const SC_JSON_TYPE json = SC_JSON_MAKE_JSON();
    JsonParser parser(json);
    int value = 0;
    parser.Acquire<JsonInt, Required>("neg"_path, value);
    EXPECT_EQ(-7, value);
}

TEST(SC_JSON_TEST_SUITE_NAME, AcquireInt64Negative) {
    const SC_JSON_TYPE json = SC_JSON_MAKE_JSON();
    JsonParser parser(json);
    std::int64_t value = 0;
    parser.Acquire<JsonInt64, Required>("big_neg"_path, value);
    EXPECT_EQ(-9007199254740993LL, value);
}

TEST(SC_JSON_TEST_SUITE_NAME, AcquireInt64SmallInt) {
    const SC_JSON_TYPE json = SC_JSON_MAKE_JSON();
    JsonParser parser(json);
    std::int64_t value = 0;
    parser.Acquire<JsonInt64, Required>("a.b"_path, value);
    EXPECT_EQ(42LL, value);
}

TEST(SC_JSON_TEST_SUITE_NAME, AcquireInt64) {
    const SC_JSON_TYPE json = SC_JSON_MAKE_JSON();
    JsonParser parser(json);
    std::int64_t value = 0;
    parser.Acquire<JsonInt64, Required>("big"_path, value);
    EXPECT_EQ(9007199254740993LL, value);
}

TEST(SC_JSON_TEST_SUITE_NAME, Int64TypeMismatchThrows) {
    const SC_JSON_TYPE json = SC_JSON_MAKE_JSON();
    JsonParser parser(json);
    std::int64_t value = 0;
    EXPECT_THROW((parser.Acquire<JsonInt64, Required>("d"_path, value)), std::runtime_error);
    EXPECT_THROW((parser.Acquire<JsonInt64, Required>("s"_path, value)), std::runtime_error);
}

TEST(SC_JSON_TEST_SUITE_NAME, IntLosslessRangeChecks) {
    const SC_JSON_TYPE json = SC_JSON_MAKE_JSON();
    JsonParser parser(json);
    int value = 0;
    // 超出 int32 范围（上下界）的整数值不能无损转 int，必须按类型不匹配处理。
    EXPECT_THROW((parser.Acquire<JsonInt, Required>("big"_path, value)), std::runtime_error);
    EXPECT_THROW((parser.Acquire<JsonInt, Required>("big_neg"_path, value)), std::runtime_error);
}

TEST(SC_JSON_TEST_SUITE_NAME, AcquireString) {
    const SC_JSON_TYPE json = SC_JSON_MAKE_JSON();
    JsonParser parser(json);
    std::string value;
    parser.Acquire<JsonString, Required>("s"_path, value);
    EXPECT_EQ(std::string("hello"), value);
}

TEST(SC_JSON_TEST_SUITE_NAME, AcquireDouble) {
    const SC_JSON_TYPE json = SC_JSON_MAKE_JSON();
    JsonParser parser(json);
    double value = 0.0;
    parser.Acquire<JsonDouble, Required>("d"_path, value);
    EXPECT_DOUBLE_EQ(3.14, value);
}

TEST(SC_JSON_TEST_SUITE_NAME, AcquireBool) {
    const SC_JSON_TYPE json = SC_JSON_MAKE_JSON();
    JsonParser parser(json);
    bool value = false;
    parser.Acquire<JsonBool, Required>("flag"_path, value);
    EXPECT_TRUE(value);
}

TEST(SC_JSON_TEST_SUITE_NAME, AcquireArrayIndex) {
    const SC_JSON_TYPE json = SC_JSON_MAKE_JSON();
    JsonParser parser(json);
    int value = 0;
    parser.Acquire<JsonInt, Required>("list[1]"_path, value);
    EXPECT_EQ(2, value);
}

TEST(SC_JSON_TEST_SUITE_NAME, AcquireNestedArrayIndex) {
    const SC_JSON_TYPE json = SC_JSON_MAKE_JSON();
    JsonParser parser(json);
    int value = 0;
    parser.Acquire<JsonInt, Required>("obj_list[1].id"_path, value);
    EXPECT_EQ(2, value);
}

TEST(SC_JSON_TEST_SUITE_NAME, AcquireEmptyPathUsesRoot) {
    const SC_JSON_TYPE json = SC_JSON_MAKE_JSON_LEAF();
    JsonParser parser(json);
    int value = 0;
    parser.Acquire<JsonInt, Required>(""_path, value);
    EXPECT_EQ(42, value);
}

TEST(SC_JSON_TEST_SUITE_NAME, AcquireChainedCalls) {
    const SC_JSON_TYPE json = SC_JSON_MAKE_JSON();
    JsonParser parser(json);
    int intValue = 0;
    std::string stringValue;
    bool boolValue = false;
    JsonParser<SC_JSON_TYPE>& ref = parser.Acquire<JsonInt, Required>("a.b"_path, intValue)
                                            .Acquire<JsonString, Required>("s"_path, stringValue)
                                            .Acquire<JsonBool, Required>("flag"_path, boolValue);
    EXPECT_EQ(&ref, &parser);
    EXPECT_EQ(42, intValue);
    EXPECT_EQ(std::string("hello"), stringValue);
    EXPECT_TRUE(boolValue);
}

TEST(SC_JSON_TEST_SUITE_NAME, OptionalArrayIndexOutOfRangeWritesDefault) {
    const SC_JSON_TYPE json = SC_JSON_MAKE_JSON();
    JsonParser parser(json);
    int value = -1;
    parser.Acquire<JsonInt, Optional>("list[9]"_path, value);
    EXPECT_EQ(0, value);
}

TEST(SC_JSON_TEST_SUITE_NAME, AcquireArrayIndexOutOfRangeThrows) {
    const SC_JSON_TYPE json = SC_JSON_MAKE_JSON();
    JsonParser parser(json);
    int value = 0;
    EXPECT_THROW((parser.Acquire<JsonInt, Required>("list[9]"_path, value)), std::runtime_error);
}

TEST(SC_JSON_TEST_SUITE_NAME, AcquireTraversalTypeMismatchThrows) {
    const SC_JSON_TYPE json = SC_JSON_MAKE_JSON();
    JsonParser parser(json);
    int value = 0;
    EXPECT_THROW((parser.Acquire<JsonInt, Required>("list.b"_path, value)), std::runtime_error);
}

TEST(SC_JSON_TEST_SUITE_NAME, ViewArrayZeroCopy) {
    const SC_JSON_TYPE json = SC_JSON_MAKE_JSON();
    JsonParser parser(json);
    auto range = parser.ViewArray<Required>("list"_path);
    ASSERT_EQ(3u, range.size());
    const SC_JSON_TYPE* listNode = JsonTraits<SC_JSON_TYPE>::TryGetChild(json, "list");
    ASSERT_NE(nullptr, listNode);
    auto second = *(range.begin() + 1);
    // 元素零拷贝：直接指向底层 Json 节点
    EXPECT_EQ(&second.json(), &JsonTraits<SC_JSON_TYPE>::GetArrayElement(*listNode, 1));
    int value = 0;
    second.Acquire<JsonInt>(""_path, value);
    EXPECT_EQ(2, value);
    // 也可以基于该节点构造子解析器
    JsonParser sub(second.json());
    sub.Acquire<JsonInt, Required>(""_path, value);
    EXPECT_EQ(2, value);
}

TEST(SC_JSON_TEST_SUITE_NAME, CustomExtractor) {
    using Point = SC_JSON_CONCAT(Point_, SC_JSON_TEST_SUITE_NAME);
    const SC_JSON_TYPE json = SC_JSON_MAKE_JSON();
    JsonParser parser(json);
    Point point;
    parser.Acquire<SC_JSON_CONCAT(PointExtractor_, SC_JSON_TEST_SUITE_NAME), Required>("point"_path, point);
    EXPECT_EQ(10, point.x);
    EXPECT_EQ(20, point.y);
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
    ErrorHandler::resolve_errors.clear();
    ErrorHandler::mismatch.clear();

    const SC_JSON_TYPE json = SC_JSON_MAKE_JSON();
    JsonParser<SC_JSON_TYPE, ErrorHandler> parser(json);
    int value = -1;
    // 路径解析失败 → OnPathResolveError；handler 返回时 out 写入默认值
    parser.Acquire<JsonInt, Required>("no.such.path"_path, value, -1);
    EXPECT_EQ(-1, value);
    ASSERT_EQ(1u, ErrorHandler::resolve_errors.size());
    // 错误消息中的路径不得带前导点
    EXPECT_NE(std::string::npos,
              ErrorHandler::resolve_errors.front().find("path 'no.such.path'"));

    // 节点存在但类型不符 → OnTypeMismatch
    parser.Acquire<JsonInt, Required>("s"_path, value, -1);
    EXPECT_EQ(-1, value);
    ASSERT_EQ(1u, ErrorHandler::mismatch.size());
    EXPECT_EQ(std::string("s"), ErrorHandler::mismatch.front());

    parser.Acquire<JsonInt, Required>("a.b"_path, value);
    EXPECT_EQ(42, value);
}

TEST(SC_JSON_TEST_SUITE_NAME, StatefulErrorHandlerCarriesExternalData) {
    using Exception = SC_JSON_CONCAT(ContextException_, SC_JSON_TEST_SUITE_NAME);
    using Handler = SC_JSON_CONCAT(ContextErrorHandler_, SC_JSON_TEST_SUITE_NAME);

    const SC_JSON_TYPE json = SC_JSON_MAKE_JSON();
    JsonParser<SC_JSON_TYPE, Handler> parser(json, Handler("req-001"));

    int value = 0;
    EXPECT_THROW((parser.Acquire<JsonInt, Required>("no.such.path"_path, value)), Exception);

    try {
        parser.Acquire<JsonInt, Required>("s"_path, value);
        FAIL() << "expected ContextException";
    } catch (const Exception& e) {
        EXPECT_EQ(std::string("req-001"), e.RequestId());
        EXPECT_EQ(std::string("s"), e.Message());
    }
}

TEST(SC_JSON_TEST_SUITE_NAME, ViewRequiredObject) {
    const SC_JSON_TYPE json = SC_JSON_MAKE_JSON();
    JsonParser parser(json);
    auto view = parser.View<Required>("a"_path);
    ASSERT_TRUE(view.valid());
    EXPECT_TRUE(JsonTraits<SC_JSON_TYPE>::IsObject(view.json()));
    const SC_JSON_TYPE* b = JsonTraits<SC_JSON_TYPE>::TryGetChild(view.json(), "b");
    ASSERT_NE(nullptr, b);
    EXPECT_EQ(42, JsonTraits<SC_JSON_TYPE>::GetInt(*b));
    // 相对 view 解析
    int value = 0;
    view.Acquire<JsonInt>("b"_path, value);
    EXPECT_EQ(42, value);
}

TEST(SC_JSON_TEST_SUITE_NAME, AcquireAssignsValue) {
    const SC_JSON_TYPE json = SC_JSON_MAKE_JSON();
    JsonParser parser(json);
    int value = 0;
    parser.Acquire<JsonInt, Required>("a.b"_path, value);
    EXPECT_EQ(42, value);
}

TEST(SC_JSON_TEST_SUITE_NAME, AcquireRequiredMissingThrows) {
    const SC_JSON_TYPE json = SC_JSON_MAKE_JSON();
    JsonParser parser(json);
    int value = 0;
    EXPECT_THROW((parser.Acquire<JsonInt, Required>("no.such.path"_path, value)), std::runtime_error);
}

TEST(SC_JSON_TEST_SUITE_NAME, AcquireRequiredTypeMismatchThrows) {
    const SC_JSON_TYPE json = SC_JSON_MAKE_JSON();
    JsonParser parser(json);
    int value = 0;
    EXPECT_THROW((parser.Acquire<JsonInt, Required>("s"_path, value)), std::runtime_error);
}

TEST(SC_JSON_TEST_SUITE_NAME, AcquireOptionalMissingAssignsDefault) {
    const SC_JSON_TYPE json = SC_JSON_MAKE_JSON();
    JsonParser parser(json);
    int value = 123;
    parser.Acquire<JsonInt>("no.such.path"_path, value);
    EXPECT_EQ(0, value);
}

TEST(SC_JSON_TEST_SUITE_NAME, AcquireOptionalTypeMismatchAssignsDefault) {
    const SC_JSON_TYPE json = SC_JSON_MAKE_JSON();
    JsonParser parser(json);
    int value = 123;
    parser.Acquire<JsonInt>("s"_path, value);
    EXPECT_EQ(0, value);
}

TEST(SC_JSON_TEST_SUITE_NAME, AcquireOptionalUsesDefaultValue) {
    const SC_JSON_TYPE json = SC_JSON_MAKE_JSON();
    JsonParser parser(json);
    int value = 0;
    parser.Acquire<JsonInt>("no.such.path"_path, value, 42);
    EXPECT_EQ(42, value);
}

TEST(SC_JSON_TEST_SUITE_NAME, AcquireOptionalTypeMismatchUsesDefaultValue) {
    const SC_JSON_TYPE json = SC_JSON_MAKE_JSON();
    JsonParser parser(json);
    int value = 0;
    parser.Acquire<JsonInt>("s"_path, value, 42);
    EXPECT_EQ(42, value);
}

TEST(SC_JSON_TEST_SUITE_NAME, AcquireDefaultValuePresentUsesActual) {
    const SC_JSON_TYPE json = SC_JSON_MAKE_JSON();
    JsonParser parser(json);
    int value = 0;
    parser.Acquire<JsonInt>("a.b"_path, value, 7);
    EXPECT_EQ(42, value);
}

TEST(SC_JSON_TEST_SUITE_NAME, AcquireOptionalStringUsesDefaultValue) {
    const SC_JSON_TYPE json = SC_JSON_MAKE_JSON();
    JsonParser parser(json);
    std::string value;
    parser.Acquire<JsonString>("missing"_path, value, std::string("unknown"));
    EXPECT_EQ(std::string("unknown"), value);
}

TEST(SC_JSON_TEST_SUITE_NAME, AcquireMultipleFields) {
    const SC_JSON_TYPE json = SC_JSON_MAKE_JSON();
    JsonParser parser(json);
    int intValue = 0;
    std::string stringValue;
    bool boolValue = false;
    parser.Acquire<JsonInt, Required>("a.b"_path, intValue);
    parser.Acquire<JsonString, Required>("s"_path, stringValue);
    parser.Acquire<JsonBool, Required>("flag"_path, boolValue);
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
        parser.Acquire<JsonInt, Required>("no.such.path"_path, value);
        FAIL() << "expected ContextException";
    } catch (const Exception& e) {
        EXPECT_EQ(std::string("req-002"), e.RequestId());
        EXPECT_NE(std::string::npos, e.Message().find("no.such.path"));
    }
}

TEST(SC_JSON_TEST_SUITE_NAME, AcquireArrayInts) {
    const SC_JSON_TYPE json = SC_JSON_MAKE_JSON();
    JsonParser parser(json);
    std::vector<int> arr;
    parser.AcquireArray<JsonInt, Required>("list"_path, arr);
    ASSERT_EQ(3u, arr.size());
    EXPECT_EQ(1, arr[0]);
    EXPECT_EQ(2, arr[1]);
    EXPECT_EQ(3, arr[2]);
}

TEST(SC_JSON_TEST_SUITE_NAME, AcquireArrayMissingOptional) {
    const SC_JSON_TYPE json = SC_JSON_MAKE_JSON();
    JsonParser parser(json);
    std::vector<int> arr = {9};
    parser.AcquireArray<JsonInt>("no.such.path"_path, arr);
    EXPECT_TRUE(arr.empty());
}

TEST(SC_JSON_TEST_SUITE_NAME, AcquireArrayMissingRequiredThrows) {
    const SC_JSON_TYPE json = SC_JSON_MAKE_JSON();
    JsonParser parser(json);
    std::vector<int> arr;
    EXPECT_THROW((parser.AcquireArray<JsonInt, Required>("no.such.path"_path, arr)), std::runtime_error);
}

TEST(SC_JSON_TEST_SUITE_NAME, AcquireArrayElementMismatchPath) {
    using ErrorHandler = SC_JSON_CONCAT(ErrorHandler_, SC_JSON_TEST_SUITE_NAME);
    ErrorHandler::mismatch.clear();

    const SC_JSON_TYPE json = SC_JSON_MAKE_JSON();
    JsonParser<SC_JSON_TYPE, ErrorHandler> parser(json);
    std::vector<int> arr;
    parser.AcquireArray<JsonInt, Required>("obj_list"_path, arr);
    ASSERT_EQ(1u, ErrorHandler::mismatch.size());
    EXPECT_EQ(std::string("obj_list[0]"), ErrorHandler::mismatch.front());
}

TEST(SC_JSON_TEST_SUITE_NAME, AcquireMapInts) {
    const SC_JSON_TYPE json = SC_JSON_MAKE_JSON();
    JsonParser parser(json);
    std::map<std::string, int> map;
    parser.AcquireMap<JsonInt, Required>("a"_path, map);
    ASSERT_EQ(1u, map.size());
    EXPECT_EQ(42, map["b"]);
}

TEST(SC_JSON_TEST_SUITE_NAME, AcquireMapMissingOptional) {
    const SC_JSON_TYPE json = SC_JSON_MAKE_JSON();
    JsonParser parser(json);
    std::map<std::string, int> map;
    parser.AcquireMap<JsonInt>("no.such.path"_path, map);
    EXPECT_TRUE(map.empty());
}

TEST(SC_JSON_TEST_SUITE_NAME, AcquireMapValueMismatchPath) {
    using ErrorHandler = SC_JSON_CONCAT(ErrorHandler_, SC_JSON_TEST_SUITE_NAME);
    ErrorHandler::mismatch.clear();

    const SC_JSON_TYPE json = SC_JSON_MAKE_JSON();
    JsonParser<SC_JSON_TYPE, ErrorHandler> parser(json);
    std::map<std::string, std::string> map;
    parser.AcquireMap<JsonString, Required>("a"_path, map);
    ASSERT_EQ(1u, ErrorHandler::mismatch.size());
    EXPECT_EQ(std::string("a.b"), ErrorHandler::mismatch.front());
}

TEST(SC_JSON_TEST_SUITE_NAME, ConstructFromString) {
    JsonParser<SC_JSON_TYPE> parser(std::string_view(R"({"a":{"b":42}})"));
    int value = 0;
    parser.Acquire<JsonInt, Required>("a.b"_path, value);
    EXPECT_EQ(42, value);
}

TEST(SC_JSON_TEST_SUITE_NAME, ConstructFromInvalidStringThrows) {
    EXPECT_THROW((JsonParser<SC_JSON_TYPE>(std::string_view("{oops"))), std::runtime_error);
}

TEST(SC_JSON_TEST_SUITE_NAME, ViewRequiredResolvesRelativePath) {
    const SC_JSON_TYPE json = SC_JSON_MAKE_JSON();
    JsonParser parser(json);
    auto point = parser.View<Required>("point"_path);
    EXPECT_TRUE(point.valid());
    int x = 0;
    point.Acquire<JsonInt>("x"_path, x);
    EXPECT_EQ(10, x);
    // parser 是无状态的：路径仍相对根节点解析
    int y = 0;
    parser.Acquire<JsonInt, Required>("point.y"_path, y);
    EXPECT_EQ(20, y);
}

TEST(SC_JSON_TEST_SUITE_NAME, ViewNestedRelativePaths) {
    const SC_JSON_TYPE json = SC_JSON_MAKE_JSON();
    JsonParser parser(json);
    auto list = parser.View<Required>("obj_list"_path);
    auto second = list.View<Required>("[1]"_path);
    int id = 0;
    second.Acquire<JsonInt>("id"_path, id);
    EXPECT_EQ(2, id);
    // 兄弟 view 互不影响（无需 RAII 回滚）
    auto first = list.View<Required>("[0]"_path);
    first.Acquire<JsonInt>("id"_path, id);
    EXPECT_EQ(1, id);
}

// ---------------- View 层 ----------------

namespace {

using ScJsonView = JsonView<SC_JSON_TYPE, DefaultJsonParserErrorHandler>;
using ScJsonArrayView = JsonArrayView<SC_JSON_TYPE, DefaultJsonParserErrorHandler>;
using ScJsonObjectView = JsonObjectView<SC_JSON_TYPE, DefaultJsonParserErrorHandler>;

// 编译期检查（设计文档 8.1）
static_assert(std::ranges::input_range<ScJsonArrayView>);
static_assert(std::ranges::sized_range<ScJsonArrayView>);
static_assert(std::ranges::random_access_range<ScJsonArrayView>);
static_assert(std::ranges::forward_range<ScJsonObjectView>);
static_assert(sizeof(ScJsonView) == 2 * sizeof(void*));
static_assert(sizeof(ScJsonArrayView) <= 3 * sizeof(void*));

}  // namespace

TEST(SC_JSON_TEST_SUITE_NAME, ViewValidOnExistingPath) {
    const SC_JSON_TYPE json = SC_JSON_MAKE_JSON();
    JsonParser parser(json);
    auto view = parser.View("point"_path);
    EXPECT_TRUE(view.valid());
    EXPECT_TRUE(static_cast<bool>(view));
    EXPECT_TRUE(JsonTraits<SC_JSON_TYPE>::IsObject(view.json()));
    int x = 0;
    view.Acquire<JsonInt>("x"_path, x);
    EXPECT_EQ(10, x);
}

TEST(SC_JSON_TEST_SUITE_NAME, ViewEmptyOnMissingPathOptional) {
    const SC_JSON_TYPE json = SC_JSON_MAKE_JSON();
    JsonParser parser(json);
    auto view = parser.View("no.such.path"_path);
    EXPECT_FALSE(view.valid());
    EXPECT_FALSE(static_cast<bool>(view));
}

TEST(SC_JSON_TEST_SUITE_NAME, AcquireOnInvalidViewWritesDefault) {
    const SC_JSON_TYPE json = SC_JSON_MAKE_JSON();
    JsonParser parser(json);
    auto invalid = parser.View("no.such.path"_path);
    int optional_value = 7;
    invalid.Acquire<JsonInt>("a.b"_path, optional_value);
    EXPECT_EQ(0, optional_value);
    // Required 在无效 view 上同样不抛异常，写默认值
    int required_value = 7;
    invalid.Acquire<JsonInt, Required>("a.b"_path, required_value);
    EXPECT_EQ(0, required_value);
}

TEST(SC_JSON_TEST_SUITE_NAME, ViewRequiredThrowsOnMissing) {
    const SC_JSON_TYPE json = SC_JSON_MAKE_JSON();
    JsonParser parser(json);
    EXPECT_THROW((void)parser.View<Required>("no.such.path"_path), std::runtime_error);
}

TEST(SC_JSON_TEST_SUITE_NAME, ViewNestedAndEmptyPath) {
    const SC_JSON_TYPE json = SC_JSON_MAKE_JSON();
    JsonParser parser(json);
    auto a = parser.View("a"_path);
    auto b = a.View("b"_path);
    EXPECT_TRUE(b.valid());
    int value = 0;
    b.Acquire<JsonInt>(""_path, value);  // 空路径 = 当前节点
    EXPECT_EQ(42, value);
}

TEST(SC_JSON_TEST_SUITE_NAME, ViewNestedRequiredPropagatesPolicy) {
    const SC_JSON_TYPE json = SC_JSON_MAKE_JSON();
    JsonParser parser(json);
    auto a = parser.View("a"_path);
    EXPECT_THROW((void)a.View<Required>("no.such"_path), std::runtime_error);
    // 无效 view 上链式调用不再抛异常，返回空 view
    auto invalid = parser.View("no.such"_path);
    EXPECT_FALSE(invalid.View<Required>("b"_path).valid());
}

TEST(SC_JSON_TEST_SUITE_NAME, ViewArrayIterateAll) {
    const SC_JSON_TYPE json = SC_JSON_MAKE_JSON();
    JsonParser parser(json);
    auto range = parser.ViewArray("list"_path);
    EXPECT_FALSE(range.empty());
    EXPECT_EQ(3u, range.size());
    std::vector<int> values;
    for (auto&& element : range) {
        int value = 0;
        element.Acquire<JsonInt>(""_path, value);
        values.push_back(value);
    }
    EXPECT_EQ((std::vector<int>{1, 2, 3}), values);
}

TEST(SC_JSON_TEST_SUITE_NAME, ViewArrayEmptyOnMissingOptional) {
    const SC_JSON_TYPE json = SC_JSON_MAKE_JSON();
    JsonParser parser(json);
    auto range = parser.ViewArray("no.such"_path);
    EXPECT_TRUE(range.empty());
    EXPECT_EQ(0u, range.size());
    EXPECT_EQ(range.begin(), range.end());
    int visited = 0;
    for (auto&& element : range) {
        (void)element;
        ++visited;
    }
    EXPECT_EQ(0, visited);
}

TEST(SC_JSON_TEST_SUITE_NAME, ViewArrayRequiredThrowsOnMissingOrTypeMismatch) {
    const SC_JSON_TYPE json = SC_JSON_MAKE_JSON();
    JsonParser parser(json);
    EXPECT_THROW((void)parser.ViewArray<Required>("no.such"_path), std::runtime_error);
    EXPECT_THROW((void)parser.ViewArray<Required>("point"_path), std::runtime_error);
}

TEST(SC_JSON_TEST_SUITE_NAME, ViewArrayComposeWithRanges) {
    const SC_JSON_TYPE json = SC_JSON_MAKE_JSON();
    JsonParser parser(json);

    std::vector<int> ids;
    auto id_view = parser.ViewArray("obj_list"_path) | std::views::transform([](auto&& element) {
                       int id = 0;
                       element.Acquire("id"_path, id);  // 由 out 类型推导，无需 .template
                       return id;
                   });
    for (int id : id_view) {
        ids.push_back(id);
    }
    EXPECT_EQ((std::vector<int>{1, 2}), ids);

    int matched = 0;
    auto filtered = parser.ViewArray("obj_list"_path) | std::views::filter([](auto&& element) {
                        int id = 0;
                        element.Acquire("id"_path, id);  // 同上
                        return id == 2;
                    });
    for (auto&& element : filtered) {
        int id = 0;
        element.Acquire<JsonInt>("id"_path, id);
        EXPECT_EQ(2, id);
        ++matched;
    }
    EXPECT_EQ(1, matched);
}

TEST(SC_JSON_TEST_SUITE_NAME, ViewObjectStructuredBinding) {
    const SC_JSON_TYPE json = SC_JSON_MAKE_JSON();
    JsonParser parser(json);
    std::map<std::string, int> values;
    for (auto&& [key, value] : parser.ViewObject("point"_path)) {
        int number = 0;
        value.Acquire<JsonInt>(""_path, number);
        values.emplace(std::string(key), number);
    }
    EXPECT_EQ(2u, values.size());
    EXPECT_EQ(10, values["x"]);
    EXPECT_EQ(20, values["y"]);
}

TEST(SC_JSON_TEST_SUITE_NAME, ViewObjectEmptyAndRequired) {
    const SC_JSON_TYPE json = SC_JSON_MAKE_JSON();
    JsonParser parser(json);
    auto range = parser.ViewObject("no.such"_path);
    EXPECT_TRUE(range.empty());
    EXPECT_EQ(range.begin(), range.end());
    EXPECT_THROW((void)parser.ViewObject<Required>("no.such"_path), std::runtime_error);
    EXPECT_THROW((void)parser.ViewObject<Required>("list"_path), std::runtime_error);
}

// ---------------- 运行期路径（RuntimePath）----------------

TEST(SC_JSON_TEST_SUITE_NAME, RuntimePathResolvesDynamicPath) {
    const SC_JSON_TYPE json = SC_JSON_MAKE_JSON();
    JsonParser parser(json);
    const std::string prefix = "a";
    const RuntimePath path(prefix + ".b");  // 运行期拼接的路径

    int value = 0;
    parser.Acquire(path, value);  // 推导写法
    EXPECT_EQ(42, value);

    int explicit_value = 0;
    parser.Acquire<JsonInt, Required>(path, explicit_value);  // 显式提取器写法
    EXPECT_EQ(42, explicit_value);
}

TEST(SC_JSON_TEST_SUITE_NAME, RuntimePathTokenizesLikeCompileTime) {
    const RuntimePath path("obj_list[1].id");
    // 与 "obj_list[1].id"_path 产出同样的 token 序列
    ASSERT_EQ(3u, path.Size());
    EXPECT_EQ(std::string_view("obj_list"), path[0]);
    EXPECT_EQ(std::string_view("[1]"), path[1]);
    EXPECT_EQ(std::string_view("id"), path[2]);
    EXPECT_FALSE(path.Empty());

    // 纯下标开头 / 连续下标：与编译期 token 化规则一致
    const RuntimePath indexed("[0].a");
    ASSERT_EQ(2u, indexed.Size());
    EXPECT_EQ(std::string_view("[0]"), indexed[0]);
    const RuntimePath consecutive("a[0][1]");
    ASSERT_EQ(3u, consecutive.Size());
    EXPECT_EQ(std::string_view("a"), consecutive[0]);
    EXPECT_EQ(std::string_view("[0]"), consecutive[1]);
    EXPECT_EQ(std::string_view("[1]"), consecutive[2]);

    const SC_JSON_TYPE json = SC_JSON_MAKE_JSON();
    JsonParser parser(json);
    int id = 0;
    parser.Acquire(path, id);
    EXPECT_EQ(2, id);
    EXPECT_TRUE(parser.View<Required>(path).valid());
}

TEST(SC_JSON_TEST_SUITE_NAME, RuntimePathInvalidReportedByOnPathResolveError) {
    using ErrorHandler = SC_JSON_CONCAT(ErrorHandler_, SC_JSON_TEST_SUITE_NAME);
    ErrorHandler::resolve_errors.clear();

    const SC_JSON_TYPE json = SC_JSON_MAKE_JSON();
    JsonParser<SC_JSON_TYPE, ErrorHandler> parser(json);

    // 编译期字面量会直接编译失败；运行期路径改由 OnPathResolveError 报告
    const RuntimePath empty_segment("a..b");
    int value = 0;
    parser.Acquire<JsonInt, Required>(empty_segment, value, -1);
    EXPECT_EQ(-1, value);
    ASSERT_EQ(1u, ErrorHandler::resolve_errors.size());
    EXPECT_NE(std::string::npos, ErrorHandler::resolve_errors.front().find("path 'a..b'"));

    // '[' 未闭合：同样交给 OnPathResolveError（记录型 handler 返回后写默认值）
    const RuntimePath unclosed("a[1");
    int required_value = 0;
    parser.Acquire<JsonInt, Required>(unclosed, required_value);
    EXPECT_EQ(0, required_value);
    ASSERT_EQ(2u, ErrorHandler::resolve_errors.size());
    EXPECT_NE(std::string::npos, ErrorHandler::resolve_errors.back().find("path 'a[1'"));

    // 默认 handler 下同样报错（抛异常）
    JsonParser<SC_JSON_TYPE> strict(json);
    int throwing_value = 0;
    EXPECT_THROW((strict.Acquire<JsonInt, Required>(unclosed, throwing_value)), std::runtime_error);
}

TEST(SC_JSON_TEST_SUITE_NAME, RuntimePathArrayAndMap) {
    const SC_JSON_TYPE json = SC_JSON_MAKE_JSON();
    JsonParser parser(json);

    const RuntimePath list_path(std::string("list"));
    std::vector<int> nums;
    parser.AcquireArray(list_path, nums);
    EXPECT_EQ((std::vector<int>{1, 2, 3}), nums);

    const RuntimePath point_path(std::string("point"));
    std::map<std::string, int> point;
    parser.AcquireMap(point_path, point);
    ASSERT_EQ(2u, point.size());
    EXPECT_EQ(10, point["x"]);
    EXPECT_EQ(20, point["y"]);
}

TEST(SC_JSON_TEST_SUITE_NAME, RuntimePathOnViewIsRelative) {
    const SC_JSON_TYPE json = SC_JSON_MAKE_JSON();
    JsonParser parser(json);
    auto point = parser.View<Required>("point"_path);

    const RuntimePath inner(std::string("x"));  // 相对 view 的当前节点
    int x = 0;
    point.Acquire(inner, x);
    EXPECT_EQ(10, x);
}

// ---------------- JsonTraits 操作（JsonParser / JsonView 同构）----------------

namespace {

// 编译期检查：JsonTraits 的全部操作在 Source（JsonParser 或 JsonView）上都可用
template <typename Source>
bool TraitsOpsAvailable(const Source& source) {
    static_assert(requires { source.IsObject(); });
    static_assert(requires { source.IsArray(); });
    static_assert(requires { source.IsInt(); });
    static_assert(requires { source.IsUint(); });
    static_assert(requires { source.IsInt64(); });
    static_assert(requires { source.IsUint64(); });
    static_assert(requires { source.IsDouble(); });
    static_assert(requires { source.IsString(); });
    static_assert(requires { source.IsBool(); });
    static_assert(requires { source.TryGetChild(std::string_view{}); });
    static_assert(requires { source.GetArraySize(); });
    static_assert(requires { source.GetArrayElement(std::size_t{}); });
    static_assert(requires { source.MemberBegin(); });
    static_assert(requires { source.MemberEnd(); });
    static_assert(requires { source.GetInt(); });
    static_assert(requires { source.GetUint(); });
    static_assert(requires { source.GetInt64(); });
    static_assert(requires { source.GetUint64(); });
    static_assert(requires { source.GetDouble(); });
    static_assert(requires { source.GetString(); });
    static_assert(requires { source.GetBool(); });
    return true;
}

}  // namespace

TEST(SC_JSON_TEST_SUITE_NAME, TraitsOpsAreSymmetric) {
    const SC_JSON_TYPE json = SC_JSON_MAKE_JSON();
    JsonParser parser(json);
    auto view = parser.View<Required>("point"_path);
    // 同一份检查代码对 parser 与 view 都成立
    EXPECT_TRUE(TraitsOpsAvailable(parser));
    EXPECT_TRUE(TraitsOpsAvailable(view));
}

TEST(SC_JSON_TEST_SUITE_NAME, TraitsOpsOnParserAndView) {
    const SC_JSON_TYPE json = SC_JSON_MAKE_JSON();
    JsonParser parser(json);

    // 根节点：对象
    EXPECT_TRUE(parser.IsObject());
    EXPECT_FALSE(parser.IsArray());
    EXPECT_FALSE(parser.IsString());

    // 子节点访问（不再需要 JsonTraits<Json>::TryGetChild(parser.json(), "a")）
    const SC_JSON_TYPE* a = parser.TryGetChild("a");
    ASSERT_NE(nullptr, a);
    EXPECT_TRUE(JsonTraits<SC_JSON_TYPE>::IsObject(*a));
    EXPECT_EQ(nullptr, parser.TryGetChild("no.such"));

    // 成员遍历
    std::size_t members = 0;
    for (auto it = parser.MemberBegin(); it != parser.MemberEnd(); ++it) {
        ++members;
    }
    EXPECT_GT(members, 0u);

    // view 上的同类操作（相对当前节点）
    auto point = parser.View<Required>("point"_path);
    EXPECT_TRUE(point.IsObject());
    const SC_JSON_TYPE* x = point.TryGetChild("x");
    ASSERT_NE(nullptr, x);
    EXPECT_TRUE(JsonTraits<SC_JSON_TYPE>::IsInt(*x));

    // 标量读取：Is* + Get*
    EXPECT_EQ(42, parser.View<Required>("a.b"_path).GetInt());
    EXPECT_EQ(9007199254740993LL, parser.View<Required>("big"_path).GetInt64());
    EXPECT_TRUE(parser.View<Required>("flag"_path).GetBool());
    EXPECT_DOUBLE_EQ(3.14, parser.View<Required>("d"_path).GetDouble());
    EXPECT_EQ(std::string("hello"), parser.View<Required>("s"_path).GetString());

    // 数组：尺寸与元素
    auto list = parser.View<Required>("list"_path);
    EXPECT_TRUE(list.IsArray());
    EXPECT_EQ(3u, list.GetArraySize());
    EXPECT_EQ(2, JsonTraits<SC_JSON_TYPE>::GetInt(list.GetArrayElement(1)));
}

TEST(SC_JSON_TEST_SUITE_NAME, TraitsOpsOnInvalidViewAreSafe) {
    const SC_JSON_TYPE json = SC_JSON_MAKE_JSON();
    JsonParser parser(json);
    auto invalid = parser.View("no.such"_path);
    EXPECT_FALSE(invalid.valid());

    EXPECT_FALSE(invalid.IsObject());
    EXPECT_FALSE(invalid.IsArray());
    EXPECT_FALSE(invalid.IsInt());
    EXPECT_EQ(nullptr, invalid.TryGetChild("a"));
    EXPECT_EQ(0u, invalid.GetArraySize());
    EXPECT_EQ(0, invalid.GetInt());
    EXPECT_EQ(0.0, invalid.GetDouble());
    EXPECT_TRUE(invalid.GetString().empty());
    EXPECT_FALSE(invalid.GetBool());
    EXPECT_EQ(invalid.MemberBegin(), invalid.MemberEnd());
    // GetArrayElement：返回静态空节点，不会解引用空指针
    EXPECT_FALSE(JsonTraits<SC_JSON_TYPE>::IsObject(invalid.GetArrayElement(0)));
}
