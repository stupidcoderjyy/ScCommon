// 每个 JSON 库的测试文件通过先定义以下宏、再包含本文件来复用同一套用例：
//   SC_JSON_TEST_SUITE_NAME   测试套件名（如 JsonViewNlohmann）
//   SC_JSON_TYPE              JSON 库类型（如 nlohmann::json）
//   SC_JSON_MAKE_JSON()       构造包含测试数据的 JSON 文档
//   SC_JSON_MAKE_JSON_LEAF()  构造根节点为整数的 JSON 文档
#pragma once
#include <gtest/gtest.h>

#include <cstdint>
#include <exception>
#include <map>
#include <ranges>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "json/json_view.hpp"
#include "json/view/json_array_view.h"
#include "json/view/json_object_view.h"

using namespace sc::json;

#define SC_JSON_CONCAT_IMPL(a, b) a##b
#define SC_JSON_CONCAT(a, b) SC_JSON_CONCAT_IMPL(a, b)

// 记录型错误处理器：不抛异常，只记录
struct SC_JSON_CONCAT(ErrorHandler_, SC_JSON_TEST_SUITE_NAME) {
  static inline std::vector<std::string> resolve_errors;
  static inline std::vector<std::string> mismatches;
  static inline std::vector<std::string> parse_errors;

  static void OnPathResolveError(const std::exception& e) {
    resolve_errors.emplace_back(e.what());
  }
  static void OnTypeMismatch(std::string_view path) {
    mismatches.emplace_back(path);
  }
  static void OnJsonParseError(std::string_view json, std::exception& e) {
    (void)json;
    parse_errors.emplace_back(e.what());
  }
};

// 携带外部数据（请求 ID）的异常与处理器
struct SC_JSON_CONCAT(ContextException_, SC_JSON_TEST_SUITE_NAME)
    : std::runtime_error {
  SC_JSON_CONCAT(ContextException_, SC_JSON_TEST_SUITE_NAME)(
      std::string request_id, std::string message)
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
  // JsonView 按值持有 handler（成员默认初始化 + JsonView{}），故需可默认构造
  explicit SC_JSON_CONCAT(ContextErrorHandler_, SC_JSON_TEST_SUITE_NAME)(
      std::string request_id = {})
      : request_id_(std::move(request_id)) {}

  void OnPathResolveError(const std::exception& e) const {
    throw SC_JSON_CONCAT(ContextException_, SC_JSON_TEST_SUITE_NAME)(
        request_id_, e.what());
  }
  void OnTypeMismatch(std::string_view path) const {
    throw SC_JSON_CONCAT(ContextException_, SC_JSON_TEST_SUITE_NAME)(
        request_id_, std::string(path));
  }
  void OnJsonParseError(std::string_view, std::exception& e) const {
    throw SC_JSON_CONCAT(ContextException_, SC_JSON_TEST_SUITE_NAME)(
        request_id_, e.what());
  }

 private:
  std::string request_id_;
};

// 自定义类型 + 自定义提取器（特化 Extractor<JsonType, MyType>）
struct SC_JSON_CONCAT(Point_, SC_JSON_TEST_SUITE_NAME) {
  int x = 0;
  int y = 0;
};

template <>
struct sc::json::Extractor<SC_JSON_TYPE, SC_JSON_CONCAT(Point_, SC_JSON_TEST_SUITE_NAME)> {
  using ValueType = SC_JSON_CONCAT(Point_, SC_JSON_TEST_SUITE_NAME);

  static bool Extract(const SC_JSON_TYPE& json, ValueType& out) {
    const SC_JSON_TYPE* x = JsonTraits<SC_JSON_TYPE>::TryGetChild(json, "x");
    const SC_JSON_TYPE* y = JsonTraits<SC_JSON_TYPE>::TryGetChild(json, "y");
    if (x == nullptr || y == nullptr) {
      return false;
    }
    out.x = JsonTraits<SC_JSON_TYPE>::GetInt(*x);
    out.y = JsonTraits<SC_JSON_TYPE>::GetInt(*y);
    return true;
  }
};

// Acquire 系列要求显式指定提取器模板，自定义类型的提取器用别名模板传进去
template <typename Json>
using SC_JSON_CONCAT(PointExtractor_, SC_JSON_TEST_SUITE_NAME) =
    sc::json::Extractor<Json, SC_JSON_CONCAT(Point_, SC_JSON_TEST_SUITE_NAME)>;

// ---------------- 构造 ----------------

