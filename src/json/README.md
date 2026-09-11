# ScCommon · JsonParser

基于 **C++20** 的轻量 JSON 路径解析库：编译期路径、零拷贝视图、无状态只读、跨 JSON 库（nlohmann / RapidJSON / JsonCpp）统一接口。

```cpp
#include <nlohmann/json.hpp>
#include "json/json_parser.h"

using namespace sc::json;   // 使用 "_path" 必须引入该命名空间

const nlohmann::json doc = nlohmann::json::parse(R"({"model":{"version":3,"users":[{"name":"Alice"}]}})");
JsonParser parser(doc);

int version = 0;
parser.Acquire<JsonInt, Required>("model.version"_path, version);

for (auto&& user : parser.ViewArray("model.users"_path)) {
    std::string name;
    user.Acquire<JsonString>("name"_path, name);
}
```

---

## 1. 特性一览

| 特性 | 说明 |
|---|---|
| **编译期路径** | `"a.b[2].c"_path` 在编译期编译成 token 数组，运行时零字符串解析；非法路径（空段、`[` 未闭合、下标非数字）直接编译失败 |
| **传出参取值** | `Acquire` / `AcquireArray` / `AcquireMap` 直接把结果写进传入的变量/容器，并返回 `*this` 支持链式调用 |
| **零拷贝视图** | `JsonView` 只有 2 个指针（16 字节）；`JsonArrayView` / `JsonObjectView` 惰性遍历，不构造中间容器 |
| **标准 ranges** | `JsonArrayView` 满足 `random_access_range`，`JsonObjectView` 满足 `forward_range`，可直接接入 `std::views` 管道 |
| **无状态只读** | 解析过程不修改 `JsonParser`，天然可并发、可任意交叉嵌套；`JsonParser` 本身不可拷贝/移动 |
| **多库适配** | 同一套代码支持 nlohmann/json、RapidJSON、JsonCpp；通过 `JsonTraits` 可扩展新库 |
| **策略化错误处理** | `Optional`（默认，失败返回默认值）/ `Required`（失败走错误处理器，默认抛异常） |
| **自定义错误处理器** | 可携带外部数据（如请求 ID）抛出业务异常；解析 JSON 文本失败同样走处理器 |
| **接口对称** | `JsonParser` 与 `JsonView` 共同继承 `JsonAccessMixin`，`Acquire` / `AcquireArray` / `AcquireMap` 只有一份实现；同一段泛型代码对两者都适用 |
| **节点操作直通** | `JsonTraits<Json>` 的全部操作（`Is*` / `Get*` / `TryGetChild` / `MemberBegin` …）直接作为 `JsonParser`、`JsonView` 的成员，无需再写 `JsonTraits<Json>::X(parser.json())` |

---

## 2. 环境要求

- **C++20**（GCC 12+ / Clang 15+ / MSVC 19.32+ 即 VS2022 17.x）
  - `_path` 依赖 `consteval` + 类类型 NTTP，C++17 无法编译；
  - VS2019（19.29）对类类型 NTTP 支持不完整，建议 VS2022。
- 至少一个 JSON 库：nlohmann/json、RapidJSON、JsonCpp（`json_parser.h` 通过 `__has_include` 自动适配已安装的库）。

---

## 3. 目录结构

```
src/json/
├── json_parser.h        // 总入口：JsonParser（含 traits 自动适配）
├── json_parser.hpp      // 兼容用的转发头，内容同 json_parser.h
├── json_path.h          // 路径：Parts<N> / "_path" UDL / PathLike / RuntimePath
├── json_error.h         // Required / Optional / DefaultJsonParserErrorHandler
├── json_extractor.h     // Extractor<Json,T> 与 JsonInt/JsonInt64/... 别名
├── json_traits.h        // JsonTraits 契约（各库适配的扩展点）
├── json_access_mixin.h  // CRTP mixin：Acquire / AcquireArray / AcquireMap 的唯一实现
├── json_traits_mixin.h  // CRTP mixin：把 JsonTraits 的节点操作暴露为成员函数
├── json_detail.h        // 内部工具（路径格式化、单步解析）
├── traits/              // 各 JSON 库的 JsonTraits / JsonBuilder 特化
│   ├── nlohmann.h  ├── rapidjson.h  └── jsoncpp.h
└── view/                // 视图层
    ├── json_view.h          // JsonView
    ├── json_array_view.h    // JsonArrayView
    ├── json_object_view.h   // JsonObjectView
    └── json_views.h         // 视图层总入口
```

