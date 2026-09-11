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
        value = parser.Acquire<JsonInt, Required>("a.b"_path);
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
    value = parser.Acquire<JsonInt, Required>("a.b"_path);
    EXPECT_EQ(42, value);
}

TEST(SC_JSON_TEST_SUITE_NAME, AcquireNegativeInteger) {
    const SC_JSON_TYPE json = SC_JSON_MAKE_JSON();
    JsonParser parser(json);
    int value = 0;
    value = parser.Acquire<JsonInt, Required>("neg"_path);
    EXPECT_EQ(-7, value);
}

TEST(SC_JSON_TEST_SUITE_NAME, AcquireInt64Negative) {
    const SC_JSON_TYPE json = SC_JSON_MAKE_JSON();
    JsonParser parser(json);
    std::int64_t value = 0;
    value = parser.Acquire<JsonInt64, Required>("big_neg"_path);
    EXPECT_EQ(-9007199254740993LL, value);
}

TEST(SC_JSON_TEST_SUITE_NAME, AcquireInt64SmallInt) {
    const SC_JSON_TYPE json = SC_JSON_MAKE_JSON();
    JsonParser parser(json);
    std::int64_t value = 0;
    value = parser.Acquire<JsonInt64, Required>("a.b"_path);
    EXPECT_EQ(42LL, value);
}

TEST(SC_JSON_TEST_SUITE_NAME, AcquireInt64) {
    const SC_JSON_TYPE json = SC_JSON_MAKE_JSON();
    JsonParser parser(json);
    std::int64_t value = 0;
    value = parser.Acquire<JsonInt64, Required>("big"_path);
    EXPECT_EQ(9007199254740993LL, value);
}

TEST(SC_JSON_TEST_SUITE_NAME, Int64TypeMismatchThrows) {
    const SC_JSON_TYPE json = SC_JSON_MAKE_JSON();
    JsonParser parser(json);
    std::int64_t value = 0;
    EXPECT_THROW(((parser.Acquire<JsonInt64, Required>("d"_path))), std::runtime_error);
    EXPECT_THROW(((parser.Acquire<JsonInt64, Required>("s"_path))), std::runtime_error);
}

TEST(SC_JSON_TEST_SUITE_NAME, IntLosslessRangeChecks) {
    const SC_JSON_TYPE json = SC_JSON_MAKE_JSON();
    JsonParser parser(json);
    int value = 0;
    // 超出 int32 范围（上下界）的整数值不能无损转 int，必须按类型不匹配处理。
    EXPECT_THROW(((parser.Acquire<JsonInt, Required>("big"_path))), std::runtime_error);
    EXPECT_THROW(((parser.Acquire<JsonInt, Required>("big_neg"_path))), std::runtime_error);
}

TEST(SC_JSON_TEST_SUITE_NAME, AcquireString) {
    const SC_JSON_TYPE json = SC_JSON_MAKE_JSON();
    JsonParser parser(json);
    std::string value;
    value = parser.Acquire<JsonString, Required>("s"_path);
    EXPECT_EQ(std::string("hello"), value);
}

TEST(SC_JSON_TEST_SUITE_NAME, AcquireDouble) {
    const SC_JSON_TYPE json = SC_JSON_MAKE_JSON();
    JsonParser parser(json);
    double value = 0.0;
    value = parser.Acquire<JsonDouble, Required>("d"_path);
    EXPECT_DOUBLE_EQ(3.14, value);
}

TEST(SC_JSON_TEST_SUITE_NAME, AcquireBool) {
    const SC_JSON_TYPE json = SC_JSON_MAKE_JSON();
    JsonParser parser(json);
    bool value = false;
    value = parser.Acquire<JsonBool, Required>("flag"_path);
    EXPECT_TRUE(value);
}

TEST(SC_JSON_TEST_SUITE_NAME, AcquireArrayIndex) {
    const SC_JSON_TYPE json = SC_JSON_MAKE_JSON();
    JsonParser parser(json);
    int value = 0;
    value = parser.Acquire<JsonInt, Required>("list[1]"_path);
    EXPECT_EQ(2, value);
}

TEST(SC_JSON_TEST_SUITE_NAME, AcquireNestedArrayIndex) {
    const SC_JSON_TYPE json = SC_JSON_MAKE_JSON();
    JsonParser parser(json);
    int value = 0;
    value = parser.Acquire<JsonInt, Required>("obj_list[1].id"_path);
    EXPECT_EQ(2, value);
}