TEST(SC_JSON_TEST_SUITE_NAME, ViewFromNode) {
  const SC_JSON_TYPE json = SC_JSON_MAKE_JSON();
  JsonView<SC_JSON_TYPE> view(json);
  EXPECT_TRUE(view.valid());
  EXPECT_TRUE(static_cast<bool>(view));
  EXPECT_TRUE(view.IsObject());
  EXPECT_FALSE(view.IsArray());
}

TEST(SC_JSON_TEST_SUITE_NAME, DefaultConstructedViewIsInvalid) {
  // 两个构造函数的 handler 都有默认值：不带 handler 时用 DefaultJsonErrorHandler
  JsonView<SC_JSON_TYPE> empty;
  EXPECT_FALSE(empty.valid());
  EXPECT_FALSE(static_cast<bool>(empty));
  EXPECT_EQ(0, empty.GetInt());
}

TEST(SC_JSON_TEST_SUITE_NAME, ViewFromText) {
  JsonView<SC_JSON_TYPE> view(std::string_view(R"({"a":{"b":42},"s":"hi"})"));
  EXPECT_TRUE(view.IsObject());
  EXPECT_EQ(42, view.View<Required>("a.b"_path).GetInt());
  EXPECT_EQ(std::string("hi"), view.View<Required>("s"_path).GetString());
}

TEST(SC_JSON_TEST_SUITE_NAME, ViewFromInvalidTextThrows) {
  EXPECT_THROW(
      (JsonView<SC_JSON_TYPE>(std::string_view("{oops"))),
      std::runtime_error);
}

TEST(SC_JSON_TEST_SUITE_NAME, ViewFromInvalidTextCallsHandler) {
  using ErrorHandler = SC_JSON_CONCAT(ErrorHandler_, SC_JSON_TEST_SUITE_NAME);
  ErrorHandler handler;
  ErrorHandler::parse_errors.clear();

  // 处理器被调用是核心约定；处理器吞掉异常后各库抛出的具体异常类型不完全一致
  // （nlohmann 会重新抛出自身的 parse_error，jsoncpp/rapidjson 抛 std::runtime_error），
  // 所以这里只要求抛出 std::exception。
  EXPECT_THROW(
      (JsonView<SC_JSON_TYPE, ErrorHandler>(std::string_view("{oops"), handler)),
      std::exception);
  EXPECT_EQ(1u, ErrorHandler::parse_errors.size());
}

// ---------------- 路径解析 ----------------

TEST(SC_JSON_TEST_SUITE_NAME, ViewResolvesPath) {
  const SC_JSON_TYPE json = SC_JSON_MAKE_JSON();
  JsonView<SC_JSON_TYPE> view(json);

  EXPECT_EQ(42, view.View<Required>("a.b"_path).GetInt());
  EXPECT_EQ(-7, view.View<Required>("neg"_path).GetInt());
  EXPECT_EQ(9007199254740993LL, view.View<Required>("big"_path).GetInt64());
  EXPECT_EQ(std::string("hello"), view.View<Required>("s"_path).GetString());
  EXPECT_DOUBLE_EQ(3.14, view.View<Required>("d"_path).GetDouble());
  EXPECT_TRUE(view.View<Required>("flag"_path).GetBool());
}

TEST(SC_JSON_TEST_SUITE_NAME, ViewResolvesArrayIndex) {
  const SC_JSON_TYPE json = SC_JSON_MAKE_JSON();
  JsonView<SC_JSON_TYPE> view(json);

  EXPECT_EQ(2, view.View<Required>("list[1]"_path).GetInt());
  EXPECT_EQ(2, view.View<Required>("obj_list[1].id"_path).GetInt());
}

TEST(SC_JSON_TEST_SUITE_NAME, ViewEmptyPathIsCurrentNode) {
  const SC_JSON_TYPE leaf = SC_JSON_MAKE_JSON_LEAF();
  JsonView<SC_JSON_TYPE> view(leaf);
  EXPECT_EQ(42, view.View<Required>(""_path).GetInt());
}

TEST(SC_JSON_TEST_SUITE_NAME, ViewOptionalMissingGivesInvalidView) {
  const SC_JSON_TYPE json = SC_JSON_MAKE_JSON();
  JsonView<SC_JSON_TYPE> view(json);

  auto missing = view.View("no.such.path"_path);
  EXPECT_FALSE(missing.valid());
  EXPECT_EQ(0, missing.GetInt());  // 无效 view 上取值返回默认值
}

TEST(SC_JSON_TEST_SUITE_NAME, ViewRequiredMissingThrows) {
  const SC_JSON_TYPE json = SC_JSON_MAKE_JSON();
  JsonView<SC_JSON_TYPE> view(json);
  EXPECT_THROW((void)view.View<Required>("no.such.path"_path), std::runtime_error);
  EXPECT_THROW((void)view.View<Required>("list[9]"_path), std::runtime_error);
  EXPECT_THROW((void)view.View<Required>("list.b"_path), std::runtime_error);
}