---

## 4. 快速开始

```cpp
#include <nlohmann/json.hpp>
#include "json/json_parser.h"

using namespace sc::json;

const nlohmann::json doc = nlohmann::json::parse(R"({
    "users":  [{"name":"Alice","age":30},{"name":"Bob","age":17}],
    "config": {"timeout":30,"retry":3,"debug":true}
})");

// 1) 从已有节点构造（零拷贝，只持有 const 引用）
JsonParser parser(doc);

// 2) 从文本构造（JsonBuilder 内部持有解析结果；文本非法时走 OnJsonParseError）
JsonParser<nlohmann::json> from_text(std::string_view(R"({"a":{"b":42}})"));
```

> 文本构造**必须显式写出 Json 类型**：纯文本无法推导出用哪个 JSON 库；只写 `JsonParser p(text)` 会因两个构造器都可行而歧义，或（对某些类型）推导出错误类型。
> 从已有节点构造则可以省略：`JsonParser p(doc);` 会正确推导。

---

## 5. 路径

### 5.1 编译期路径 `_path`（推荐）

```cpp
using namespace sc::json;            // 或者：using sc::json::operator""_path;

constexpr auto p = "a.b[2].c.d"_path;   // → {"a","b","[2]","c","d"}
static_assert(p.Size() == 5);
static_assert(p[0] == "a");
```

规则（由 `static_assert` 在编译期拦截）：

| 写法 | 结果 |
|---|---|
| `"a.b[2].c"` | ✅ `{"a","b","[2]","c"}` |
| `""` | ✅ 空路径，表示**当前节点/根节点** |
| `"a..b"`、`".a"`、`"a."` | ❌ 空段，编译错误 |
| `"a[2"`、`"a[]"`、`"a[abc]"` | ❌ `'['` 未闭合 / 空下标 / 下标非数字，编译错误 |

注意：

1. `_path` 定义在 `sc::json`，外部项目**必须**写 `using namespace sc::json;` 或 `using sc::json::operator""_path;`（未引入时报 `unable to find string literal operator 'operator""_path'`）。
2. 路径必须是**字面量**（编译期常量），换来的是编译期校验与零解析开销。

### 5.2 运行期路径 `RuntimePath`

需要运行期拼接路径时，用 `RuntimePath`（同样满足 `PathLike`，可在 `Acquire` / `View*` 中直接使用）：

```cpp
const std::string key = req.param("field");
const RuntimePath path(key + ".sub[0]");   // 运行期切分成与 Parts<N> 相同的 token 序列

int v = 0;
parser.Acquire(path, v);                       // 推导写法
parser.Acquire<JsonInt, Required>(path, v);    // 显式写法
auto view = parser.View<Required>(path);       // View* 同样支持
```

| | 编译期 `Parts<N>` | 运行期 `RuntimePath` |
|---|---|---|
| 校验时机 | 编译期（空段 / `[` 未闭合 / 下标非数字直接编译失败） | 解析期，由 `OnPathResolveError` 报告 |
| 路径解析开销 | 0 | 每次构造时切分 token |
| 单次调用开销 | ~20 ns（1 段） | 预构造后 ~21 ns；每次现构造 ~63 ns |

`PathLike` 概念（`Size()` + `operator[]`）就是这两者的公共契约，用户也可以自定义满足该概念的路径类型。

---

## 6. 取值：Acquire 系列

`Acquire` / `AcquireArray` / `AcquireMap` 由 `JsonAccessMixin`（CRTP mixin）提供，`JsonParser` 与 `JsonView` 共用同一份实现：

每个取值接口有两种等价写法：

