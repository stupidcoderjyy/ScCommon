// 每个 JSON 库的测试文件通过先定义以下宏、再包含本文件来复用同一套用例：
//   SC_JSON_TEST_SUITE_NAME   测试套件名（如 JsonViewNlohmann）
//   SC_JSON_TYPE              JSON 库类型（如 nlohmann::json）
//   SC_JSON_MAKE_JSON()       构造包含测试数据的 JSON 文档
//   SC_JSON_MAKE_JSON_LEAF()  构造根节点为整数的 JSON 文档
#pragma once
#include <gtest/gtest.h>

#include <exception>
#include <iterator>
#include <map>
#include <optional>
#include <ranges>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "json/json.hpp"

using namespace sc::json;

// 迭代器能力在编译期固化：补 -> 之后这些概念仍然成立
static_assert(std::random_access_iterator<JsonArrayView<SC_JSON_TYPE>::iterator>);
static_assert(std::ranges::random_access_range<JsonArrayView<SC_JSON_TYPE>>);
static_assert(std::ranges::sized_range<JsonArrayView<SC_JSON_TYPE>>);
static_assert(std::forward_iterator<JsonObjectView<SC_JSON_TYPE>::iterator>);
static_assert(std::ranges::forward_range<JsonObjectView<SC_JSON_TYPE>>);

#define SC_JSON_CONCAT_IMPL(a, b) a##b
#define SC_JSON_CONCAT(a, b) SC_JSON_CONCAT_IMPL(a, b)

// 记录型错误处理器：不抛异常，只记录
struct SC_JSON_CONCAT(ErrorHandler_, SC_JSON_TEST_SUITE_NAME) {
  static inline std::vector<std::string> resolve_errors;
  static inline std::vector<std::string> mismatches;
  static inline std::vector<std::string> parse_errors;

  static void OnPathResolveError(std::string_view path) {
    resolve_errors.emplace_back(path);
  }
  static void OnTypeMismatch(std::string_view path) {
    mismatches.emplace_back(path);
  }
  static void OnJsonParseError(std::string_view reason) {
    parse_errors.emplace_back(reason);
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

  [[nodiscard]] const std::string& RequestId() const noexcept { return request_id_; }
  [[nodiscard]] const std::string& Message() const noexcept { return message_; }

 private:
  std::string request_id_;
  std::string message_;
};

struct SC_JSON_CONCAT(ContextErrorHandler_, SC_JSON_TEST_SUITE_NAME) {
  // JsonView 按值持有 handler（成员默认初始化 + JsonView{}），故需可默认构造
  explicit SC_JSON_CONCAT(ContextErrorHandler_, SC_JSON_TEST_SUITE_NAME)(
      std::string request_id = {})
      : request_id_(std::move(request_id)) {}

  void OnPathResolveError(std::string_view path) const {
    throw SC_JSON_CONCAT(ContextException_, SC_JSON_TEST_SUITE_NAME)(
        request_id_, std::string(path));
  }
  void OnTypeMismatch(std::string_view path) const {
    throw SC_JSON_CONCAT(ContextException_, SC_JSON_TEST_SUITE_NAME)(
        request_id_, std::string(path));
  }
  void OnJsonParseError(std::string_view reason) const {
    throw SC_JSON_CONCAT(ContextException_, SC_JSON_TEST_SUITE_NAME)(
        request_id_, std::string(reason));
  }

 private:
  std::string request_id_;
};

template <typename T>
T TraitGet(const SC_JSON_TYPE& node) {
  T out{};
  if constexpr (std::is_same_v<T, int>) {
    JsonTraits<SC_JSON_TYPE>::GetInt(node, out);
  } else if constexpr (std::is_same_v<T, unsigned int>) {
    JsonTraits<SC_JSON_TYPE>::GetUint(node, out);
  } else if constexpr (std::is_same_v<T, std::int64_t>) {
    JsonTraits<SC_JSON_TYPE>::GetInt64(node, out);
  } else if constexpr (std::is_same_v<T, std::uint64_t>) {
    JsonTraits<SC_JSON_TYPE>::GetUint64(node, out);
  } else if constexpr (std::is_same_v<T, double>) {
    JsonTraits<SC_JSON_TYPE>::GetDouble(node, out);
  } else if constexpr (std::is_same_v<T, std::string>) {
    JsonTraits<SC_JSON_TYPE>::GetString(node, out);
  } else if constexpr (std::is_same_v<T, bool>) {
    JsonTraits<SC_JSON_TYPE>::GetBool(node, out);
  }
  return out;
}

// 自定义类型 + 自定义提取器（特化 Extractor<JsonType, MyType>）
struct SC_JSON_CONCAT(InnerPoint_, SC_JSON_TEST_SUITE_NAME) {
  int x = 0;
  int y = 0;
};

template <>
struct sc::json::Extractor<SC_JSON_TYPE, SC_JSON_CONCAT(InnerPoint_, SC_JSON_TEST_SUITE_NAME)> {
  using ValueType = SC_JSON_CONCAT(InnerPoint_, SC_JSON_TEST_SUITE_NAME);

  static bool Extract(const SC_JSON_TYPE& json, ValueType& out) {
    const SC_JSON_TYPE* x = JsonTraits<SC_JSON_TYPE>::TryGetChild(json, "x");
    const SC_JSON_TYPE* y = JsonTraits<SC_JSON_TYPE>::TryGetChild(json, "y");
    if (x == nullptr || y == nullptr) {
      return false;
    }
    out.x = TraitGet<int>(*x);
    out.y = TraitGet<int>(*y);
    return true;
  }
};

// ---------------- 构造 ----------------

TEST(SC_JSON_TEST_SUITE_NAME, ViewFromNode) {
  const SC_JSON_TYPE json = SC_JSON_MAKE_JSON();
  JsonView view(json);
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
  JsonOwner<SC_JSON_TYPE> owner(std::string_view(R"({"a":{"b":42},"s":"hi"})"));
  JsonView view(owner);
  EXPECT_TRUE(view.IsObject());
  EXPECT_EQ(42, view.View<Required>("a.b"_path).GetInt());
  EXPECT_EQ(std::string("hi"), view.View<Required>("s"_path).GetString());
}

TEST(SC_JSON_TEST_SUITE_NAME, ViewFromInvalidTextThrows) {
  EXPECT_THROW(
      (JsonOwner<SC_JSON_TYPE>(std::string_view("{oops"))),
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
      (JsonOwner<SC_JSON_TYPE, ErrorHandler>(std::string_view("{oops"), handler)),
      std::exception);
  EXPECT_EQ(1u, ErrorHandler::parse_errors.size());
}

// std::string 实参按“文本”解析：JsonOwner 的文本构造需显式取 string_view，
// 否则会被 Json 值类型的转换构造函数抢走。
TEST(SC_JSON_TEST_SUITE_NAME, ViewFromStdString) {
  const std::string text = R"({"a":{"b":42},"s":"hi"})";
  JsonOwner<SC_JSON_TYPE> owner{std::string_view(text)};
  JsonView view(owner);
  EXPECT_TRUE(view.valid());
  EXPECT_TRUE(view.IsObject());  // 若被当成节点，这里会是字符串节点
  EXPECT_EQ(42, view.View<Required>("a.b"_path).GetInt());
  EXPECT_EQ(std::string("hi"), view.View<Required>("s"_path).GetString());

  int b = 0;
  view.Put<int, Required>("a.b"_path, b);
  EXPECT_EQ(42, b);
}

TEST(SC_JSON_TEST_SUITE_NAME, ViewFromTemporaryStdString) {
  // 右值：临时 string 只在构造期间被读取，解析结果由 owner 自己持有
  JsonOwner<SC_JSON_TYPE> owner(
      std::string_view(std::string(R"({"a":{"b":42}})")));
  JsonView view(owner);
  EXPECT_TRUE(view.valid());
  EXPECT_EQ(42, view.View<Required>("a.b"_path).GetInt());
}

TEST(SC_JSON_TEST_SUITE_NAME, ViewFromInvalidStdStringThrows) {
  const std::string bad = "{oops";
  EXPECT_THROW((JsonOwner<SC_JSON_TYPE>(std::string_view(bad))),
               std::runtime_error);
}

TEST(SC_JSON_TEST_SUITE_NAME, ViewFromStdStringUsesGivenHandler) {
  using Exception = SC_JSON_CONCAT(ContextException_, SC_JSON_TEST_SUITE_NAME);
  using Handler = SC_JSON_CONCAT(ContextErrorHandler_, SC_JSON_TEST_SUITE_NAME);

  const std::string bad = "{oops";
  Handler handler("req-003");
  try {
    JsonOwner<SC_JSON_TYPE, Handler> owner(std::string_view(bad), handler);
    FAIL() << "expected ContextException";
  } catch (const Exception& e) {
    EXPECT_EQ(std::string("req-003"), e.RequestId());
    EXPECT_FALSE(e.Message().empty());
  }
}

// 输入流构造：解析结果由 owner 持有，流只在构造期间被读取
TEST(SC_JSON_TEST_SUITE_NAME, ViewFromStream) {
  std::istringstream input(R"({"a":{"b":42},"s":"hi"})");
  JsonOwner<SC_JSON_TYPE> owner(input);
  JsonView view(owner);
  EXPECT_TRUE(view.valid());
  EXPECT_TRUE(view.IsObject());
  EXPECT_EQ(42, view.View<Required>("a.b"_path).GetInt());
  EXPECT_EQ(std::string("hi"), view.View<Required>("s"_path).GetString());

  int b = 0;
  view.Put<int, Required>("a.b"_path, b);
  EXPECT_EQ(42, b);
}

TEST(SC_JSON_TEST_SUITE_NAME, ViewFromMultilineStream) {
  std::istringstream input(R"({
    "a": { "b": 42 },
    "list": [1, 2, 3]
  })");
  JsonOwner<SC_JSON_TYPE> owner(input);
  JsonView view(owner);
  EXPECT_TRUE(view.valid());
  EXPECT_EQ(42, view.View<Required>("a.b"_path).GetInt());

  auto list = view.ViewArray<Required>("list"_path);
  EXPECT_EQ(3u, list.size());
  EXPECT_EQ(2, (*(list.begin() + 1)).GetInt());
}

TEST(SC_JSON_TEST_SUITE_NAME, OwnerOutlivesStream) {
  // 流销毁后 owner 仍然有效：解析结果的所有权在 owner 手里
  JsonOwner<SC_JSON_TYPE> owner = [] {
    std::istringstream input(R"({"a":{"b":42}})");
    return JsonOwner<SC_JSON_TYPE>(input);
  }();
  JsonView view(owner);
  EXPECT_TRUE(view.valid());
  EXPECT_EQ(42, view.View<Required>("a.b"_path).GetInt());
}