TEST(SC_JSON_TEST_SUITE_NAME, NestedViewsAreRelative) {
  const SC_JSON_TYPE json = SC_JSON_MAKE_JSON();
  JsonView<SC_JSON_TYPE> view(json);

  auto point = view.View<Required>("point"_path);
  EXPECT_TRUE(point.IsObject());
  EXPECT_EQ(10, point.View<Required>("x"_path).GetInt());
  EXPECT_EQ(20, point.View<Required>("y"_path).GetInt());

  // 无效 view 上链式调用返回空 view，即使策略是 Required（没有节点可解析）
  auto invalid = view.View("no.such"_path);
  EXPECT_FALSE(invalid.View<Required>("x"_path).valid());
}

// ---------------- 数组视图 ----------------

TEST(SC_JSON_TEST_SUITE_NAME, ArrayViewIteratesElements) {
  const SC_JSON_TYPE json = SC_JSON_MAKE_JSON();
  JsonView<SC_JSON_TYPE> view(json);

  auto range = view.ViewArray<Required>("list"_path);
  EXPECT_FALSE(range.empty());
  EXPECT_EQ(3u, range.size());

  std::vector<int> values;
  for (auto&& element : range) {
    values.push_back(element.GetInt());
  }
  EXPECT_EQ((std::vector<int>{1, 2, 3}), values);

  // 随机访问
  EXPECT_EQ(2, (*(range.begin() + 1)).GetInt());
  EXPECT_EQ(3, static_cast<int>(range.end() - range.begin()));
}

TEST(SC_JSON_TEST_SUITE_NAME, ArrayViewEmptyOnMissingOptional) {
  const SC_JSON_TYPE json = SC_JSON_MAKE_JSON();
  JsonView<SC_JSON_TYPE> view(json);

  auto range = view.ViewArray("no.such"_path);
  EXPECT_TRUE(range.empty());
  EXPECT_EQ(0u, range.size());
  EXPECT_EQ(range.begin(), range.end());

  // 路径存在但不是数组：Optional 下同样得到空区间
  EXPECT_TRUE(view.ViewArray("point"_path).empty());
}

TEST(SC_JSON_TEST_SUITE_NAME, ArrayViewRequiredReportsErrors) {
  using ErrorHandler = SC_JSON_CONCAT(ErrorHandler_, SC_JSON_TEST_SUITE_NAME);
  ErrorHandler handler;
  ErrorHandler::resolve_errors.clear();
  ErrorHandler::mismatches.clear();

  const SC_JSON_TYPE json = SC_JSON_MAKE_JSON();
  JsonView<SC_JSON_TYPE, ErrorHandler> view(json, handler);

  // 路径缺失 → OnPathResolveError
  EXPECT_TRUE(view.ViewArray<Required>("no.such"_path).empty());
  ASSERT_EQ(1u, ErrorHandler::resolve_errors.size());
  EXPECT_NE(std::string::npos,
            ErrorHandler::resolve_errors.front().find("path 'no.such'"));

  // 节点存在但不是数组 → OnTypeMismatch
  EXPECT_TRUE(view.ViewArray<Required>("point"_path).empty());
  ASSERT_EQ(1u, ErrorHandler::mismatches.size());
  EXPECT_EQ(std::string("point"), ErrorHandler::mismatches.front());
}

TEST(SC_JSON_TEST_SUITE_NAME, ArrayViewComposesWithRanges) {
  const SC_JSON_TYPE json = SC_JSON_MAKE_JSON();
  JsonView<SC_JSON_TYPE> view(json);

  // transform：取每个元素的 id
  std::vector<int> ids;
  auto id_view = view.ViewArray("obj_list"_path) |
                 std::views::transform([](auto&& element) {
                   return element.View("id"_path).GetInt();
                 });
  for (int id : id_view) {
    ids.push_back(id);
  }
  EXPECT_EQ((std::vector<int>{1, 2}), ids);

  // filter + transform：只保留 id == 2
  auto names = view.ViewArray("obj_list"_path) |
               std::views::filter([](auto&& element) {
                 // 无显式模板实参 → 泛型 lambda 里不需要 .template
                 return element.View("id"_path).GetInt() == 2;
               });
  int matched = 0;
  for (auto&& element : names) {
    EXPECT_EQ(2, element.View("id"_path).GetInt());
    ++matched;
  }
  EXPECT_EQ(1, matched);
}