TEST(SC_JSON_TEST_SUITE_NAME, AcquireEmptyPathUsesRoot) {
    const SC_JSON_TYPE json = SC_JSON_MAKE_JSON_LEAF();
    JsonParser parser(json);
    int value = 0;
    value = parser.Acquire<JsonInt, Required>(""_path);
    EXPECT_EQ(42, value);
}

TEST(SC_JSON_TEST_SUITE_NAME, AcquireChainedCalls) {
    const SC_JSON_TYPE json = SC_JSON_MAKE_JSON();
    JsonParser parser(json);
    const int intValue = parser.Acquire<JsonInt, Required>("a.b"_path);
    const std::string stringValue = parser.Acquire<JsonString, Required>("s"_path);
    const bool boolValue = parser.Acquire<JsonBool, Required>("flag"_path);
    EXPECT_EQ(42, intValue);
    EXPECT_EQ(std::string("hello"), stringValue);
    EXPECT_TRUE(boolValue);
}

TEST(SC_JSON_TEST_SUITE_NAME, OptionalArrayIndexOutOfRangeWritesDefault) {
    const SC_JSON_TYPE json = SC_JSON_MAKE_JSON();
    JsonParser parser(json);
    int value = -1;
    value = parser.Acquire<JsonInt, Optional>("list[9]"_path);
    EXPECT_EQ(0, value);
}

TEST(SC_JSON_TEST_SUITE_NAME, AcquireArrayIndexOutOfRangeThrows) {
    const SC_JSON_TYPE json = SC_JSON_MAKE_JSON();
    JsonParser parser(json);
    int value = 0;
    EXPECT_THROW(((parser.Acquire<JsonInt, Required>("list[9]"_path))), std::runtime_error);
}

TEST(SC_JSON_TEST_SUITE_NAME, AcquireTraversalTypeMismatchThrows) {
    const SC_JSON_TYPE json = SC_JSON_MAKE_JSON();
    JsonParser parser(json);
    int value = 0;
    EXPECT_THROW(((parser.Acquire<JsonInt, Required>("list.b"_path))), std::runtime_error);
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
    value = second.Acquire<JsonInt>(""_path);
    EXPECT_EQ(2, value);
    // 也可以基于该节点构造子解析器
    JsonParser sub(second.json());
    value = sub.Acquire<JsonInt, Required>(""_path);
    EXPECT_EQ(2, value);
}

TEST(SC_JSON_TEST_SUITE_NAME, CustomExtractor) {
    using Point = SC_JSON_CONCAT(Point_, SC_JSON_TEST_SUITE_NAME);
    const SC_JSON_TYPE json = SC_JSON_MAKE_JSON();
    JsonParser parser(json);
    Point point;
    point = parser.Acquire<SC_JSON_CONCAT(PointExtractor_, SC_JSON_TEST_SUITE_NAME), Required>(
            "point"_path);
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
    // 路径解析失败 → OnPathResolveError；handler 返回时 out 保持不变
    value = parser.Acquire<JsonInt, Required>("no.such.path"_path, -1);
    EXPECT_EQ(-1, value);
    ASSERT_EQ(1u, ErrorHandler::resolve_errors.size());
    EXPECT_NE(std::string::npos, ErrorHandler::resolve_errors.front().find("no.such.path"));

    // 节点存在但类型不符 → OnTypeMismatch
    value = parser.Acquire<JsonInt, Required>("s"_path, -1);
    EXPECT_EQ(-1, value);
    ASSERT_EQ(1u, ErrorHandler::mismatch.size());
    EXPECT_EQ(std::string("s"), ErrorHandler::mismatch.front());

    value = parser.Acquire<JsonInt, Required>("a.b"_path);
    EXPECT_EQ(42, value);
}