TEST(SC_JSON_TEST_SUITE_NAME, ParserFromStream) {
  std::istringstream input(R"({"a":{"b":42}})");
  JsonOwner<SC_JSON_TYPE> owner(input);
  JsonView view(owner);
  EXPECT_EQ(42, view.View<Required>("a.b"_path).GetInt());
}

TEST(SC_JSON_TEST_SUITE_NAME, ViewFromInvalidStreamThrows) {
  std::istringstream input("{oops");
  EXPECT_THROW((JsonOwner<SC_JSON_TYPE>(input)), std::runtime_error);
}

TEST(SC_JSON_TEST_SUITE_NAME, ViewFromInvalidStreamCallsHandler) {
  using ErrorHandler = SC_JSON_CONCAT(ErrorHandler_, SC_JSON_TEST_SUITE_NAME);
  ErrorHandler::parse_errors.clear();

  // 与文本构造一致：处理器被调用是核心约定；处理器吞掉异常后各库抛出的具体
  // 异常类型不完全一致，所以这里只要求抛出 std::exception。
  std::istringstream input("{oops");
  ErrorHandler handler;
  EXPECT_THROW((JsonOwner<SC_JSON_TYPE, ErrorHandler>(input, handler)),
               std::exception);
  EXPECT_EQ(1u, ErrorHandler::parse_errors.size());
}

TEST(SC_JSON_TEST_SUITE_NAME, ViewFromStreamUsesGivenHandler) {
  using Exception = SC_JSON_CONCAT(ContextException_, SC_JSON_TEST_SUITE_NAME);
  using Handler = SC_JSON_CONCAT(ContextErrorHandler_, SC_JSON_TEST_SUITE_NAME);

  std::istringstream input("{oops");
  Handler handler("req-004");
  try {
    JsonOwner<SC_JSON_TYPE, Handler> owner(input, handler);
    FAIL() << "expected ContextException";
  } catch (const Exception& e) {
    EXPECT_EQ(std::string("req-004"), e.RequestId());
    EXPECT_FALSE(e.Message().empty());
  }
}

// ---------------- 路径解析 ----------------

TEST(SC_JSON_TEST_SUITE_NAME, ViewResolvesPath) {
  const SC_JSON_TYPE json = SC_JSON_MAKE_JSON();
  JsonView view(json);

  EXPECT_EQ(42, view.View<Required>("a.b"_path).GetInt());
  EXPECT_EQ(-7, view.View<Required>("neg"_path).GetInt());
  EXPECT_EQ(9007199254740993LL, view.View<Required>("big"_path).GetInt64());
  EXPECT_EQ(std::string("hello"), view.View<Required>("s"_path).GetString());
  EXPECT_DOUBLE_EQ(3.14, view.View<Required>("d"_path).GetDouble());
  EXPECT_TRUE(view.View<Required>("flag"_path).GetBool());
}

TEST(SC_JSON_TEST_SUITE_NAME, ViewResolvesArrayIndex) {
  const SC_JSON_TYPE json = SC_JSON_MAKE_JSON();
  JsonView view(json);

  EXPECT_EQ(2, view.View<Required>("list[1]"_path).GetInt());
  EXPECT_EQ(2, view.View<Required>("obj_list[1].id"_path).GetInt());
}

TEST(SC_JSON_TEST_SUITE_NAME, ViewEmptyPathIsCurrentNode) {
  const SC_JSON_TYPE leaf = SC_JSON_MAKE_JSON_LEAF();
  JsonView view(leaf);
  EXPECT_EQ(42, view.View<Required>(""_path).GetInt());
}

TEST(SC_JSON_TEST_SUITE_NAME, ViewOptionalMissingGivesInvalidView) {
  const SC_JSON_TYPE json = SC_JSON_MAKE_JSON();
  JsonView view(json);

  auto missing = view.View("no.such.path"_path);
  EXPECT_FALSE(missing.valid());
  EXPECT_EQ(0, missing.GetInt());  // 无效 view 上取值返回默认值
}

TEST(SC_JSON_TEST_SUITE_NAME, ViewRequiredMissingThrows) {
  const SC_JSON_TYPE json = SC_JSON_MAKE_JSON();
  JsonView view(json);
  EXPECT_THROW((void)view.View<Required>("no.such.path"_path), std::runtime_error);
  EXPECT_THROW((void)view.View<Required>("list[9]"_path), std::runtime_error);
  EXPECT_THROW((void)view.View<Required>("list.b"_path), std::runtime_error);
}

TEST(SC_JSON_TEST_SUITE_NAME, NestedViewsAreRelative) {
  const SC_JSON_TYPE json = SC_JSON_MAKE_JSON();
  JsonView view(json);

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
  JsonView view(json);

  auto range = view.ViewArray<Required>("list"_path);
  EXPECT_FALSE(range.empty());
  EXPECT_EQ(3u, range.size());

  std::vector<int> values;
  for (auto&& element : range) {
    values.push_back(element.GetInt());
  }
  EXPECT_EQ((std::vector{1, 2, 3}), values);

  // 随机访问
  EXPECT_EQ(2, (*(range.begin() + 1)).GetInt());
  EXPECT_EQ(3, static_cast<int>(range.end() - range.begin()));
}

TEST(SC_JSON_TEST_SUITE_NAME, ArrayViewEmptyOnMissingOptional) {
  const SC_JSON_TYPE json = SC_JSON_MAKE_JSON();
  JsonView view(json);

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
            ErrorHandler::resolve_errors.front().find("no.such"));

  // 节点存在但不是数组 → OnTypeMismatch
  EXPECT_TRUE(view.ViewArray<Required>("point"_path).empty());
  ASSERT_EQ(1u, ErrorHandler::mismatches.size());
  EXPECT_EQ(std::string("point"), ErrorHandler::mismatches.front());
}

TEST(SC_JSON_TEST_SUITE_NAME, ArrayViewComposesWithRanges) {
  const SC_JSON_TYPE json = SC_JSON_MAKE_JSON();
  JsonView view(json);

  // transform：取每个元素的 id
  std::vector<int> ids;
  auto id_view = view.ViewArray("obj_list"_path) |
                 std::views::transform([](auto&& element) {
                   return element.View("id"_path).GetInt();
                 });
  for (int id : id_view) {
    ids.push_back(id);
  }
  EXPECT_EQ((std::vector{1, 2}), ids);

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
  JsonView view(json);

  auto range = view.ViewArray<Required>("list"_path);
  // 元素指向底层文档中的节点（零拷贝）
  const SC_JSON_TYPE* list_node = JsonTraits<SC_JSON_TYPE>::TryGetChild(json, "list");
  ASSERT_NE(nullptr, list_node);
  EXPECT_EQ((*(range.begin() + 1)).node(),
            &JsonTraits<SC_JSON_TYPE>::GetArrayElement(*list_node, 1));
}

TEST(SC_JSON_TEST_SUITE_NAME, ArrayIteratorSupportsArrow) {
  const SC_JSON_TYPE json = SC_JSON_MAKE_JSON();
  JsonView view(json);

  auto range = view.ViewArray<Required>("list"_path);
  EXPECT_EQ(1, range.begin()->GetInt());
  EXPECT_EQ(2, (range.begin() + 1)->GetInt());
  EXPECT_EQ(3, (range.end() - 1)->GetInt());
  // operator[] 直接返回元素值（不是迭代器），所以用 . 而不是 ->
  EXPECT_EQ(2, range.begin()[1].GetInt());

  // 随机访问的三件套：-=、it - n、n + it
  auto last = range.end();
  last -= 1;
  EXPECT_EQ(3, last->GetInt());
  const std::ptrdiff_t back = 2;
  EXPECT_EQ(2, (range.end() - back)->GetInt());
  EXPECT_EQ(1, (0 + range.begin())->GetInt());

  // 用 -> 遍历
  int sum = 0;
  for (auto it = range.begin(); it != range.end(); ++it) {
    sum += it->GetInt();
  }
  EXPECT_EQ(6, sum);

  // 元素是对象时，-> 上继续链式调用
  auto objects = view.ViewArray<Required>("obj_list"_path);
  EXPECT_EQ(1, objects.begin()->View<Required>("id"_path).GetInt());
  EXPECT_EQ(2, (objects.begin() + 1)->View<Required>("id"_path).GetInt());
}

TEST(SC_JSON_TEST_SUITE_NAME, ArrowPutOnArrayElements) {
  const SC_JSON_TYPE json = SC_JSON_MAKE_JSON();
  JsonView view(json);
  auto objects = view.ViewArray<Required>("obj_list"_path);

  // 典型场景：pkg_list_view.begin()->Put("id"_path, result, kInvalidId)
  // -> 返回的是可变视图，非 const 的 Put 才能调用
  std::int64_t id = 0;
  const long long kInvalidId = -1;
  objects.begin()->Put("id"_path, id, kInvalidId);
  EXPECT_EQ(1, id);

  // 随机访问 + 显式提取器形式
  (objects.begin() + 1)->Put<std::int64_t, Required>("id"_path, id);
  EXPECT_EQ(2, id);

  // 默认值类型与 out 不同（int 字面量 → int64_t）：T 只由 out 推导
  std::int64_t zero = 5;
  objects.begin()->Put("no.such"_path, zero, 0);
  EXPECT_EQ(0, zero);

  std::int64_t fallback = 5;
  objects.begin()->Put("no.such"_path, fallback, kInvalidId);
  EXPECT_EQ(kInvalidId, fallback);

  // Required + 类型不符：默认 handler 抛异常
  std::string text;
  EXPECT_THROW(
      ((void)objects.begin()->Put<std::string, Required>("id"_path, text)),
      std::runtime_error);
}

// ---------------- 对象视图 ----------------