TEST(SC_JSON_TEST_SUITE_NAME, ArrayViewIsZeroCopyAndComposable) {
  const SC_JSON_TYPE json = SC_JSON_MAKE_JSON();
  JsonView<SC_JSON_TYPE> view(json);

  auto range = view.ViewArray<Required>("list"_path);
  // 元素指向底层文档中的节点（零拷贝）
  const SC_JSON_TYPE* list_node = JsonTraits<SC_JSON_TYPE>::TryGetChild(json, "list");
  ASSERT_NE(nullptr, list_node);
  EXPECT_EQ(&(*(range.begin() + 1)).json(),
            &JsonTraits<SC_JSON_TYPE>::GetArrayElement(*list_node, 1));
}

// ---------------- 对象视图 ----------------

TEST(SC_JSON_TEST_SUITE_NAME, ObjectViewStructuredBinding) {
  const SC_JSON_TYPE json = SC_JSON_MAKE_JSON();
  JsonView<SC_JSON_TYPE> view(json);

  std::map<std::string, int> values;
  for (auto&& [key, value] : view.ViewObject("point"_path)) {
    values.emplace(std::string(key), value.GetInt());
  }
  EXPECT_EQ(2u, values.size());
  EXPECT_EQ(10, values["x"]);
  EXPECT_EQ(20, values["y"]);
}

TEST(SC_JSON_TEST_SUITE_NAME, ObjectViewEmptyAndRequiredErrors) {
  using ErrorHandler = SC_JSON_CONCAT(ErrorHandler_, SC_JSON_TEST_SUITE_NAME);
  ErrorHandler handler;
  ErrorHandler::resolve_errors.clear();
  ErrorHandler::mismatches.clear();

  const SC_JSON_TYPE json = SC_JSON_MAKE_JSON();
  JsonView<SC_JSON_TYPE, ErrorHandler> view(json, handler);

  auto empty = view.ViewObject("no.such"_path);
  EXPECT_EQ(empty.begin(), empty.end());

  (void)view.ViewObject<Required>("no.such"_path);
  ASSERT_EQ(1u, ErrorHandler::resolve_errors.size());

  (void)view.ViewObject<Required>("list"_path);  // 数组不是对象
  ASSERT_EQ(1u, ErrorHandler::mismatches.size());
  EXPECT_EQ(std::string("list"), ErrorHandler::mismatches.front());
}

// ---------------- JsonTraits 操作（JsonView 直通）----------------

// ---------------- 变量访问器 Acquire 系列 ----------------
// 每种写法都支持：显式提取器 Acquire<JsonInt, Required>(...)
// 与由目标类型推导 Acquire<Required>(...) / Acquire(...)

TEST(SC_JSON_TEST_SUITE_NAME, AcquireScalarsWithExplicitExtractor) {
  const SC_JSON_TYPE json = SC_JSON_MAKE_JSON();
  JsonView<SC_JSON_TYPE> view(json);

  int i = 0;
  view.Acquire<JsonInt>("a.b"_path, i);
  EXPECT_EQ(42, i);

  int neg = 0;
  view.Acquire<JsonInt>("neg"_path, neg);
  EXPECT_EQ(-7, neg);

  std::int64_t big = 0;
  view.Acquire<JsonInt64>("big"_path, big);
  EXPECT_EQ(9007199254740993LL, big);

  std::string s;
  view.Acquire<JsonString>("s"_path, s);
  EXPECT_EQ(std::string("hello"), s);

  double d = 0;
  view.Acquire<JsonDouble>("d"_path, d);
  EXPECT_DOUBLE_EQ(3.14, d);

  bool flag = false;
  view.Acquire<JsonBool>("flag"_path, flag);
  EXPECT_TRUE(flag);
}

TEST(SC_JSON_TEST_SUITE_NAME, AcquireScalarsByTypeDeduction) {
  const SC_JSON_TYPE json = SC_JSON_MAKE_JSON();
  JsonView<SC_JSON_TYPE> view(json);

  int i = 0;
  view.Acquire("a.b"_path, i);  // 提取器由 out 类型推导
  EXPECT_EQ(42, i);

  std::int64_t big = 0;
  view.Acquire("big"_path, big);
  EXPECT_EQ(9007199254740993LL, big);

  std::string s;
  view.Acquire("s"_path, s);
  EXPECT_EQ(std::string("hello"), s);

  double d = 0;
  view.Acquire("d"_path, d);
  EXPECT_DOUBLE_EQ(3.14, d);

  bool flag = false;
  view.Acquire("flag"_path, flag);
  EXPECT_TRUE(flag);

  // 推导形式下，唯一的模板实参是 ExistencePolicy
  int required = 0;
  view.Acquire<Required>("a.b"_path, required, -1);
  EXPECT_EQ(42, required);
}