| 写法 | 形式 | 适用 |
|---|---|---|
| 显式提取器 | `Acquire<JsonInt, Required>(path, out)` | 想明确写出提取器类型时 |
| 类型推导 | `Acquire(path, out)` | 由 `out` 类型推导 `Extractor<Json,T>`；**没有显式模板实参，因此在泛型 lambda / 模板里无需写 `.template`** |

```cpp
template <typename Source>              // Source 可以是 JsonParser，也可以是任意 JsonView
void use(Source& source) {
    int v = 0;
    source.Acquire("a.b"_path, v);                       // 推导写法：无需 .template
    source.template Acquire<JsonInt, Required>("a.b"_path, v);  // 显式写法（依赖上下文需加 .template）
    std::vector<int> nums;
    source.AcquireArray("list"_path, nums);
    std::map<std::string, int> m;
    source.AcquireMap("m"_path, m);
}
```

### 6.1 `Acquire` —— 取标量/自定义类型

```cpp
// Optional（默认）：路径缺失或类型不符时返回默认值，不抛异常
int    a = 0;
parser.Acquire<JsonInt>("a.b"_path, a);
int    b = 0;
parser.Acquire<JsonInt>("missing"_path, b, 42);      // 缺失 → b = 42
double d = 0;
parser.Acquire<JsonDouble>("d"_path, d);
std::string s;
parser.Acquire<JsonString>("s"_path, s);
bool flag = false;
parser.Acquire<JsonBool>("flag"_path, flag);
std::int64_t big = 0;
parser.Acquire<JsonInt64>("big"_path, big);

// Required：路径缺失 → OnPathResolveError；类型不符 → OnTypeMismatch（默认抛 std::runtime_error）
int v = 0;
parser.Acquire<JsonInt, Required>("a.b"_path, v);

// 返回 *this，支持链式调用
int x = 0;
std::string y;
parser.Acquire<JsonInt>("a.b"_path, x).Acquire<JsonString>("s"_path, y);

// 由 out 类型推导提取器（等价写法，无需显式模板实参）
int z = 0;
parser.Acquire("a.b"_path, z);
```

### 6.2 `AcquireArray` —— 取数组

```cpp
std::vector<int> nums;
parser.AcquireArray<JsonInt, Required>("list"_path, nums);

// 容器类型由实参推导，任意序列容器均可
std::list<int> nums_list;
parser.AcquireArray<JsonInt, Required>("list"_path, nums_list);
```

### 6.3 `AcquireMap` —— 取对象

```cpp
std::map<std::string, int> cfg;
parser.AcquireMap<JsonInt, Required>("config"_path, cfg);
```

### 6.4 策略语义

| ExistencePolicy | 路径缺失 | 类型不符 | 元素类型不符（数组/对象内部） |
|---|---|---|---|
| `Optional`（默认） | `out` 写默认值 | `out` 写默认值 | `OnTypeMismatch(完整路径)` |
| `Required` | `OnPathResolveError` | `OnTypeMismatch(path)` | `OnTypeMismatch(完整路径)` |

- 默认值是 `Acquire(path, out, default)` 的第三个实参；不传则写入值初始化值（`int{}` = 0、空字符串等）。
- `AcquireArray` / `AcquireMap` 的元素错误会带完整路径，例如 `obj_list[0]`、`config.retry`。
- 调用前容器会被清空；自定义错误处理器若**吞掉**异常并返回，元素填充中止（容器保留已填充部分）。

---

## 7. 视图层

视图层是零拷贝、无状态的惰性访问接口，适合遍历与 `std::ranges` 组合。

### 7.1 `JsonView` —— 单节点

```cpp
auto view = parser.View("config"_path);        // Optional：缺失时返回无效 view
if (view) {                                     // 或 view.valid()
    const nlohmann::json& node = view.json();
}

auto required = parser.View<Required>("config"_path);   // 缺失 → OnPathResolveError

// 相对当前节点解析
int timeout = 0;
view.Acquire<JsonInt>("timeout"_path, timeout);
auto inner = view.View("timeout"_path);
```