TEST(SC_JSON_TEST_SUITE_NAME, StatefulErrorHandlerCarriesExternalData) {
    using Exception = SC_JSON_CONCAT(ContextException_, SC_JSON_TEST_SUITE_NAME);
    using Handler = SC_JSON_CONCAT(ContextErrorHandler_, SC_JSON_TEST_SUITE_NAME);

    const SC_JSON_TYPE json = SC_JSON_MAKE_JSON();
    JsonParser<SC_JSON_TYPE, Handler> parser(json, Handler("req-001"));

    int value = 0;
    EXPECT_THROW(((parser.Acquire<JsonInt, Required>("no.such.path"_path))), Exception);

    try {
        value = parser.Acquire<JsonInt, Required>("s"_path);
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
    value = view.Acquire<JsonInt>("b"_path);
    EXPECT_EQ(42, value);
}

TEST(SC_JSON_TEST_SUITE_NAME, AcquireAssignsValue) {
    const SC_JSON_TYPE json = SC_JSON_MAKE_JSON();
    JsonParser parser(json);
    int value = 0;
    value = parser.Acquire<JsonInt, Required>("a.b"_path);
    EXPECT_EQ(42, value);
}

TEST(SC_JSON_TEST_SUITE_NAME, AcquireRequiredMissingThrows) {
    const SC_JSON_TYPE json = SC_JSON_MAKE_JSON();
    JsonParser parser(json);
    int value = 0;
    EXPECT_THROW(((parser.Acquire<JsonInt, Required>("no.such.path"_path))), std::runtime_error);
}

TEST(SC_JSON_TEST_SUITE_NAME, AcquireRequiredTypeMismatchThrows) {
    const SC_JSON_TYPE json = SC_JSON_MAKE_JSON();
    JsonParser parser(json);
    int value = 0;
    EXPECT_THROW(((parser.Acquire<JsonInt, Required>("s"_path))), std::runtime_error);
}

TEST(SC_JSON_TEST_SUITE_NAME, AcquireOptionalMissingAssignsDefault) {
    const SC_JSON_TYPE json = SC_JSON_MAKE_JSON();
    JsonParser parser(json);
    int value = 123;
    value = parser.Acquire<JsonInt>("no.such.path"_path);
    EXPECT_EQ(0, value);
}

TEST(SC_JSON_TEST_SUITE_NAME, AcquireOptionalTypeMismatchAssignsDefault) {
    const SC_JSON_TYPE json = SC_JSON_MAKE_JSON();
    JsonParser parser(json);
    int value = 123;
    value = parser.Acquire<JsonInt>("s"_path);
    EXPECT_EQ(0, value);
}

TEST(SC_JSON_TEST_SUITE_NAME, AcquireOptionalUsesDefaultValue) {
    const SC_JSON_TYPE json = SC_JSON_MAKE_JSON();
    JsonParser parser(json);
    int value = 0;
    value = parser.Acquire<JsonInt>("no.such.path"_path, 42);
    EXPECT_EQ(42, value);
}

TEST(SC_JSON_TEST_SUITE_NAME, AcquireOptionalTypeMismatchUsesDefaultValue) {
    const SC_JSON_TYPE json = SC_JSON_MAKE_JSON();
    JsonParser parser(json);
    int value = 0;
    value = parser.Acquire<JsonInt>("s"_path, 42);
    EXPECT_EQ(42, value);
}

TEST(SC_JSON_TEST_SUITE_NAME, AcquireDefaultValuePresentUsesActual) {
    const SC_JSON_TYPE json = SC_JSON_MAKE_JSON();
    JsonParser parser(json);
    int value = 0;
    value = parser.Acquire<JsonInt>("a.b"_path, 7);
    EXPECT_EQ(42, value);
}

TEST(SC_JSON_TEST_SUITE_NAME, AcquireOptionalStringUsesDefaultValue) {
    const SC_JSON_TYPE json = SC_JSON_MAKE_JSON();
    JsonParser parser(json);
    std::string value = parser.Acquire<JsonString>("missing"_path, std::string("unknown"));
    EXPECT_EQ(std::string("unknown"), value);
}

TEST(SC_JSON_TEST_SUITE_NAME, AcquireMultipleFields) {
    const SC_JSON_TYPE json = SC_JSON_MAKE_JSON();
    JsonParser parser(json);
    const int intValue = parser.Acquire<JsonInt, Required>("a.b"_path);
    const std::string stringValue = parser.Acquire<JsonString, Required>("s"_path);
    const bool boolValue = parser.Acquire<JsonBool, Required>("flag"_path);
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
        value = parser.Acquire<JsonInt, Required>("no.such.path"_path);
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
    arr = parser.AcquireArray<JsonInt, Required>("list"_path);
    ASSERT_EQ(3u, arr.size());
    EXPECT_EQ(1, arr[0]);
    EXPECT_EQ(2, arr[1]);
    EXPECT_EQ(3, arr[2]);
}

TEST(SC_JSON_TEST_SUITE_NAME, AcquireArrayMissingOptional) {
    const SC_JSON_TYPE json = SC_JSON_MAKE_JSON();
    JsonParser parser(json);
    std::vector<int> arr = {9};
    arr = parser.AcquireArray<JsonInt>("no.such.path"_path);
    EXPECT_TRUE(arr.empty());
}

TEST(SC_JSON_TEST_SUITE_NAME, AcquireArrayMissingRequiredThrows) {
    const SC_JSON_TYPE json = SC_JSON_MAKE_JSON();
    JsonParser parser(json);
    std::vector<int> arr;
    EXPECT_THROW(((parser.AcquireArray<JsonInt, Required>("no.such.path"_path))), std::runtime_error);
}

TEST(SC_JSON_TEST_SUITE_NAME, AcquireArrayElementMismatchPath) {
    using ErrorHandler = SC_JSON_CONCAT(ErrorHandler_, SC_JSON_TEST_SUITE_NAME);
    ErrorHandler::mismatch.clear();

    const SC_JSON_TYPE json = SC_JSON_MAKE_JSON();
    JsonParser<SC_JSON_TYPE, ErrorHandler> parser(json);
    std::vector<int> arr;
    arr = parser.AcquireArray<JsonInt, Required>("obj_list"_path);
    ASSERT_EQ(1u, ErrorHandler::mismatch.size());
    EXPECT_EQ(std::string("obj_list[0]"), ErrorHandler::mismatch.front());
}

TEST(SC_JSON_TEST_SUITE_NAME, AcquireMapInts) {
    const SC_JSON_TYPE json = SC_JSON_MAKE_JSON();
    JsonParser parser(json);
    std::map<std::string, int> map;
    map = parser.AcquireMap<JsonInt, Required>("a"_path);
    ASSERT_EQ(1u, map.size());
    EXPECT_EQ(42, map["b"]);
}

TEST(SC_JSON_TEST_SUITE_NAME, AcquireMapMissingOptional) {
    const SC_JSON_TYPE json = SC_JSON_MAKE_JSON();
    JsonParser parser(json);
    std::map<std::string, int> map;
    map = parser.AcquireMap<JsonInt>("no.such.path"_path);
    EXPECT_TRUE(map.empty());
}

TEST(SC_JSON_TEST_SUITE_NAME, AcquireMapValueMismatchPath) {
    using ErrorHandler = SC_JSON_CONCAT(ErrorHandler_, SC_JSON_TEST_SUITE_NAME);
    ErrorHandler::mismatch.clear();

    const SC_JSON_TYPE json = SC_JSON_MAKE_JSON();
    JsonParser<SC_JSON_TYPE, ErrorHandler> parser(json);
    std::map<std::string, std::string> map;
    map = parser.AcquireMap<JsonString, Required>("a"_path);
    ASSERT_EQ(1u, ErrorHandler::mismatch.size());
    EXPECT_EQ(std::string("a.b"), ErrorHandler::mismatch.front());
}

TEST(SC_JSON_TEST_SUITE_NAME, ConstructFromString) {
    JsonParser<SC_JSON_TYPE> parser(std::string_view(R"({"a":{"b":42}})"));
    int value = 0;
    value = parser.Acquire<JsonInt, Required>("a.b"_path);
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
    x = point.Acquire<JsonInt>("x"_path);
    EXPECT_EQ(10, x);
    // parser 是无状态的：路径仍相对根节点解析
    int y = 0;
    y = parser.Acquire<JsonInt, Required>("point.y"_path);
    EXPECT_EQ(20, y);
}

TEST(SC_JSON_TEST_SUITE_NAME, ViewNestedRelativePaths) {
    const SC_JSON_TYPE json = SC_JSON_MAKE_JSON();
    JsonParser parser(json);
    auto list = parser.View<Required>("obj_list"_path);
    auto second = list.View<Required>("[1]"_path);
    int id = 0;
    id = second.Acquire<JsonInt>("id"_path);
    EXPECT_EQ(2, id);
    // 兄弟 view 互不影响（无需 RAII 回滚）
    auto first = list.View<Required>("[0]"_path);
    id = first.Acquire<JsonInt>("id"_path);
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
    x = view.Acquire<JsonInt>("x"_path);
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
    optional_value = invalid.Acquire<JsonInt>("a.b"_path);
    EXPECT_EQ(0, optional_value);
    // Required 在无效 view 上同样不抛异常，写默认值
    int required_value = 7;
    required_value = invalid.Acquire<JsonInt, Required>("a.b"_path);
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
    value = b.Acquire<JsonInt>(""_path);  // 空路径 = 当前节点
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
        value = element.Acquire<JsonInt>(""_path);
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
                       id = element.template Acquire<JsonInt>("id"_path);
                       return id;
                   });
    for (int id : id_view) {
        ids.push_back(id);
    }
    EXPECT_EQ((std::vector<int>{1, 2}), ids);

    int matched = 0;
    auto filtered = parser.ViewArray("obj_list"_path) | std::views::filter([](auto&& element) {
                        int id = 0;
                        id = element.template Acquire<JsonInt>("id"_path);
                        return id == 2;
                    });
    for (auto&& element : filtered) {
        int id = 0;
        id = element.Acquire<JsonInt>("id"_path);
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
        number = value.Acquire<JsonInt>(""_path);
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