TEST(SC_JSON_TEST_SUITE_NAME, AcquireOptionalWritesDefault) {
  const SC_JSON_TYPE json = SC_JSON_MAKE_JSON();
  JsonView<SC_JSON_TYPE> view(json);

  // 路径缺失 → 默认值
  int missing = 0;
  view.Acquire<JsonInt>("no.such.path"_path, missing, 7);
  EXPECT_EQ(7, missing);

  // 类型不符 → 默认值
  int mismatch = 0;
  view.Acquire<JsonInt>("s"_path, mismatch, -1);
  EXPECT_EQ(-1, mismatch);

  // 不传默认值 → 值初始化
  int zero = 123;
  view.Acquire<JsonInt>("no.such.path"_path, zero);
  EXPECT_EQ(0, zero);

  // 路径存在且类型匹配 → 实际值
  int actual = 0;
  view.Acquire<JsonInt>("a.b"_path, actual, 7);
  EXPECT_EQ(42, actual);

  // 无效 view 上的 Acquire 等同 Optional（即使策略是 Required，也没有节点可解析）
  auto invalid = view.View("no.such"_path);
  int from_invalid = 5;
  invalid.Acquire<JsonInt, Required>("a.b"_path, from_invalid, -1);
  EXPECT_EQ(-1, from_invalid);

  // 推导形式的默认值语义与显式形式一致
  int deduced_missing = 0;
  view.Acquire("no.such.path"_path, deduced_missing, 7);
  EXPECT_EQ(7, deduced_missing);

  int deduced_mismatch = 0;
  view.Acquire("s"_path, deduced_mismatch, -1);
  EXPECT_EQ(-1, deduced_mismatch);

  int deduced_actual = 0;
  view.Acquire("a.b"_path, deduced_actual, 7);
  EXPECT_EQ(42, deduced_actual);
}

TEST(SC_JSON_TEST_SUITE_NAME, AcquireRequiredReportsErrors) {
  using ErrorHandler = SC_JSON_CONCAT(ErrorHandler_, SC_JSON_TEST_SUITE_NAME);
  ErrorHandler handler;
  ErrorHandler::resolve_errors.clear();
  ErrorHandler::mismatches.clear();

  const SC_JSON_TYPE json = SC_JSON_MAKE_JSON();
  JsonView<SC_JSON_TYPE, ErrorHandler> view(json, handler);

  int value = 0;
  // 路径缺失 → OnPathResolveError（handler 返回时写默认值）
  view.Acquire<JsonInt, Required>("no.such.path"_path, value, -1);
  EXPECT_EQ(-1, value);
  ASSERT_EQ(1u, ErrorHandler::resolve_errors.size());
  EXPECT_NE(std::string::npos,
            ErrorHandler::resolve_errors.front().find("path 'no.such.path'"));

  // 数组越界 → OnPathResolveError
  view.Acquire<JsonInt, Required>("list[9]"_path, value, -1);
  EXPECT_EQ(2u, ErrorHandler::resolve_errors.size());

  // 类型不符 → OnTypeMismatch
  view.Acquire<JsonInt, Required>("s"_path, value, -1);
  EXPECT_EQ(-1, value);
  ASSERT_EQ(1u, ErrorHandler::mismatches.size());
  EXPECT_EQ(std::string("s"), ErrorHandler::mismatches.front());

  // int 放不下的大整数同样按类型不符处理
  view.Acquire<JsonInt, Required>("big"_path, value, -1);
  EXPECT_EQ(2u, ErrorHandler::mismatches.size());
  EXPECT_EQ(std::string("big"), ErrorHandler::mismatches.back());

  // 推导形式（唯一模板实参是策略）同样按 Required 上报
  view.Acquire<Required>("no.such.again"_path, value, -1);
  EXPECT_EQ(-1, value);
  EXPECT_EQ(3u, ErrorHandler::resolve_errors.size());
  EXPECT_NE(std::string::npos, ErrorHandler::resolve_errors.back().find(
                                   "path 'no.such.again'"));

  view.Acquire<Required>("d"_path, value, -1);  // double 不是 int
  EXPECT_EQ(3u, ErrorHandler::mismatches.size());
  EXPECT_EQ(std::string("d"), ErrorHandler::mismatches.back());

  // 默认 handler 下抛异常
  JsonView<SC_JSON_TYPE> strict(json);
  // 注意：模板实参里的逗号会切断宏参数，故整体再包一层括号
  EXPECT_THROW(
      ((void)strict.Acquire<JsonInt, Required>("no.such.path"_path, value)),
      std::runtime_error);
  EXPECT_THROW(((void)strict.Acquire<JsonInt, Required>("s"_path, value)),
               std::runtime_error);
}