- `JsonView` = 2 个指针（16 字节），可拷贝、可移动、可判空。
- **无效 view 上的 `Acquire` 等同 Optional**：向 `out` 写入默认值，不触发错误回调。
- 无效 view 上的 `View*` 一律返回空 view（即使策略是 `Required`，因为没有节点可解析）。

### 7.2 `JsonArrayView` —— 数组

```cpp
// 惰性遍历（满足 random_access_range：size()/empty()/begin()/end()，可随机访问）
for (auto&& user : parser.ViewArray("users"_path)) {
    std::string name;
    user.Acquire<JsonString>("name"_path, name);
}

auto users = parser.ViewArray<Required>("users"_path);   // 缺失/非数组 → 错误处理器
const std::size_t n = users.size();                      // O(1)
const auto& first = *(users.begin() + 0);                // 元素是 JsonView
```

### 7.3 `JsonObjectView` —— 对象

```cpp
// 满足 forward_range，元素为 std::pair<std::string_view, JsonView>，支持结构化绑定
for (auto&& [key, value] : parser.ViewObject("config"_path)) {
    std::cout << key << " = " << value.json().dump() << '\n';
}
```

> key 为 `string_view`，指向底层 JSON 存储；**JsonCpp 例外**：其 `name()` 按值返回字符串，key 由迭代器自身持有，仅在当前迭代器解引用有效期内可用。

### 7.4 与 `std::ranges` 组合

```cpp
auto adult_names = parser.ViewArray("users"_path)
    | std::views::filter([](auto&& u) {
          int age = 0;
          u.Acquire("age"_path, age);          // 推导写法：泛型 lambda 里也无需 .template
          return age >= 18;
      })
    | std::views::transform([](auto&& u) {
          std::string name;
          u.Acquire("name"_path, name);
          return name;
      });

for (const std::string& name : adult_names) std::cout << name << '\n';
```

---

## 8. 节点操作（JsonTraits 直通）

`JsonParser` 与 `JsonView` 都直接暴露 `JsonTraits<Json>` 的全部操作，不必再写
`JsonTraits<Json>::X(parser.json())`；两边完全对称（同一份实现来自 `JsonTraitsMixin`）：

```cpp
// 类型判定
parser.IsObject();
view.IsArray();
view.IsString();

// 结构与子节点
const Json* child = parser.TryGetChild("a");     // 未命中返回 nullptr
std::size_t n = view.GetArraySize();
const Json& elem = view.GetArrayElement(1);
for (auto it = parser.MemberBegin(); it != parser.MemberEnd(); ++it) { /* ... */ }

// 取值
int v = view.GetInt();
std::int64_t big = view.GetInt64();
std::string s = view.GetString();
```

| 类别 | 成员 |
|---|---|
| 判定 | `IsObject` `IsArray` `IsInt` `IsUint` `IsInt64` `IsUint64` `IsDouble` `IsString` `IsBool` |
| 结构 | `TryGetChild` `GetArraySize` `GetArrayElement` `MemberBegin` `MemberEnd` |
| 取值 | `GetInt` `GetUint` `GetInt64` `GetUint64` `GetDouble` `GetString` `GetBool` |

语义：

- 与 traits 同名同义，直接转发到 `JsonTraits<Json>::X`，**无额外开销**（实测 1.61 ns vs 1.65 ns）；
- **无效 view 上安全**：判定返回 `false`，`TryGetChild` 返回 `nullptr`，`GetArraySize` 返回 `0`，
  取值类返回默认构造值，`GetArrayElement` 返回静态空节点，`MemberBegin == MemberEnd`；
- 取值前请按 traits 约定先用 `Is*` 判定（例如 RapidJSON 的 `GetUint()` 要求值确实是 unsigned 标志）。

## 9. 提取器（Extractor）

内置别名：`JsonInt`、`JsonInt64`、`JsonDouble`、`JsonString`、`JsonBool`。

`Is<整型>` 采用**无损转换**语义：只要该 JSON 整数值能无损转成目标类型即为真（例如 `901234567890` 满足 `IsInt64` 但不满足 `IsInt`）。