TEST(SC_JSON_TEST_SUITE_NAME, ObjectViewStructuredBinding) {
  const SC_JSON_TYPE json = SC_JSON_MAKE_JSON();
  JsonView view(json);

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

TEST(SC_JSON_TEST_SUITE_NAME, ObjectIteratorSupportsArrow) {
  const SC_JSON_TYPE json = SC_JSON_MAKE_JSON();
  JsonView view(json);

  auto object = view.ViewObject<Required>("point"_path);

  // it->first / it->second（顺序无关，用 map 收结果）
  std::map<std::string, int> values;
  for (auto it = object.begin(); it != object.end(); ++it) {
    values.emplace(std::string(it->first), it->second.GetInt());
  }
  EXPECT_EQ(2u, values.size());
  EXPECT_EQ(10, values["x"]);
  EXPECT_EQ(20, values["y"]);

  // 空区间上 begin == end，且解引用前比较是安全的
  auto empty = view.ViewObject("no.such"_path);
  EXPECT_EQ(empty.begin(), empty.end());

  // 结构化绑定（operator*）不受影响
  int sum = 0;
  for (auto&& [key, value] : object) {
    EXPECT_FALSE(key.empty());
    sum += value.GetInt();
  }
  EXPECT_EQ(30, sum);
}

// ---------------- JsonTraits 操作（JsonView 直通）----------------

// ---------------- 变量访问器 Put / Get 系列 ----------------
// Put<类型, 策略>(path, out[, default])：写入 out，类型可省（由 out 推导）
// Get<类型, 策略>(path[, default])  ：直接返回数据

TEST(SC_JSON_TEST_SUITE_NAME, PutScalarsWithExplicitType) {
  const SC_JSON_TYPE json = SC_JSON_MAKE_JSON();
  JsonView view(json);

  int i = 0;
  view.Put("a.b"_path, i);
  EXPECT_EQ(42, i);

  int neg = 0;
  view.Put("neg"_path, neg);
  EXPECT_EQ(-7, neg);

  std::int64_t big = 0;
  view.Put("big"_path, big);
  EXPECT_EQ(9007199254740993LL, big);

  std::string s;
  view.Put("s"_path, s);
  EXPECT_EQ(std::string("hello"), s);

  double d = 0;
  view.Put("d"_path, d);
  EXPECT_DOUBLE_EQ(3.14, d);

  bool flag = false;
  view.Put("flag"_path, flag);
  EXPECT_TRUE(flag);
}

TEST(SC_JSON_TEST_SUITE_NAME, PutScalarsByTypeDeduction) {
  const SC_JSON_TYPE json = SC_JSON_MAKE_JSON();
  JsonView view(json);

  int i = 0;
  view.Put("a.b"_path, i);  // 提取器由 out 类型推导
  EXPECT_EQ(42, i);

  std::int64_t big = 0;
  view.Put("big"_path, big);
  EXPECT_EQ(9007199254740993LL, big);

  std::string s;
  view.Put("s"_path, s);
  EXPECT_EQ(std::string("hello"), s);

  double d = 0;
  view.Put("d"_path, d);
  EXPECT_DOUBLE_EQ(3.14, d);

  bool flag = false;
  view.Put("flag"_path, flag);
  EXPECT_TRUE(flag);

  // 推导形式下，唯一的模板实参是 ExistencePolicy
  int required = 0;
  view.Put<int, Required>("a.b"_path, required, -1);
  EXPECT_EQ(42, required);
}

TEST(SC_JSON_TEST_SUITE_NAME, PutOptionalWritesDefault) {
  const SC_JSON_TYPE json = SC_JSON_MAKE_JSON();
  JsonView view(json);

  // 路径缺失 → 默认值
  int missing = 0;
  view.Put("no.such.path"_path, missing, 7);
  EXPECT_EQ(7, missing);

  // 类型不符 → 默认值
  int mismatch = 0;
  view.Put("s"_path, mismatch, -1);
  EXPECT_EQ(-1, mismatch);

  // 不传默认值 → 值初始化
  int zero = 123;
  view.Put("no.such.path"_path, zero);
  EXPECT_EQ(0, zero);

  // 路径存在且类型匹配 → 实际值
  int actual = 0;
  view.Put("a.b"_path, actual, 7);
  EXPECT_EQ(42, actual);

  // 无效 view 上的 Put 等同 Optional（即使策略是 Required，也没有节点可解析）
  auto invalid = view.View("no.such"_path);
  int from_invalid = 5;
  invalid.Put<int, Required>("a.b"_path, from_invalid, -1);
  EXPECT_EQ(-1, from_invalid);

  // 推导形式的默认值语义与显式形式一致
  int deduced_missing = 0;
  view.Put("no.such.path"_path, deduced_missing, 7);
  EXPECT_EQ(7, deduced_missing);

  int deduced_mismatch = 0;
  view.Put("s"_path, deduced_mismatch, -1);
  EXPECT_EQ(-1, deduced_mismatch);

  int deduced_actual = 0;
  view.Put("a.b"_path, deduced_actual, 7);
  EXPECT_EQ(42, deduced_actual);
}

TEST(SC_JSON_TEST_SUITE_NAME, PutRequiredReportsErrors) {
  using ErrorHandler = SC_JSON_CONCAT(ErrorHandler_, SC_JSON_TEST_SUITE_NAME);
  ErrorHandler handler;
  ErrorHandler::resolve_errors.clear();
  ErrorHandler::mismatches.clear();

  const SC_JSON_TYPE json = SC_JSON_MAKE_JSON();
  JsonView<SC_JSON_TYPE, ErrorHandler> view(json, handler);

  int value = 0;
  // 路径缺失 → OnPathResolveError（handler 返回时写默认值）
  view.Put<int, Required>("no.such.path"_path, value, -1);
  EXPECT_EQ(-1, value);
  ASSERT_EQ(1u, ErrorHandler::resolve_errors.size());
  EXPECT_NE(std::string::npos,
            ErrorHandler::resolve_errors.front().find("no.such.path"));

  // 数组越界 → OnPathResolveError
  view.Put<int, Required>("list[9]"_path, value, -1);
  EXPECT_EQ(2u, ErrorHandler::resolve_errors.size());

  // 类型不符 → OnTypeMismatch
  view.Put<int, Required>("s"_path, value, -1);
  EXPECT_EQ(-1, value);
  ASSERT_EQ(1u, ErrorHandler::mismatches.size());
  EXPECT_EQ(std::string("s"), ErrorHandler::mismatches.front());

  // int 放不下的大整数同样按类型不符处理
  view.Put<int, Required>("big"_path, value, -1);
  EXPECT_EQ(2u, ErrorHandler::mismatches.size());
  EXPECT_EQ(std::string("big"), ErrorHandler::mismatches.back());

  // 推导形式（唯一模板实参是策略）同样按 Required 上报
  view.Put<int, Required>("no.such.again"_path, value, -1);
  EXPECT_EQ(-1, value);
  EXPECT_EQ(3u, ErrorHandler::resolve_errors.size());
  EXPECT_NE(std::string::npos, ErrorHandler::resolve_errors.back().find(
                                   "no.such.again"));

  view.Put<int, Required>("d"_path, value, -1);  // double 不是 int
  EXPECT_EQ(3u, ErrorHandler::mismatches.size());
  EXPECT_EQ(std::string("d"), ErrorHandler::mismatches.back());

  // 默认 handler 下抛异常
  JsonView strict(json);
  // 注意：模板实参里的逗号会切断宏参数，故整体再包一层括号
  EXPECT_THROW(
      ((void)strict.Put<int, Required>("no.such.path"_path, value)),
      std::runtime_error);
  EXPECT_THROW(((void)strict.Put<int, Required>("s"_path, value)),
               std::runtime_error);
}

TEST(SC_JSON_TEST_SUITE_NAME, PutFillsOutputs) {
  const SC_JSON_TYPE json = SC_JSON_MAKE_JSON();
  JsonView view(json);

  int i = 0;
  std::string s;
  bool flag = false;
  view.Put<int, Required>("a.b"_path, i);
  view.Put<std::string, Required>("s"_path, s);
  view.Put<bool, Required>("flag"_path, flag);
  EXPECT_EQ(42, i);
  EXPECT_EQ(std::string("hello"), s);
  EXPECT_TRUE(flag);

  // 推导形式与显式形式混用
  int deduced_i = 0;
  view.Put("a.b"_path, deduced_i);
  view.Put<std::string, Required>("s"_path, s);
  EXPECT_EQ(42, deduced_i);
}

TEST(SC_JSON_TEST_SUITE_NAME, PutArrayAndMap) {
  const SC_JSON_TYPE json = SC_JSON_MAKE_JSON();
  JsonView view(json);

  std::vector<int> nums;
  view.Put<std::vector<int>, Required>("list"_path, nums);
  EXPECT_EQ((std::vector{1, 2, 3}), nums);

  std::map<std::string, int> point;
  view.Put<std::map<std::string, int>, Required>("point"_path, point);
  ASSERT_EQ(2u, point.size());
  EXPECT_EQ(10, point["x"]);
  EXPECT_EQ(20, point["y"]);

  // Optional：缺失时得到空容器
  std::vector empty_nums = {9};
  view.Put("no.such"_path, empty_nums);
  EXPECT_TRUE(empty_nums.empty());

  std::map<std::string, int> empty_map;
  view.Put("no.such"_path, empty_map);
  EXPECT_TRUE(empty_map.empty());

  // 推导形式：元素类型由容器决定
  std::vector<int> deduced_nums;
  view.Put("list"_path, deduced_nums);
  EXPECT_EQ((std::vector{1, 2, 3}), deduced_nums);

  std::map<std::string, int> deduced_point;
  view.Put("point"_path, deduced_point);
  ASSERT_EQ(2u, deduced_point.size());
  EXPECT_EQ(10, deduced_point["x"]);
  EXPECT_EQ(20, deduced_point["y"]);

  // 推导形式下 ExistencePolicy 仍是第一个模板实参
  std::vector<int> required_nums;
  view.Put<std::vector<int>, Required>("list"_path, required_nums);
  EXPECT_EQ((std::vector{1, 2, 3}), required_nums);

  std::map<std::string, int> required_point;
  view.Put<std::map<std::string, int>, Required>("point"_path, required_point);
  EXPECT_EQ(2u, required_point.size());
}

TEST(SC_JSON_TEST_SUITE_NAME, StringPathOverloads) {
  const SC_JSON_TYPE json = SC_JSON_MAKE_JSON();
  JsonView view(json);

  int neg = 0;
  view.Put("neg", neg);
  EXPECT_EQ(-7, neg);
  EXPECT_EQ(-7, view.Get<int>("neg"));
  EXPECT_EQ(-7, view.Get<int>("neg", -1));
  EXPECT_EQ(-1, view.Get<int>("no.such", -1));

  std::string text;
  view.Put("s", text);
  EXPECT_EQ(std::string("hello"), text);

  JsonOwner<SC_JSON_TYPE> owner(std::string_view(R"({})"));
  JsonWriter<SC_JSON_TYPE> writer(owner);
  writer["n"] = 5;
  writer.Set("m", 7);
  writer["status"] = "ok";
  JsonView written(owner);
  EXPECT_EQ(5, written.Get<int>("n"));
  EXPECT_EQ(7, written.Get<int>("m"));
  EXPECT_EQ(std::string("ok"), written.Get<std::string>("status"));
}

TEST(SC_JSON_TEST_SUITE_NAME, WriterSetOwnerAsValue) {
  JsonOwner<SC_JSON_TYPE> base(std::string_view(R"({"a":{}})"));
  JsonWriter<SC_JSON_TYPE> writer(base);

  JsonOwner<SC_JSON_TYPE> child(std::string_view(R"({"v":2})"));
  writer.Set("child", std::move(child));

  JsonOwner<SC_JSON_TYPE> nested(std::string_view(R"({"w":3})"));
  writer.Set<Required>("a.nested"_path, std::move(nested));

  JsonOwner<SC_JSON_TYPE> via_subscript(std::string_view(R"({"z":4})"));
  writer["sub"_path] = std::move(via_subscript);
  EXPECT_FALSE(!via_subscript.valid());

  JsonOwner<SC_JSON_TYPE> copy_source(std::string_view(R"({"v":5})"));
  writer.Set("copied", copy_source);

  JsonView view(base);
  EXPECT_EQ(2, view.Get<int>("child.v"_path));
  EXPECT_EQ(3, view.Get<int>("a.nested.w"_path));
  EXPECT_EQ(4, view.Get<int>("sub.z"_path));
  EXPECT_EQ(5, view.Get<int>("copied.v"_path));
  EXPECT_EQ(5, JsonView(copy_source).Get<int>("v"));

  JsonOwner<SC_JSON_TYPE> replacement(std::string_view(R"({"r":9})"));
  writer.Set<Required>(""_path, std::move(replacement));
  JsonView replaced(base);
  EXPECT_EQ(9, replaced.Get<int>("r"_path));
  EXPECT_FALSE(replaced.View("child").valid());

  struct Payload {
    int v = 6;
    std::string s = "x";
  };
  JsonOwner<SC_JSON_TYPE> self_target(std::string_view(R"({"old":1})"));
  JsonWriter<SC_JSON_TYPE> self_writer(self_target);
  self_writer.Set<Required>(""_path, Payload{});
  JsonView payload_view(self_target);
  EXPECT_EQ(6, payload_view.Get<int>("v"));
  EXPECT_EQ(std::string("x"), payload_view.Get<std::string>("s"));
  EXPECT_FALSE(payload_view.View("old").valid());

  std::map<std::string, int> map_value{{"k", 7}};
  self_writer.Set<Required>(""_path, map_value);
  JsonView map_view(self_target);
  EXPECT_EQ(7, map_view.Get<int>("k"));
  EXPECT_FALSE(map_view.View("v").valid());
}

TEST(SC_JSON_TEST_SUITE_NAME, ViewStringOverloads) {
  const SC_JSON_TYPE json = SC_JSON_MAKE_JSON();
  JsonView view(json);

  EXPECT_EQ(42, view.View("a").View("b").GetInt());
  EXPECT_EQ(std::string("hello"), view.View("s").GetString());
  EXPECT_EQ(3u, view.ViewArray("list").size());
  EXPECT_EQ(2, (view.ViewArray("list").begin() + 1)->GetInt());
  EXPECT_EQ(10, view.ViewObject("point").begin()->second.GetInt());
  EXPECT_EQ(42, view.View<Required>("a").View<Required>("b").GetInt());
}

TEST(SC_JSON_TEST_SUITE_NAME, WriterCreatesNestedPathBySubscript) {
  JsonOwner<SC_JSON_TYPE> owner;
  JsonWriter<SC_JSON_TYPE> writer(owner);
  writer["a.b"_path] = 1;
  writer["a.c"_path] = 1.2;

  std::ostringstream os;
  owner.ToStream(os);
  EXPECT_NE(std::string("{}"), os.str());

  JsonView view(owner);
  EXPECT_EQ(1, view.Get<int>("a.b"_path));
  EXPECT_DOUBLE_EQ(1.2, view.Get<double>("a.c"_path));
}

TEST(SC_JSON_TEST_SUITE_NAME, GetStructWithDoubleVector) {
  struct EvaluateParams {
    std::vector<double> value;
    int task = 0;
  };

  JsonOwner<SC_JSON_TYPE> owner(std::string_view(
      R"({"value":[773.14999999999998,1,1,0,1500000,85000,0,0,0]})"));
  JsonView view(owner);

  const auto params = view.Get<EvaluateParams>(""_path);
  ASSERT_EQ(9u, params.value.size());
  EXPECT_DOUBLE_EQ(773.15, params.value[0]);
  EXPECT_DOUBLE_EQ(1.0, params.value[1]);
  EXPECT_DOUBLE_EQ(1500000.0, params.value[4]);
  EXPECT_DOUBLE_EQ(85000.0, params.value[5]);
  EXPECT_EQ(0, params.task);

  EvaluateParams target;
  view.Put(""_path, target);
  ASSERT_EQ(9u, target.value.size());
  EXPECT_DOUBLE_EQ(773.15, target.value[0]);
}

TEST(SC_JSON_TEST_SUITE_NAME, PutArrayElementMismatchReportsFullPath) {
  using ErrorHandler = SC_JSON_CONCAT(ErrorHandler_, SC_JSON_TEST_SUITE_NAME);
  ErrorHandler handler;
  ErrorHandler::mismatches.clear();

  const SC_JSON_TYPE json = SC_JSON_MAKE_JSON();
  JsonView<SC_JSON_TYPE, ErrorHandler> view(json, handler);

  std::vector<int> arr;
  // 元素是对象，不是 int
  view.Put<std::vector<int>, Required>("obj_list"_path, arr);
  ASSERT_EQ(1u, ErrorHandler::mismatches.size());
  EXPECT_EQ(std::string("obj_list[0]"), ErrorHandler::mismatches.front());
  EXPECT_TRUE(arr.empty());
}

TEST(SC_JSON_TEST_SUITE_NAME, PutMapValueMismatchReportsFullPath) {
  using ErrorHandler = SC_JSON_CONCAT(ErrorHandler_, SC_JSON_TEST_SUITE_NAME);
  ErrorHandler handler;
  ErrorHandler::mismatches.clear();

  const SC_JSON_TYPE json = SC_JSON_MAKE_JSON();
  JsonView<SC_JSON_TYPE, ErrorHandler> view(json, handler);

  std::map<std::string, std::string> map;
  // 值是 int，不是 string
  view.Put<std::map<std::string, std::string>, Required>("point"_path, map);
  ASSERT_EQ(1u, ErrorHandler::mismatches.size());
  EXPECT_EQ(std::string("point."), ErrorHandler::mismatches.front().substr(0, 6));
  EXPECT_TRUE(map.empty());
}

TEST(SC_JSON_TEST_SUITE_NAME, PutCustomType) {
  using Point = SC_JSON_CONCAT(InnerPoint_, SC_JSON_TEST_SUITE_NAME);
  const SC_JSON_TYPE json = SC_JSON_MAKE_JSON();
  JsonView view(json);

  // 显式写类型：Put<类型, 策略>(path, out)
  Point point;
  view.Put<Point, Required>("point"_path, point);
  EXPECT_EQ(10, point.x);
  EXPECT_EQ(20, point.y);

  // 自定义类型不匹配时同样走类型不符分支
  Point fallback;
  view.Put("list"_path, fallback, Point{1, 2});
  EXPECT_EQ(1, fallback.x);
  EXPECT_EQ(2, fallback.y);

  // 推导形式命中 Extractor<Json, Point> 特化
  Point deduced;
  view.Put("point"_path, deduced);
  EXPECT_EQ(10, deduced.x);
  EXPECT_EQ(20, deduced.y);

  Point deduced_fallback;
  view.Put("list"_path, deduced_fallback, Point{1, 2});
  EXPECT_EQ(1, deduced_fallback.x);
  EXPECT_EQ(2, deduced_fallback.y);
}

TEST(SC_JSON_TEST_SUITE_NAME, PutWithRuntimePath) {
  const SC_JSON_TYPE json = SC_JSON_MAKE_JSON();
  JsonView view(json);

  const std::string key = "a";
  const auto path = Parts{key, "b"};
  int value = 0;
  view.Put<int, Required>(path, value);
  EXPECT_EQ(42, value);

  int deduced = 0;
  view.Put<int, Required>(path, deduced);
  EXPECT_EQ(42, deduced);
}

TEST(SC_JSON_TEST_SUITE_NAME, PutNullIsTypeMismatchNotMissing) {
  using ErrorHandler = SC_JSON_CONCAT(ErrorHandler_, SC_JSON_TEST_SUITE_NAME);
  ErrorHandler handler;
  ErrorHandler::resolve_errors.clear();
  ErrorHandler::mismatches.clear();

  const SC_JSON_TYPE json = SC_JSON_MAKE_JSON();
  JsonView<SC_JSON_TYPE, ErrorHandler> view(json, handler);

  // Optional：null 不是 int → 写默认值，且不报错
  int optional_value = 5;
  view.Put("nil"_path, optional_value, -1);
  EXPECT_EQ(-1, optional_value);
  EXPECT_TRUE(ErrorHandler::resolve_errors.empty());
  EXPECT_TRUE(ErrorHandler::mismatches.empty());

  // null 的节点是存在的，所以 Required 下走 OnTypeMismatch 而不是 OnPathResolveError
  int required_value = 5;
  view.Put<int, Required>("nil"_path, required_value, -1);
  EXPECT_EQ(-1, required_value);
  ASSERT_EQ(1u, ErrorHandler::mismatches.size());
  EXPECT_EQ(std::string("nil"), ErrorHandler::mismatches.front());
  EXPECT_TRUE(ErrorHandler::resolve_errors.empty());

  // 其它类型同理：null 拿不到 string，Optional 得到默认空串
  std::string text = "x";
  view.Put("nil"_path, text, std::string{});
  EXPECT_TRUE(text.empty());

  // 继续在 null 下面走路径属于「解析失败」
  EXPECT_FALSE(view.View<Required>("nil.child"_path).valid());
  ASSERT_EQ(1u, ErrorHandler::resolve_errors.size());
  EXPECT_TRUE(ErrorHandler::resolve_errors.front().find("nil.child") !=
              std::string::npos);
  EXPECT_EQ(1u, ErrorHandler::mismatches.size());  // 没有多记一次类型不符
}

// ---------------- std::optional 目标（JSON null ↔ std::nullopt）----------------

TEST(SC_JSON_TEST_SUITE_NAME, PutOptionalTargetTurnsNullIntoNullopt) {
  using ErrorHandler = SC_JSON_CONCAT(ErrorHandler_, SC_JSON_TEST_SUITE_NAME);
  ErrorHandler handler;
  ErrorHandler::resolve_errors.clear();
  ErrorHandler::mismatches.clear();

  const SC_JSON_TYPE json = SC_JSON_MAKE_JSON();
  JsonView<SC_JSON_TYPE, ErrorHandler> view(json, handler);

  // 命中 → engaged
  std::optional<int> value;
  view.Put("a.b"_path, value, std::nullopt);
  ASSERT_TRUE(value.has_value());
  EXPECT_EQ(42, *value);

  // JSON null → nullopt；且 Required 下也算成功（null 是 optional 的合法取值）
  view.Put<std::optional<int>, Required>("nil"_path, value, std::nullopt);
  EXPECT_FALSE(value.has_value());

  // nullopt 是「提取成功但无值」，不是错误
  EXPECT_TRUE(ErrorHandler::resolve_errors.empty());
  EXPECT_TRUE(ErrorHandler::mismatches.empty());

  // 缺失 → nullopt（Optional 下写默认值）
  std::optional missing{7};
  view.Put("no.such"_path, missing, std::nullopt);
  EXPECT_FALSE(missing.has_value());

  // 类型不符 → Optional 下同样是 nullopt
  std::optional mismatch{9};
  view.Put("s"_path, mismatch, std::nullopt);
  EXPECT_FALSE(mismatch.has_value());
  EXPECT_TRUE(ErrorHandler::resolve_errors.empty());
  EXPECT_TRUE(ErrorHandler::mismatches.empty());

  // 显式提取器形式同样支持可空目标
  std::optional explicit_form{3};
  view.Put<std::optional<int>, Required>("nil"_path, explicit_form,
                                         std::nullopt);
  EXPECT_FALSE(explicit_form.has_value());
  EXPECT_TRUE(ErrorHandler::mismatches.empty());

  // 其它类型同样可空
  std::optional<std::string> text{"x"};
  view.Put("s"_path, text, std::nullopt);
  ASSERT_TRUE(text.has_value());
  EXPECT_EQ(std::string("hello"), *text);
  view.Put<std::optional<std::string>, Required>("nil"_path, text,
                                                 std::nullopt);
  EXPECT_FALSE(text.has_value());

  std::optional<bool> flag;
  view.Put("flag"_path, flag, std::nullopt);
  ASSERT_TRUE(flag.has_value());
  EXPECT_TRUE(*flag);
  view.Put("nil"_path, flag, std::nullopt);
  EXPECT_FALSE(flag.has_value());

  // Required + 类型不符仍然报 OnTypeMismatch（只有 null 被 optional 接纳）
  std::optional bad{1};
  view.Put<std::optional<int>, Required>("s"_path, bad, std::nullopt);
  EXPECT_FALSE(bad.has_value());
  ASSERT_EQ(1u, ErrorHandler::mismatches.size());
  EXPECT_EQ(std::string("s"), ErrorHandler::mismatches.front());

  // nullopt 只吸收「null」，不能吸收「缺失」：Required 下路径缺失仍是路径错误
  std::optional required_missing{5};
  view.Put<std::optional<int>, Required>("no.such.path"_path, required_missing,
                                        std::nullopt);
  EXPECT_FALSE(required_missing.has_value());
  ASSERT_EQ(1u, ErrorHandler::resolve_errors.size());
  EXPECT_NE(std::string::npos, ErrorHandler::resolve_errors.front().find(
                                   "no.such.path"));
  EXPECT_EQ(1u, ErrorHandler::mismatches.size());
}

TEST(SC_JSON_TEST_SUITE_NAME, PutArrayOfOptionalTargets) {
  const SC_JSON_TYPE json = SC_JSON_MAKE_JSON();
  JsonView view(json);

  // [1,null,3] → [1, nullopt, 3]
  std::vector<std::optional<int>> values;
  view.Put<std::vector<std::optional<int>>, Required>("opt_list"_path,
                                                      values);
  ASSERT_EQ(3u, values.size());
  ASSERT_TRUE(values[0].has_value());
  EXPECT_EQ(1, *values[0]);
  EXPECT_FALSE(values[1].has_value());
  ASSERT_TRUE(values[2].has_value());
  EXPECT_EQ(3, *values[2]);

  // 全是具体值时为全 engaged
  std::vector<std::optional<int>> plain;
  view.Put("list"_path, plain);
  ASSERT_EQ(3u, plain.size());
  EXPECT_TRUE(plain[0].has_value());
  EXPECT_TRUE(plain[1].has_value());
  EXPECT_TRUE(plain[2].has_value());
}

TEST(SC_JSON_TEST_SUITE_NAME, PutArrayOptionalElementMismatchReportsPath) {
  using ErrorHandler = SC_JSON_CONCAT(ErrorHandler_, SC_JSON_TEST_SUITE_NAME);
  ErrorHandler handler;
  ErrorHandler::mismatches.clear();

  const SC_JSON_TYPE json = SC_JSON_MAKE_JSON();
  JsonView<SC_JSON_TYPE, ErrorHandler> view(json, handler);

  // 元素是对象，既不是 null 也不是 int → 类型不符
  std::vector<std::optional<int>> arr;
  view.Put<std::vector<std::optional<int>>, Required>("obj_list"_path, arr);
  ASSERT_EQ(1u, ErrorHandler::mismatches.size());
  EXPECT_EQ(std::string("obj_list[0]"), ErrorHandler::mismatches.front());
  EXPECT_TRUE(arr.empty());
}

TEST(SC_JSON_TEST_SUITE_NAME, PutMapOfOptionalTargets) {
  const SC_JSON_TYPE json = SC_JSON_MAKE_JSON();
  JsonView view(json);

  // {"x":10,"y":null} → x engaged 10，y nullopt
  std::map<std::string, std::optional<int>> values;
  view.Put<std::map<std::string, std::optional<int>>, Required>(
      "opt_point"_path, values);
  ASSERT_EQ(2u, values.size());
  ASSERT_TRUE(values["x"].has_value());
  EXPECT_EQ(10, *values["x"]);
  EXPECT_FALSE(values["y"].has_value());
}

// ---------------- Get：直接返回数据 ----------------

TEST(SC_JSON_TEST_SUITE_NAME, GetScalarsReturnsValue) {
  const SC_JSON_TYPE json = SC_JSON_MAKE_JSON();
  JsonView view(json);

  // 显式写 <类型, 策略>；注意 gtest 宏不识别尖括号里的逗号，
  // 所以要么先赋值给局部变量，要么把整个调用再包一层括号
  const int i = view.Get<int, Required>("a.b"_path);
  EXPECT_EQ(42, i);
  const auto big = view.Get<std::int64_t, Required>("big"_path);
  EXPECT_EQ(9007199254740993LL, big);
  const auto s = view.Get<std::string, Required>("s"_path);
  EXPECT_EQ(std::string("hello"), s);
  EXPECT_DOUBLE_EQ(3.14, (view.Get<double, Required>("d"_path)));
  EXPECT_TRUE((view.Get<bool, Required>("flag"_path)));

  // 默认值决定 T（可省模板实参；策略仍是第一个实参）
  EXPECT_EQ(42, view.Get("a.b"_path, 0));
  EXPECT_EQ(-1, view.Get("no.such.path"_path, -1));

  // Optional：缺失/类型不符 → T{}；给了默认值就用默认值；命中则给实际值
  EXPECT_EQ(0, (view.Get<int, Optional>("no.such.path"_path)));
  EXPECT_EQ(-1, (view.Get<int, Optional>("s"_path, -1)));
  EXPECT_EQ(-1, (view.Get<int, Optional>("no.such.path"_path, -1)));
  EXPECT_EQ(42, (view.Get<int, Optional>("a.b"_path, -1)));

  // JSON null 不是 int → Optional 回落默认值；nullopt 语义用 optional 目标表达
  EXPECT_EQ(-1, (view.Get<int, Optional>("nil"_path, -1)));
  const auto nil =
      view.Get<std::optional<int>, Required>("nil"_path);
  EXPECT_FALSE(nil.has_value());
  const auto hit =
      view.Get<std::optional<int>, Required>("a.b"_path);
  EXPECT_EQ(42, hit.value_or(0));
}

TEST(SC_JSON_TEST_SUITE_NAME, GetRequiredReportsErrors) {
  using ErrorHandler = SC_JSON_CONCAT(ErrorHandler_, SC_JSON_TEST_SUITE_NAME);
  ErrorHandler handler;
  ErrorHandler::resolve_errors.clear();
  ErrorHandler::mismatches.clear();

  const SC_JSON_TYPE json = SC_JSON_MAKE_JSON();
  JsonView<SC_JSON_TYPE, ErrorHandler> view(json, handler);

  // 路径缺失 → OnPathResolveError，返回值回落默认值
  EXPECT_EQ(-1, (view.Get<int, Required>("no.such.path"_path, -1)));
  ASSERT_EQ(1u, ErrorHandler::resolve_errors.size());

  // 类型不符 → OnTypeMismatch
  EXPECT_EQ(-1, (view.Get<int, Required>("s"_path, -1)));
  ASSERT_EQ(1u, ErrorHandler::mismatches.size());
  EXPECT_EQ(std::string("s"), ErrorHandler::mismatches.front());

  // 默认 handler 下抛异常（模板实参里的逗号要再包一层括号）
  JsonView strict(json);
  EXPECT_THROW(((void)strict.Get<int, Required>("no.such.path"_path)),
               std::runtime_error);
  EXPECT_THROW(((void)strict.Get<int, Required>("s"_path)), std::runtime_error);
}

TEST(SC_JSON_TEST_SUITE_NAME, GetArrayAndGetMapReturnContainers) {
  const SC_JSON_TYPE json = SC_JSON_MAKE_JSON();
  JsonView view(json);

  // 第二个模板实参是「返回类型」
  const std::vector<int> nums =
      view.Get<std::vector<int>, Required>("list"_path);
  EXPECT_EQ((std::vector{1, 2, 3}), nums);

  const auto point =
      view.Get<std::map<std::string, int>, Required>("point"_path);
  ASSERT_EQ(2u, point.size());
  EXPECT_EQ(10, point.at("x"));
  EXPECT_EQ(20, point.at("y"));

  // Optional：失败 → 空容器
  const std::vector<int> missing =
      view.Get<std::vector<int>, Optional>("no.such"_path);
  EXPECT_TRUE(missing.empty());
  const auto not_object =
      view.Get<std::map<std::string, int>, Optional>("list"_path);
  EXPECT_TRUE(not_object.empty());

  // 元素可空：null → nullopt
  const auto opts =
      view.Get<std::vector<std::optional<int>>, Required>("opt_list"_path);
  ASSERT_EQ(3u, opts.size());
  ASSERT_TRUE(opts[0].has_value());
  EXPECT_EQ(1, *opts[0]);
  EXPECT_FALSE(opts[1].has_value());
  ASSERT_TRUE(opts[2].has_value());
  EXPECT_EQ(3, *opts[2]);
}

TEST(SC_JSON_TEST_SUITE_NAME, GetOnArrowAndRuntimePath) {
  const SC_JSON_TYPE json = SC_JSON_MAKE_JSON();
  JsonView view(json);

  // 箭头链式：元素是对象时直接取值
  auto objects = view.ViewArray<Required>("obj_list"_path);
  const auto first = objects.begin()->Get<std::int64_t, Required>("id"_path);
  EXPECT_EQ(1, first);
  const std::int64_t second =
      (objects.begin() + 1)->Get<std::int64_t, Required>("id"_path);
  EXPECT_EQ(2, second);
  EXPECT_EQ(-1, (objects.begin()->Get<std::int64_t, Optional>(
                    "no.such"_path, std::int64_t{-1})));
  EXPECT_EQ(7, objects.begin()->Get("no.such"_path, 7));  // T 由默认值推导

  // 运行期路径同样可用
  const auto path = Parts{"a", "b"};
  const int from_runtime = view.Get<int, Required>(path);
  EXPECT_EQ(42, from_runtime);
}

TEST(SC_JSON_TEST_SUITE_NAME, TypeIsFirstPolicyIsSecond) {
  const SC_JSON_TYPE json = SC_JSON_MAKE_JSON();
  JsonView view(json);

  // 策略在第二个参数且有默认值 → 只写类型即可
  EXPECT_TRUE(view.Get<bool>("flag"_path));
  EXPECT_EQ(42, view.Get<int>("a.b"_path));
  EXPECT_EQ(std::string("hello"), view.Get<std::string>("s"_path));
  EXPECT_DOUBLE_EQ(3.14, view.Get<double>("d"_path));
  EXPECT_EQ(9007199254740993LL, view.Get<std::int64_t>("big"_path));

  // Optional（默认）：null / 缺失 → T{} 或给定默认值
  EXPECT_FALSE(view.Get<bool>("nil"_path));
  EXPECT_EQ(-1, view.Get<int>("no.such.path"_path, -1));
  const int mismatch = view.Get<int, Optional>("s"_path);
  EXPECT_EQ(0, mismatch);

  // 第二个参数给策略
  EXPECT_THROW(((void)view.Get<int, Required>("s"_path)), std::runtime_error);

  // 容器版同理：只写容器类型
  const std::vector<int> nums = view.Get<std::vector<int>>("list"_path);
  EXPECT_EQ((std::vector{1, 2, 3}), nums);
  const std::map<std::string, int> point =
      view.Get<std::map<std::string, int>>("point"_path);
  EXPECT_EQ(2u, point.size());

  // Put：类型在第一个，策略在第二个；全推导写法不受影响
  int value = 0;
  view.Put<int, Required>("a.b"_path, value);
  EXPECT_EQ(42, value);
  view.Put("a.b"_path, value);
  EXPECT_EQ(42, value);
}

TEST(SC_JSON_TEST_SUITE_NAME, GetArrayOfDoubleFromNestedPath) {
  const SC_JSON_TYPE json = SC_JSON_MAKE_JSON();
  JsonView view(json);

  // 外部项目里的写法：auto values = view.Get<std::vector<double>, Required>(path);
  const std::vector<double> values =
      view.Get<std::vector<double>, Required>("mix_res[0].value"_path);
  ASSERT_EQ(2u, values.size());
  EXPECT_DOUBLE_EQ(1.5, values[0]);
  EXPECT_DOUBLE_EQ(2.5, values[1]);

  // 省略策略
  const auto values2 =
      view.Get<std::vector<double>>("mix_res[0].value"_path);
  EXPECT_EQ(values, values2);

  // 写入形式：显式 <类型, 策略> 与全推导
  std::vector<double> out;
  view.Put<std::vector<double>, Required>("mix_res[0].value"_path, out);
  EXPECT_EQ(values, out);
  std::vector<double> out2;
  view.Put("mix_res[0].value"_path, out2);
  EXPECT_EQ(values, out2);

  // 缺失 → 空容器（策略默认 Optional）
  const std::vector<double> missing =
      view.Get<std::vector<double>>("no.such"_path);
  EXPECT_TRUE(missing.empty());
}

TEST(SC_JSON_TEST_SUITE_NAME, TraitsOpsOnView) {
  const SC_JSON_TYPE json = SC_JSON_MAKE_JSON();
  JsonView view(json);

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
  EXPECT_EQ(2, TraitGet<int>(list.GetArrayElement(1)));
}

TEST(SC_JSON_TEST_SUITE_NAME, TraitsOpsOnInvalidViewAreSafe) {
  const SC_JSON_TYPE json = SC_JSON_MAKE_JSON();
  JsonView view(json);
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
  EXPECT_FALSE(invalid.IsNull());
  EXPECT_TRUE(invalid.ToString().empty());
  EXPECT_EQ(invalid.MemberBegin(), invalid.MemberEnd());
  EXPECT_FALSE(JsonTraits<SC_JSON_TYPE>::IsObject(invalid.GetArrayElement(0)));
}

TEST(SC_JSON_TEST_SUITE_NAME, TraitsIsNull) {
  const SC_JSON_TYPE json = SC_JSON_MAKE_JSON();
  JsonView view(json);

  const auto nil = view.View<Required>("nil"_path);
  ASSERT_TRUE(nil.valid());
  EXPECT_TRUE(nil.IsNull());
  EXPECT_FALSE(nil.IsObject());
  EXPECT_FALSE(nil.IsArray());
  EXPECT_FALSE(nil.IsInt());
  EXPECT_FALSE(nil.IsUint());
  EXPECT_FALSE(nil.IsInt64());
  EXPECT_FALSE(nil.IsUint64());
  EXPECT_FALSE(nil.IsDouble());
  EXPECT_FALSE(nil.IsString());
  EXPECT_FALSE(nil.IsBool());

  // 其它类型不是 null
  EXPECT_FALSE(view.IsNull());  // 根是对象
  EXPECT_FALSE(view.View<Required>("s"_path).IsNull());
  EXPECT_FALSE(view.View<Required>("list"_path).IsNull());
  EXPECT_FALSE(view.View<Required>("flag"_path).IsNull());
  EXPECT_FALSE(view.View<Required>("a"_path).IsNull());

  // 直接走 JsonTraits 同样成立，且无效 view 上返回 false（不崩）
  const SC_JSON_TYPE* node = view.TryGetChild("nil");
  ASSERT_NE(nullptr, node);
  EXPECT_TRUE(JsonTraits<SC_JSON_TYPE>::IsNull(*node));
  EXPECT_FALSE(view.View("no.such"_path).IsNull());
}

TEST(SC_JSON_TEST_SUITE_NAME, TraitsToString) {
  const SC_JSON_TYPE json = SC_JSON_MAKE_JSON();
  JsonView view(json);

  // 标量与容器都按紧凑形式序列化
  EXPECT_EQ(std::string("42"), view.View<Required>("a.b"_path).ToString());
  EXPECT_EQ(std::string("-7"), view.View<Required>("neg"_path).ToString());
  EXPECT_EQ(std::string("9007199254740993"),
            view.View<Required>("big"_path).ToString());
  EXPECT_EQ(std::string("\"hello\""), view.View<Required>("s"_path).ToString());
  EXPECT_EQ(std::string("true"), view.View<Required>("flag"_path).ToString());
  EXPECT_EQ(std::string("null"), view.View<Required>("nil"_path).ToString());
  EXPECT_EQ(std::string("[1,2,3]"), view.View<Required>("list"_path).ToString());
  EXPECT_EQ(std::string("{\"x\":10,\"y\":20}"),
            view.View<Required>("point"_path).ToString());

  // 内存里的 null 节点直接序列化
  const SC_JSON_TYPE* node = view.TryGetChild("nil");
  ASSERT_NE(nullptr, node);
  EXPECT_EQ(std::string("null"), JsonTraits<SC_JSON_TYPE>::ToString(*node));

  // 整篇文档：序列化结果可以重新解析，且再次序列化稳定（键序、数字精度都稳定）
  const std::string text = view.ToString();
  EXPECT_FALSE(text.empty());
  JsonOwner<SC_JSON_TYPE> reparsed_owner{std::string_view(text)};
  JsonView reparsed(reparsed_owner);
  EXPECT_EQ(42, reparsed.View<Required>("a.b"_path).GetInt());
  EXPECT_EQ(std::string("hello"), reparsed.View<Required>("s"_path).GetString());
  EXPECT_TRUE(reparsed.View<Required>("nil"_path).IsNull());
  EXPECT_EQ(text, reparsed.ToString());
}

TEST(SC_JSON_TEST_SUITE_NAME, TraitsToStringKeepsUtf8) {
  // 期望值用显式 UTF-8 字节写出，避免源码编码问题把乱码掩盖过去
  const std::string kZh = "\xE4\xB8\xAD\xE6\x96\x87";       // 中文
  const std::string kQuotedZh = "\"" + kZh + "\"";          // "中文"
  const std::string kKey = "\xE9\x94\xAE";                  // 键
  const std::string kValue = "\xE5\x80\xBC";                // 值
  const std::string kZhKeyObj = "{\"" + kKey + "\":\"" + kValue + "\"}";

  const SC_JSON_TYPE json = SC_JSON_MAKE_JSON();
  JsonView view(json);

  // 1) 解析：\uXXXX 转义与源码里的字面 UTF-8 都必须还原成同一串 UTF-8 字节
  EXPECT_EQ(kZh, view.View<Required>("zh"_path).GetString());
  EXPECT_EQ(kZh, view.View<Required>("zh_raw"_path).GetString());

  // 2) 序列化：直接输出 UTF-8 字节，既不乱码也不转义成 \uXXXX
  EXPECT_EQ(kQuotedZh, view.View<Required>("zh"_path).ToString());
  EXPECT_EQ(kQuotedZh, view.View<Required>("zh_raw"_path).ToString());
  EXPECT_EQ(kZhKeyObj, view.View<Required>("zh_key"_path).ToString());

  // 3) 整篇文档：中文键值原样出现，且没有 \uXXXX 转义
  const std::string text = view.ToString();
  EXPECT_NE(std::string::npos, text.find(kQuotedZh));
  EXPECT_NE(std::string::npos, text.find(kZhKeyObj));
  EXPECT_EQ(std::string::npos, text.find("\\u4e2d"));
  EXPECT_EQ(std::string::npos, text.find("\\u952e"));

  // 4) 序列化 → 再解析 → 再序列化，中文与键都不丢字节
  JsonOwner<SC_JSON_TYPE> reparsed_owner{std::string_view(text)};
  JsonView reparsed(reparsed_owner);
  EXPECT_EQ(kZh, reparsed.View<Required>("zh"_path).GetString());
  EXPECT_EQ(kZh, reparsed.View<Required>("zh_raw"_path).GetString());
  EXPECT_EQ(kValue,
            reparsed.View<Required>("zh_key.\xE9\x94\xAE"_path).GetString());
  EXPECT_EQ(text, reparsed.ToString());
}

// ---------------- 运行期路径 ----------------

TEST(SC_JSON_TEST_SUITE_NAME, RuntimePathResolvesDynamicPath) {
  const SC_JSON_TYPE json = SC_JSON_MAKE_JSON();
  JsonView view(json);

  const std::string prefix = "a";
  const auto path = Parts{prefix, "b"};
  EXPECT_EQ(42, view.View<Required>(path).GetInt());

  const auto indexed = Parts{"obj_list", "[1]", "id"};
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
  const auto empty_segment = Parts{"a", ""};
  EXPECT_FALSE(view.View<Required>(empty_segment).valid());
  ASSERT_EQ(1u, ErrorHandler::resolve_errors.size());
  EXPECT_NE(std::string::npos,
            ErrorHandler::resolve_errors.front().find("a."));

  const auto unclosed = Parts{"a", "[1"};
  EXPECT_FALSE(view.View<Required>(unclosed).valid());
  EXPECT_EQ(2u, ErrorHandler::resolve_errors.size());

  // 默认 handler 下同样报错（抛异常）
  JsonView strict(json);
  EXPECT_THROW((void)strict.View<Required>(unclosed), std::runtime_error);
}

// ---------------- 错误处理器 ----------------

TEST(SC_JSON_TEST_SUITE_NAME, StatefulErrorHandlerCarriesExternalData) {
  using Exception = SC_JSON_CONCAT(ContextException_, SC_JSON_TEST_SUITE_NAME);
  using Handler = SC_JSON_CONCAT(ContextErrorHandler_, SC_JSON_TEST_SUITE_NAME);

  const SC_JSON_TYPE json = SC_JSON_MAKE_JSON();
  Handler handler("req-001");
  // handler 由 view 持有指针，外部数据随引用一起带进异常
  JsonView<SC_JSON_TYPE, Handler> view(json, handler);

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

  // 子视图共享同一个 handler 指针
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

TEST(SC_JSON_TEST_SUITE_NAME, AutoChainOnWrite) {
  using ErrorHandler = SC_JSON_CONCAT(ErrorHandler_, SC_JSON_TEST_SUITE_NAME);
  ErrorHandler handler;
  JsonOwner<SC_JSON_TYPE, ErrorHandler> owner(
      std::string_view(R"({"a":{"x":1}})"), handler);
  const SC_JSON_TYPE& root = owner.json();

  auto resolve = [&]<typename T0>(const T0& path) {
    return detail::ResolveWithPolicy<SC_JSON_TYPE, std::decay_t<T0>::kSize, Required,
                                     ErrorHandler, true>(
        &root, path, handler, &owner);
  };

  // 深度 0：空路径即根节点
  EXPECT_EQ(&root, resolve(""_path));

  // 深度 1：缺失的一段被建出来
  ASSERT_NE(nullptr, resolve("p"_path));
  EXPECT_NE(nullptr, JsonTraits<SC_JSON_TYPE>::TryGetChild(root, "p"));

  // 深度 2：两段都补齐
  ASSERT_NE(nullptr, resolve("q.r"_path));
  const SC_JSON_TYPE* q = JsonTraits<SC_JSON_TYPE>::TryGetChild(root, "q");
  ASSERT_NE(nullptr, q);
  EXPECT_NE(nullptr, JsonTraits<SC_JSON_TYPE>::TryGetChild(*q, "r"));

  // 已存在：直接返回原节点
  const SC_JSON_TYPE* existing = resolve("a.x"_path);
  ASSERT_NE(nullptr, existing);
  EXPECT_EQ(1, TraitGet<int>(*existing));
}

TEST(SC_JSON_TEST_SUITE_NAME, WriteAddInt) {
  JsonOwner<SC_JSON_TYPE> owner(std::string_view(R"({})"));
  SC_JSON_TYPE* node =
      JsonTraits<SC_JSON_TYPE>::AddInt(owner, owner.json(), "v", 42);
  ASSERT_NE(nullptr, node);
  EXPECT_EQ(42, TraitGet<int>(*node));
  EXPECT_EQ(node, JsonTraits<SC_JSON_TYPE>::TryGetChild(owner.json(), "v"));
}

TEST(SC_JSON_TEST_SUITE_NAME, WriteAddUint) {
  JsonOwner<SC_JSON_TYPE> owner(std::string_view(R"({})"));
  SC_JSON_TYPE* node =
      JsonTraits<SC_JSON_TYPE>::AddUint(owner, owner.json(), "v", 7u);
  ASSERT_NE(nullptr, node);
  EXPECT_EQ(7u, TraitGet<unsigned int>(*node));
}

TEST(SC_JSON_TEST_SUITE_NAME, WriteAddInt64) {
  JsonOwner<SC_JSON_TYPE> owner(std::string_view(R"({})"));
  SC_JSON_TYPE* node = JsonTraits<SC_JSON_TYPE>::AddInt64(
      owner, owner.json(), "v", -9007199254740993LL);
  ASSERT_NE(nullptr, node);
  EXPECT_EQ(-9007199254740993LL, TraitGet<std::int64_t>(*node));
}

TEST(SC_JSON_TEST_SUITE_NAME, WriteAddUint64) {
  JsonOwner<SC_JSON_TYPE> owner(std::string_view(R"({})"));
  SC_JSON_TYPE* node = JsonTraits<SC_JSON_TYPE>::AddUint64(
      owner, owner.json(), "v", 18446744073709551615ULL);
  ASSERT_NE(nullptr, node);
  EXPECT_EQ(18446744073709551615ULL,
            TraitGet<std::uint64_t>(*node));
}

TEST(SC_JSON_TEST_SUITE_NAME, WriteAddDouble) {
  JsonOwner<SC_JSON_TYPE> owner(std::string_view(R"({})"));
  SC_JSON_TYPE* node =
      JsonTraits<SC_JSON_TYPE>::AddDouble(owner, owner.json(), "v", 3.5);
  ASSERT_NE(nullptr, node);
  EXPECT_DOUBLE_EQ(3.5, TraitGet<double>(*node));
}

TEST(SC_JSON_TEST_SUITE_NAME, WriteAddBool) {
  JsonOwner<SC_JSON_TYPE> owner(std::string_view(R"({})"));
  SC_JSON_TYPE* node =
      JsonTraits<SC_JSON_TYPE>::AddBool(owner, owner.json(), "v", true);
  ASSERT_NE(nullptr, node);
  EXPECT_TRUE(TraitGet<bool>(*node));
}

TEST(SC_JSON_TEST_SUITE_NAME, WriteAddNull) {
  JsonOwner<SC_JSON_TYPE> owner(std::string_view(R"({})"));
  SC_JSON_TYPE* node = JsonTraits<SC_JSON_TYPE>::AddNull(owner, owner.json(), "v");
  ASSERT_NE(nullptr, node);
  EXPECT_TRUE(JsonTraits<SC_JSON_TYPE>::IsNull(*node));
}

TEST(SC_JSON_TEST_SUITE_NAME, WriteAddString) {
  JsonOwner<SC_JSON_TYPE> owner(std::string_view(R"({})"));
  SC_JSON_TYPE* node =
      JsonTraits<SC_JSON_TYPE>::AddString(owner, owner.json(), "v", "hi");
  ASSERT_NE(nullptr, node);
  EXPECT_EQ(std::string("hi"), TraitGet<std::string>(*node));
}

TEST(SC_JSON_TEST_SUITE_NAME, WriterAssignByPath) {
  JsonOwner<SC_JSON_TYPE> owner(std::string_view(R"({"a":{},"scalar":7})"));
  JsonWriter<SC_JSON_TYPE> writer(owner);

  auto child = [&](std::string_view key) {
    return JsonTraits<SC_JSON_TYPE>::TryGetChild(owner.json(), key);
  };

  writer["i"_path] = 42;
  writer["u"_path] = 7u;
  writer["i64"_path] = -9007199254740993LL;
  writer["u64"_path] = 18446744073709551615ULL;
  writer["d"_path] = 3.5;
  writer["b"_path] = true;
  writer["s"_path] = std::string("hi");
  writer["n"_path] = nullptr;

  ASSERT_NE(nullptr, child("i"));
  EXPECT_EQ(42, TraitGet<int>(*child("i")));
  EXPECT_EQ(7u, TraitGet<unsigned int>(*child("u")));
  EXPECT_EQ(-9007199254740993LL,
            TraitGet<std::int64_t>(*child("i64")));
  EXPECT_EQ(18446744073709551615ULL,
            TraitGet<std::uint64_t>(*child("u64")));
  EXPECT_DOUBLE_EQ(3.5, TraitGet<double>(*child("d")));
  EXPECT_TRUE(TraitGet<bool>(*child("b")));
  EXPECT_EQ(std::string("hi"),
            TraitGet<std::string>(*child("s")));
  EXPECT_TRUE(JsonTraits<SC_JSON_TYPE>::IsNull(*child("n")));

  writer["a.x"_path] = 5;
  const SC_JSON_TYPE* a = child("a");
  ASSERT_NE(nullptr, a);
  EXPECT_EQ(5, TraitGet<int>(
                   *JsonTraits<SC_JSON_TYPE>::TryGetChild(*a, "x")));

  // [] 默认 Required：缺失的中间节点自动建链
  writer["new.deep"_path] = 9;
  const SC_JSON_TYPE* created_new = child("new");
  ASSERT_NE(nullptr, created_new);
  EXPECT_EQ(9, TraitGet<int>(
                   *JsonTraits<SC_JSON_TYPE>::TryGetChild(*created_new, "deep")));

  // Set 默认 Optional：非法路径静默忽略
  writer.Set<Optional>(""_path, 1);
  writer.Set<Optional>("scalar.y"_path, 1);
  EXPECT_EQ(7, TraitGet<int>(*child("scalar")));

  // 显式 Required：空路径/父节点非对象上报错，缺失的中间节点自动建链
  EXPECT_THROW((writer.Set<Required>(""_path, 1)), std::runtime_error);
  EXPECT_THROW((writer.Set<Required>("scalar.y"_path, 1)),
               std::runtime_error);

  writer.Set<Required>("new.deep"_path, 9);
  const SC_JSON_TYPE* created = child("new");
  ASSERT_NE(nullptr, created);
  EXPECT_EQ(9, TraitGet<int>(
                   *JsonTraits<SC_JSON_TYPE>::TryGetChild(*created, "deep")));
}

TEST(SC_JSON_TEST_SUITE_NAME, WriterRemoveByPath) {
  JsonOwner<SC_JSON_TYPE> owner(
      std::string_view(R"({"a":{"x":1,"y":2},"list":[1,2],"scalar":7})"));
  JsonWriter writer(owner);

  auto child = [&](std::string_view key) {
    return JsonTraits<SC_JSON_TYPE>::TryGetChild(owner.json(), key);
  };

  // 深度 1：删除根下的成员
  writer.Remove("scalar"_path);
  EXPECT_EQ(nullptr, child("scalar"));

  // 深度 2：删除嵌套成员，父节点与兄弟成员保留
  writer.Remove("a.x"_path);
  const SC_JSON_TYPE* a = child("a");
  ASSERT_NE(nullptr, a);
  EXPECT_EQ(nullptr, JsonTraits<SC_JSON_TYPE>::TryGetChild(*a, "x"));
  EXPECT_NE(nullptr, JsonTraits<SC_JSON_TYPE>::TryGetChild(*a, "y"));

  // 默认策略是 Optional：空路径、缺失的中间路径、不存在的成员都静默忽略
  writer.Remove(""_path);
  writer.Remove("no.such"_path);
  writer.Remove("a.zz"_path);
  EXPECT_EQ(nullptr, child("no"));
  ASSERT_NE(nullptr, child("a"));

  // 父节点不是对象（数组）同样静默忽略
  writer.Remove("list.x"_path);
  EXPECT_NE(nullptr, child("list"));

  // 显式 Required：以上非法路径都上报
  EXPECT_THROW((writer.Remove<Required>(""_path)), std::runtime_error);
  EXPECT_THROW((writer.Remove<Required>("no.such"_path)), std::runtime_error);
  EXPECT_THROW((writer.Remove<Required>("a.zz"_path)), std::runtime_error);
  EXPECT_THROW((writer.Remove<Required>("list.x"_path)), std::runtime_error);
}

TEST(SC_JSON_TEST_SUITE_NAME, OwnerDefaultConstructedIsEmptyObject) {
  JsonOwner<SC_JSON_TYPE> owner;
  ASSERT_TRUE(owner.valid());

  JsonView view(owner);
  EXPECT_TRUE(view.IsObject());
  EXPECT_EQ(JsonTraits<SC_JSON_TYPE>::MemberBegin(*view.node()),
            JsonTraits<SC_JSON_TYPE>::MemberEnd(*view.node()));

  // 空白文档可以直接作为写入目标
  JsonWriter writer(owner);
  writer["i"_path] = 42;
  writer.Set<Required>("a.b"_path, 7);
  EXPECT_EQ(42, view.View<Required>("i"_path).GetInt());
  EXPECT_EQ(7, view.View<Required>("a.b"_path).GetInt());
}

TEST(SC_JSON_TEST_SUITE_NAME, OwnerViewWriterUseIndependentErrorHandlers) {
  using ErrorHandler = SC_JSON_CONCAT(ErrorHandler_, SC_JSON_TEST_SUITE_NAME);
  ErrorHandler handler;
  ErrorHandler::resolve_errors.clear();
  ErrorHandler::mismatches.clear();
  JsonOwner<SC_JSON_TYPE, ErrorHandler> owner(
      std::string_view(R"({"a":{"b":42}})"), handler);

  // 自定义 handler 的 owner 可以喂给默认 handler 的 view / writer
  JsonView view(owner);
  EXPECT_EQ(42, view.View<Required>("a.b"_path).GetInt());

  JsonWriter<SC_JSON_TYPE> writer(owner);
  writer["i"_path] = 7;
  EXPECT_EQ(7, view.View<Required>("i"_path).GetInt());

  // 反之亦然：默认 handler 的 owner 可以喂给自定义 handler 的 view
  JsonOwner<SC_JSON_TYPE> plain(std::string_view(R"({"a":{"b":42}})"));
  JsonView<SC_JSON_TYPE, ErrorHandler> custom(plain, handler);
  EXPECT_EQ(42, custom.View<Required>("a.b"_path).GetInt());

  JsonWriter<SC_JSON_TYPE, ErrorHandler> custom_writer(plain, handler);
  custom_writer.Set<Required>(""_path, 1);
  EXPECT_EQ(1u, ErrorHandler::resolve_errors.size());
}

struct SC_JSON_CONCAT(Reflected_, SC_JSON_TEST_SUITE_NAME) {
  std::int64_t neg = 0;
  double d = 0;
  std::string s;
};

TEST(SC_JSON_TEST_SUITE_NAME, GetTypeFillsStruct) {
  using Reflected = SC_JSON_CONCAT(Reflected_, SC_JSON_TEST_SUITE_NAME);
  const SC_JSON_TYPE json = SC_JSON_MAKE_JSON();
  JsonView<SC_JSON_TYPE> view(json);

  const auto value = view.Get<Reflected>(""_path);
  EXPECT_EQ(-7, value.neg);
  EXPECT_DOUBLE_EQ(3.14, value.d);
  EXPECT_EQ(std::string("hello"), value.s);

  Reflected target;
  view.Put<Reflected>(""_path, target);
  EXPECT_EQ(-7, target.neg);
  EXPECT_EQ(std::string("hello"), target.s);

  const auto partial = view.Get<Reflected>("a"_path);
  EXPECT_EQ(0, partial.neg);
  EXPECT_DOUBLE_EQ(0.0, partial.d);
  EXPECT_TRUE(partial.s.empty());
}

struct SC_JSON_CONCAT(NestedPoint_, SC_JSON_TEST_SUITE_NAME) {
  int x = 0;
  int y = 0;
};

struct SC_JSON_CONCAT(NestedBag_, SC_JSON_TEST_SUITE_NAME) {
  SC_JSON_CONCAT(NestedPoint_, SC_JSON_TEST_SUITE_NAME) point;
  double d = 0;
  std::string s;
};

TEST(SC_JSON_TEST_SUITE_NAME, GetTypeWithNestedStruct) {
  using Bag = SC_JSON_CONCAT(NestedBag_, SC_JSON_TEST_SUITE_NAME);
  const SC_JSON_TYPE json = SC_JSON_MAKE_JSON();
  JsonView<SC_JSON_TYPE> view(json);

  const auto bag = view.Get<Bag>(""_path);
  EXPECT_EQ(10, bag.point.x);
  EXPECT_EQ(20, bag.point.y);
  EXPECT_DOUBLE_EQ(3.14, bag.d);
  EXPECT_EQ(std::string("hello"), bag.s);

  Bag target;
  view.Put<Bag>(""_path, target);
  EXPECT_EQ(10, target.point.x);
  EXPECT_EQ(20, target.point.y);

  const auto point = view.Get<
      SC_JSON_CONCAT(NestedPoint_, SC_JSON_TEST_SUITE_NAME)>("point"_path);
  EXPECT_EQ(10, point.x);
  EXPECT_EQ(20, point.y);
}

struct SC_JSON_CONCAT(Item_, SC_JSON_TEST_SUITE_NAME) {
  std::int64_t id = 0;
};

struct SC_JSON_CONCAT(ContainerBag_, SC_JSON_TEST_SUITE_NAME) {
  std::vector<std::int64_t> list;
  std::map<std::string, int> point;
  std::vector<SC_JSON_CONCAT(Item_, SC_JSON_TEST_SUITE_NAME)> obj_list;
};

TEST(SC_JSON_TEST_SUITE_NAME, GetTypeWithContainers) {
  using Bag = SC_JSON_CONCAT(ContainerBag_, SC_JSON_TEST_SUITE_NAME);
  const SC_JSON_TYPE json = SC_JSON_MAKE_JSON();
  JsonView<SC_JSON_TYPE> view(json);

  const auto bag = view.Get<Bag>(""_path);
  EXPECT_EQ((std::vector<std::int64_t>{1, 2, 3}), bag.list);
  ASSERT_EQ(2u, bag.point.size());
  EXPECT_EQ(10, bag.point.at("x"));
  EXPECT_EQ(20, bag.point.at("y"));
  ASSERT_EQ(2u, bag.obj_list.size());
  EXPECT_EQ(1, bag.obj_list[0].id);
  EXPECT_EQ(2, bag.obj_list[1].id);
}

TEST(SC_JSON_TEST_SUITE_NAME, SetTypeWithContainers) {
  using Bag = SC_JSON_CONCAT(ContainerBag_, SC_JSON_TEST_SUITE_NAME);
  using Item = SC_JSON_CONCAT(Item_, SC_JSON_TEST_SUITE_NAME);

  JsonOwner<SC_JSON_TYPE> owner(std::string_view(R"({})"));
  JsonWriter<SC_JSON_TYPE> writer(owner);

  Bag bag;
  bag.list = {1, 2, 3};
  bag.point = {{"x", 10}, {"y", 20}};
  Item first;
  first.id = 1;
  Item second;
  second.id = 2;
  bag.obj_list = {first, second};

  writer["bag"_path] = bag;

  JsonView<SC_JSON_TYPE> view(owner.json());
  const auto round = view.Get<Bag>("bag"_path);
  EXPECT_EQ((std::vector<std::int64_t>{1, 2, 3}), round.list);
  ASSERT_EQ(2u, round.point.size());
  EXPECT_EQ(10, round.point.at("x"));
  EXPECT_EQ(20, round.point.at("y"));
  ASSERT_EQ(2u, round.obj_list.size());
  EXPECT_EQ(1, round.obj_list[0].id);
  EXPECT_EQ(2, round.obj_list[1].id);
}