TEST(SC_JSON_TEST_SUITE_NAME, AcquireReturnsSelfForChaining) {
  const SC_JSON_TYPE json = SC_JSON_MAKE_JSON();
  JsonView<SC_JSON_TYPE> view(json);

  int i = 0;
  std::string s;
  bool flag = false;
  JsonView<SC_JSON_TYPE>& ref =
      view.Acquire<JsonInt, Required>("a.b"_path, i)
          .Acquire<JsonString, Required>("s"_path, s)
          .Acquire<JsonBool, Required>("flag"_path, flag);
  EXPECT_EQ(&ref, &view);
  EXPECT_EQ(42, i);
  EXPECT_EQ(std::string("hello"), s);
  EXPECT_TRUE(flag);

  // 推导形式同样可链式调用，且与显式形式混用
  int deduced_i = 0;
  JsonView<SC_JSON_TYPE>& deduced_ref =
      view.Acquire("a.b"_path, deduced_i).Acquire<JsonString, Required>("s"_path, s);
  EXPECT_EQ(&deduced_ref, &view);
  EXPECT_EQ(42, deduced_i);
}

TEST(SC_JSON_TEST_SUITE_NAME, AcquireArrayAndMap) {
  const SC_JSON_TYPE json = SC_JSON_MAKE_JSON();
  JsonView<SC_JSON_TYPE> view(json);

  std::vector<int> nums;
  view.AcquireArray<JsonInt, Required>("list"_path, nums);
  EXPECT_EQ((std::vector<int>{1, 2, 3}), nums);

  std::map<std::string, int> point;
  view.AcquireMap<JsonInt, Required>("point"_path, point);
  ASSERT_EQ(2u, point.size());
  EXPECT_EQ(10, point["x"]);
  EXPECT_EQ(20, point["y"]);

  // Optional：缺失时得到空容器
  std::vector<int> empty_nums = {9};
  view.AcquireArray<JsonInt>("no.such"_path, empty_nums);
  EXPECT_TRUE(empty_nums.empty());

  std::map<std::string, int> empty_map;
  view.AcquireMap<JsonInt>("no.such"_path, empty_map);
  EXPECT_TRUE(empty_map.empty());

  // 推导形式：元素类型由容器决定
  std::vector<int> deduced_nums;
  view.AcquireArray("list"_path, deduced_nums);
  EXPECT_EQ((std::vector<int>{1, 2, 3}), deduced_nums);

  std::map<std::string, int> deduced_point;
  view.AcquireMap("point"_path, deduced_point);
  ASSERT_EQ(2u, deduced_point.size());
  EXPECT_EQ(10, deduced_point["x"]);
  EXPECT_EQ(20, deduced_point["y"]);

  // 推导形式下 ExistencePolicy 仍是第一个模板实参
  std::vector<int> required_nums;
  view.AcquireArray<Required>("list"_path, required_nums);
  EXPECT_EQ((std::vector<int>{1, 2, 3}), required_nums);

  std::map<std::string, int> required_point;
  view.AcquireMap<Required>("point"_path, required_point);
  EXPECT_EQ(2u, required_point.size());
}

TEST(SC_JSON_TEST_SUITE_NAME, AcquireArrayElementMismatchReportsFullPath) {
  using ErrorHandler = SC_JSON_CONCAT(ErrorHandler_, SC_JSON_TEST_SUITE_NAME);
  ErrorHandler handler;
  ErrorHandler::mismatches.clear();

  const SC_JSON_TYPE json = SC_JSON_MAKE_JSON();
  JsonView<SC_JSON_TYPE, ErrorHandler> view(json, handler);

  std::vector<int> arr;
  view.AcquireArray<JsonInt, Required>("obj_list"_path, arr);  // 元素是对象，不是 int
  ASSERT_EQ(1u, ErrorHandler::mismatches.size());
  EXPECT_EQ(std::string("obj_list[0]"), ErrorHandler::mismatches.front());
  EXPECT_TRUE(arr.empty());
}

TEST(SC_JSON_TEST_SUITE_NAME, AcquireMapValueMismatchReportsFullPath) {
  using ErrorHandler = SC_JSON_CONCAT(ErrorHandler_, SC_JSON_TEST_SUITE_NAME);
  ErrorHandler handler;
  ErrorHandler::mismatches.clear();

  const SC_JSON_TYPE json = SC_JSON_MAKE_JSON();
  JsonView<SC_JSON_TYPE, ErrorHandler> view(json, handler);

  std::map<std::string, std::string> map;
  view.AcquireMap<JsonString, Required>("point"_path, map);  // 值是 int，不是 string
  ASSERT_EQ(1u, ErrorHandler::mismatches.size());
  EXPECT_EQ(std::string("point."), ErrorHandler::mismatches.front().substr(0, 6));
  EXPECT_TRUE(map.empty());
}