### 自定义提取器

```cpp
struct Point { int x = 0; int y = 0; };

template <>
struct sc::json::Extractor<Json::Value, Point> {
    using ValueType = Point;

    static bool Extract(const Json::Value& node, Point& out) {
        if (!JsonTraits<Json::Value>::IsObject(node)) return false;
        const Json::Value* x = JsonTraits<Json::Value>::TryGetChild(node, "x");
        const Json::Value* y = JsonTraits<Json::Value>::TryGetChild(node, "y");
        if (!x || !y) return false;
        out.x = JsonTraits<Json::Value>::GetInt(*x);
        out.y = JsonTraits<Json::Value>::GetInt(*y);
        return true;
    }
};

template <typename Json>
using PointExtractor = sc::json::Extractor<Json, Point>;

Point p;
parser.Acquire<PointExtractor, Required>("point"_path, p);
```

---

## 10. 错误处理

### 10.1 协议

```cpp
struct DefaultJsonParserErrorHandler {
    static void OnPathResolveError(const std::exception& e);            // 路径解析失败
    static void OnTypeMismatch(std::string_view path);                  // 节点类型不符
    static void OnJsonParseError(std::string_view json, std::exception& e);  // JSON 文本非法
};
```

默认实现直接抛 `std::runtime_error`；路径错误的消息形如
`JsonParser: cannot resolve path 'a.b[2].c' at segment 'b[2]' (resolved 1 of 3 segments)`。

### 10.2 自定义处理器（可携带外部数据）

```cpp
class RequestError : public std::runtime_error {
public:
    RequestError(std::string id, std::string msg)
        : std::runtime_error(msg), id_(std::move(id)) {}
    const std::string& Id() const noexcept { return id_; }
private:
    std::string id_;
};

struct RequestErrorHandler {
    explicit RequestErrorHandler(std::string id) : id_(std::move(id)) {}

    void OnPathResolveError(const std::exception& e) const {
        throw RequestError(id_, e.what());
    }
    void OnTypeMismatch(std::string_view path) const {
        throw RequestError(id_, "type mismatch at " + std::string(path));
    }
    void OnJsonParseError(std::string_view, std::exception& e) const {
        throw RequestError(id_, e.what());
    }

private:
    std::string id_;
};

JsonParser<Json::Value, RequestErrorHandler> parser(req.body(), RequestErrorHandler(req.id()));
```

处理器可以是**有状态对象**（构造 `JsonParser` 时传入），也可以只有静态方法（用默认构造）。

---

## 11. 支持的 JSON 库

| 库 | Json 类型 | 说明 |
|---|---|---|
| nlohmann/json | `nlohmann::json` | 完整支持，key 零拷贝 |
| RapidJSON | `rapidjson::Value` | 建议显式写 `JsonParser<rapidjson::Value>`；传入 `rapidjson::Document` 会被推导成 `JsonParser<Document>`（无 traits 特化） |
| JsonCpp | `Json::Value` | 完整支持；对象 key 由迭代器持有（见 7.3） |

`json_parser.h` 会用 `__has_include` 自动包含已安装库的 traits，无需手动 include。

---

## 12. 扩展新 JSON 库（JsonTraits 契约）

```cpp
template <>
struct sc::json::JsonTraits<MyJson> {
    using Json = MyJson;

    // 类型判定（Is<整型> 为“可无损转换”语义）
    static bool IsObject(const Json&);
    static bool IsArray(const Json&);
    static bool IsInt(const Json&);
    static bool IsUint(const Json&);
    static bool IsInt64(const Json&);
    static bool IsUint64(const Json&);
    static bool IsDouble(const Json&);
    static bool IsString(const Json&);
    static bool IsBool(const Json&);

    // 路径解析（路径解析只走 TryGetChild）
    static const Json* TryGetChild(const Json&, std::string_view key);
    static std::size_t GetArraySize(const Json&);
    static const Json& GetArrayElement(const Json&, std::size_t index);

    // 取值
    static int GetInt(const Json&);
    static unsigned int GetUint(const Json&);
    static std::int64_t GetInt64(const Json&);
    static std::uint64_t GetUint64(const Json&);
    static double GetDouble(const Json&);
    static std::string GetString(const Json&);
    static bool GetBool(const Json&);

    // 对象遍历：*it 得到 std::pair<std::string_view, const Json*>
    struct MemberIterator { /* 满足 forward_iterator */ };
    static MemberIterator MemberBegin(const Json&);
    static MemberIterator MemberEnd(const Json&);
};
```

若该库支持从文本解析，再特化 `JsonBuilder`：

```cpp
template <typename ErrorHandler>
class sc::json::JsonBuilder<MyJson, ErrorHandler> {
public:
    JsonBuilder() = default;
    JsonBuilder(std::string_view text, ErrorHandler handler = {}) {
        // 解析失败：handler.OnJsonParseError(text, e); 然后 std::rethrow / throw
        // 成功：把结果存进 json_
    }
    const MyJson& json() const { return json_; }

private:
    MyJson json_;
};
```

> `JsonBuilder` 需要提供默认构造（`JsonParser` 从已有节点构造时会默认构造它），并且**应删除拷贝/移动**——`JsonParser` 依赖它持有解析结果，移动 `JsonParser` 会导致 `root_` 悬垂，因此 parser 本身也禁用了拷贝/移动。

---

## 13. 生命周期、并发与约束

- `JsonParser` **不拥有** 外部传入的 JSON 节点，仅持有 `const Json&`；节点必须比 parser 存活更久。
- 从字符串构造时，解析结果由内部的 `JsonBuilder` 持有，生命周期与 parser 一致。
- 视图（`JsonView` / `JsonArrayView` / `JsonObjectView`）同样不拥有数据，且持有指向 `JsonParser` 错误处理器的指针，**不得比 parser 存活更久**。
- 视图产出的 `string_view` 指向底层 JSON 存储，底层数据销毁后失效。
- `JsonParser` 无可变状态，多个线程可对同一个 parser 并发调用 `Acquire` / `View*`（底层 JSON 数据只读）。
- `JsonParser` 的拷贝/移动均被删除；需要传递时请传引用或指针。
- 库是**只读**的：不提供任何 JSON 修改能力。

---

## 14. 性能参考

MinGW g++ 16.1、`-O2 -DNDEBUG`、本机实测（数值仅供量级参考）：

| 操作 | nlohmann | jsoncpp | rapidjson |
|---|---|---|---|
| `JsonParser(doc)` 构造（root） | ~2 ns | ~2 ns | ~2 ns |
| `Acquire` 1 段路径 | ~19 ns | ~17 ns | ~9 ns |
| `Acquire` 4 段路径 | ~58 ns | ~62 ns | ~25 ns |
| `Acquire` 缺失（Optional） | ~6 ns | — | — |
| `Acquire` 缺失（Required，抛异常） | ~2.2 µs | ~2.3 µs | ~2.1 µs |
| `ViewArray` 遍历（50 元素） | ~1.4 ns/元素，零分配 | — | — |
| `Acquire` 用 `RuntimePath`（预构造） | ~21 ns | — | — |
| `Acquire` 用 `RuntimePath`（每次现构造） | ~63 ns | — | — |

要点：

- 编译期路径 + 单次 `TryGetChild` 查找，`Acquire` 已接近库原生访问速度；
- 视图层不为遍历分配任何容器；
- 运行期路径只有构造 `RuntimePath` 时付出切分成本，路径本身为空的解析开销与编译期路径一致；
- Required 缺失的 ~2 µs 是 C++ 异常机制本身的开销，高频可选字段建议用 `Optional` + 默认值。

---

## 15. 构建、安装与测试

```bash
# 构建
cmake -S . -B cmake-build-debug -G Ninja -DCMAKE_CXX_COMPILER=C:/msys64/mingw64/bin/c++.exe
cmake --build cmake-build-debug --target sccommon_test_all -j 8

# 运行测试（183 个用例：3 个 JSON 库 × 57 + 编译期路径 12）
./cmake-build-debug/bin/sccommon_test_all.exe

# 安装（默认前缀为 CMakeLists 里的 LIB_PREFIX_PATH，通常是 C:/msys64/mingw64）
cmake --install cmake-build-debug
cmake --install cmake-build-debug --prefix E:/your-prefix
```