TEST(SC_JSON_TEST_SUITE_NAME, AcquireCustomExtractor) {
  using Point = SC_JSON_CONCAT(Point_, SC_JSON_TEST_SUITE_NAME);
  const SC_JSON_TYPE json = SC_JSON_MAKE_JSON();
  JsonView<SC_JSON_TYPE> view(json);

  Point point;
  view.Acquire<SC_JSON_CONCAT(PointExtractor_, SC_JSON_TEST_SUITE_NAME), Required>(
      "point"_path, point);
  EXPECT_EQ(10, point.x);
  EXPECT_EQ(20, point.y);

  // 自定义提取器不匹配时同样走类型不符分支
  Point fallback;
  view.Acquire<SC_JSON_CONCAT(PointExtractor_, SC_JSON_TEST_SUITE_NAME)>(
      "list"_path, fallback, Point{1, 2});
  EXPECT_EQ(1, fallback.x);
  EXPECT_EQ(2, fallback.y);

  // 推导形式会命中 Extractor<Json, Point> 的特化
  Point deduced;
  view.Acquire("point"_path, deduced);
  EXPECT_EQ(10, deduced.x);
  EXPECT_EQ(20, deduced.y);

  Point deduced_fallback;
  view.Acquire("list"_path, deduced_fallback, Point{1, 2});
  EXPECT_EQ(1, deduced_fallback.x);
  EXPECT_EQ(2, deduced_fallback.y);
}

TEST(SC_JSON_TEST_SUITE_NAME, AcquireWithRuntimePath) {
  const SC_JSON_TYPE json = SC_JSON_MAKE_JSON();
  JsonView<SC_JSON_TYPE> view(json);

  const std::string key = "a";
  const RuntimePath path(key + ".b");
  int value = 0;
  view.Acquire<JsonInt, Required>(path, value);
  EXPECT_EQ(42, value);

  int deduced = 0;
  view.Acquire<Required>(path, deduced);
  EXPECT_EQ(42, deduced);
}

TEST(SC_JSON_TEST_SUITE_NAME, TraitsOpsOnView) {
  const SC_JSON_TYPE json = SC_JSON_MAKE_JSON();
  JsonView<SC_JSON_TYPE> view(json);

  EXPECT_TRUE(view.IsObject());
  EXPECT_FALSE(view.IsArray());

  const SC_JSON_TYPE* a = view.TryGetChild("a");
  ASSERT_NE(nullptr, a);
  EXPECT_TRUE(view.TryGetChild("no.such") == nullptr);
  EXPECT_EQ(nullptr, view.TryGetChild("no.such"));

  std::size_t members = 0;
  for (auto it = view.MemberBegin(); it != view.MemberEnd(); ++it) {
    ++members;
  }
  EXPECT_GT(members, 0u);

  auto list = view.View<Required>("list"_path);
  EXPECT_TRUE(list.IsArray());
  EXPECT_EQ(3u, list.GetArraySize());
  EXPECT_FALSE(list.IsInt());
  EXPECT_EQ(2, JsonTraits<SC_JSON_TYPE>::GetInt(list.GetArrayElement(1)));
}

TEST(SC_JSON_TEST_SUITE_NAME, TraitsOpsOnInvalidViewAreSafe) {
  const SC_JSON_TYPE json = SC_JSON_MAKE_JSON();
  JsonView<SC_JSON_TYPE> view(json);
  auto invalid = view.View("no.such"_path);
  ASSERT_FALSE(invalid.valid());

  EXPECT_FALSE(invalid.IsObject());
  EXPECT_FALSE(invalid.IsArray());
  EXPECT_FALSE(invalid.IsString());
  EXPECT_EQ(nullptr, invalid.TryGetChild("a"));
  EXPECT_EQ(0u, invalid.GetArraySize());
  EXPECT_EQ(0, invalid.GetInt());
  EXPECT_EQ(0.0, invalid.GetDouble());
  EXPECT_TRUE(invalid.GetString().empty());
  EXPECT_FALSE(invalid.GetBool());
  EXPECT_EQ(invalid.MemberBegin(), invalid.MemberEnd());
  EXPECT_FALSE(JsonTraits<SC_JSON_TYPE>::IsObject(invalid.GetArrayElement(0)));
}