在其它 CMake 工程中使用：

```cmake
find_package(sccommon REQUIRED)
target_link_libraries(app PRIVATE sccommon::json)
target_compile_features(app PRIVATE cxx_std_20)
```

测试目标：`sccommon_test_json_nlohmann`、`sccommon_test_json_rapidjson`、`sccommon_test_json_jsoncpp`、`sccommon_test_json_path`、`sccommon_test_all`。

---

## 16. 常见问题

**Q: `"a.b"_path` 报 `unable to find string literal operator 'operator""_path'`**

字面量运算符定义在 `sc::json` 中，使用处必须引入：

```cpp
using namespace sc::json;            // 或
using sc::json::operator""_path;
```

**Q: 外部项目里 `_path` / 新头文件找不到**

1. 确认用的是**新的安装结果**：安装目录下应同时存在 `json/json_path.h`、`json/json_detail.h`、`json/json_error.h`、`json/view/`；旧版本没有这些文件，需重新执行 `cmake --install`。
2. 确认编译标准是 **C++20**（C++17 会报 `'consteval' does not name a type`）。
3. 确认只保留一个头文件来源（安装目录或源码目录 `-I <repo>/src`），避免命中旧副本。

**Q: `JsonParser p(text)` 编译不过**

文本构造必须显式指定 Json 类型，见 §4 的说明。

**Q: 泛型代码里要写 `u.template Acquire<JsonInt>(...)`，很难看**

用「类型推导」写法即可，它没有显式模板实参，因此不需要 `template`：

```cpp
std::string name;
u.Acquire("name"_path, name);        // 而不是 u.template Acquire<JsonString>("name"_path, name)
```

原因是 C++ 规定：在依赖上下文（模板、泛型 lambda）中，对未知类型的成员使用**显式模板实参**时必须加 `template` 消歧义。

**Q: 路径需要在运行期拼接（来自配置、请求参数等）**

用 `RuntimePath`：

```cpp
const RuntimePath path(std::string(key) + ".sub[0]");
parser.Acquire<JsonInt, Required>(path, v);
```

它同样满足 `PathLike`，可与 `View*` / `Acquire*` 混用；区别只是失去编译期校验（非法路径由 `OnPathResolveError` 报告）。

**Q: 传入 `rapidjson::Document` 报 `JsonTraits<Document>::...` 未定义**

`JsonTraits` 只特化到 `rapidjson::Value`。请写成 `JsonParser<rapidjson::Value> parser(doc);`，或显式传 `rapidjson::Value& root = doc;`。

**Q: 想把 parser 存进容器或返回给调用方**

`JsonParser` 拷贝/移动均被删除（它持有引用与解析结果，移动会悬垂）。请传引用；需要返回请返回 `std::unique_ptr<JsonParser<...>>`，或直接在局部作用域内使用。

---

## 17. 已移除接口与迁移对照

当前版本已删除早期 API，迁移方式如下：

| 已移除 | 替代写法 |
|---|---|
| `parser.Parse<JsonInt>(path, cb)` | `int v = 0; parser.Acquire<JsonInt>(path, v);` |
| `JsonScope scope(parser, path)` | `auto v = parser.View<Required>(path);` 之后用 `v.Acquire(...)`（无状态，可并发） |
| `JsonArray` 提取器 | 直接遍历 `parser.ViewArray(path)` |
| `JsonObject` 提取器 | `parser.View(path).json()` 或遍历 `parser.ViewObject(path)` |
| `JsonTraits::HasKey` / `GetChild` | `TryGetChild`（一次查找，返回节点指针或 `nullptr`） |
| `JsonTraits::ForEachMember` | `MemberBegin` / `MemberEnd` 迭代 |
| 早期版本的运行期字符串路径 | 编译期字面量 `"a.b"_path`（推荐），或运行期 `RuntimePath("a.b")` |