// ---------------- 运行期路径 ----------------

TEST(SC_JSON_TEST_SUITE_NAME, RuntimePathResolvesDynamicPath) {
  const SC_JSON_TYPE json = SC_JSON_MAKE_JSON();
  JsonView<SC_JSON_TYPE> view(json);

  const std::string prefix = "a";
  const RuntimePath path(prefix + ".b");
  EXPECT_EQ(42, view.View<Required>(path).GetInt());

  const RuntimePath indexed(std::string("obj_list[1].id"));
  ASSERT_EQ(3u, indexed.Size());
  EXPECT_EQ(2, view.View<Required>(indexed).GetInt());
}

TEST(SC_JSON_TEST_SUITE_NAME, RuntimePathInvalidReportsResolveError) {
  using ErrorHandler = SC_JSON_CONCAT(ErrorHandler_, SC_JSON_TEST_SUITE_NAME);
  ErrorHandler handler;
  ErrorHandler::resolve_errors.clear();

  const SC_JSON_TYPE json = SC_JSON_MAKE_JSON();
  JsonView<SC_JSON_TYPE, ErrorHandler> view(json, handler);

  // 编译期字面量会直接编译失败；运行期路径改由 OnPathResolveError 报告
  const RuntimePath empty_segment("a..b");
  EXPECT_FALSE(view.View<Required>(empty_segment).valid());
  ASSERT_EQ(1u, ErrorHandler::resolve_errors.size());
  EXPECT_NE(std::string::npos,
            ErrorHandler::resolve_errors.front().find("path 'a..b'"));

  const RuntimePath unclosed("a[1");
  EXPECT_FALSE(view.View<Required>(unclosed).valid());
  EXPECT_EQ(2u, ErrorHandler::resolve_errors.size());

  // 默认 handler 下同样报错（抛异常）
  JsonView<SC_JSON_TYPE> strict(json);
  EXPECT_THROW((void)strict.View<Required>(unclosed), std::runtime_error);
}

// ---------------- 错误处理器 ----------------

TEST(SC_JSON_TEST_SUITE_NAME, StatefulErrorHandlerCarriesExternalData) {
  using Exception = SC_JSON_CONCAT(ContextException_, SC_JSON_TEST_SUITE_NAME);
  using Handler = SC_JSON_CONCAT(ContextErrorHandler_, SC_JSON_TEST_SUITE_NAME);

  const SC_JSON_TYPE json = SC_JSON_MAKE_JSON();
  // handler 按值持有：不需要左值，直接传临时对象也能把外部数据带进异常
  JsonView<SC_JSON_TYPE, Handler> view(json, Handler("req-001"));

  try {
    (void)view.View<Required>("no.such.path"_path);
    FAIL() << "expected ContextException";
  } catch (const Exception& e) {
    EXPECT_EQ(std::string("req-001"), e.RequestId());
    EXPECT_NE(std::string::npos, e.Message().find("no.such.path"));
  }

  try {
    (void)view.ViewArray<Required>("point"_path);
    FAIL() << "expected ContextException";
  } catch (const Exception& e) {
    EXPECT_EQ(std::string("req-001"), e.RequestId());
    EXPECT_EQ(std::string("point"), e.Message());
  }

  // 子视图持有 handler 的副本，外部数据随副本一起带过去
  try {
    (void)view.View<Required>("point"_path).View<Required>("no.such"_path);
    FAIL() << "expected ContextException";
  } catch (const Exception& e) {
    EXPECT_EQ(std::string("req-001"), e.RequestId());
    EXPECT_NE(std::string::npos, e.Message().find("no.such"));
  }
}

TEST(SC_JSON_TEST_SUITE_NAME, RecordingHandlerKeepsGoingAfterError) {
  using ErrorHandler = SC_JSON_CONCAT(ErrorHandler_, SC_JSON_TEST_SUITE_NAME);
  ErrorHandler handler;
  ErrorHandler::resolve_errors.clear();
  ErrorHandler::mismatches.clear();

  const SC_JSON_TYPE json = SC_JSON_MAKE_JSON();
  JsonView<SC_JSON_TYPE, ErrorHandler> view(json, handler);

  EXPECT_FALSE(view.View<Required>("no.such.path"_path).valid());
  EXPECT_TRUE(view.ViewArray<Required>("point"_path).empty());
  EXPECT_EQ(1u, ErrorHandler::resolve_errors.size());
  EXPECT_EQ(1u, ErrorHandler::mismatches.size());

  // handler 返回后 view 仍可继续使用
  EXPECT_EQ(42, view.View<Required>("a.b"_path).GetInt());
}
