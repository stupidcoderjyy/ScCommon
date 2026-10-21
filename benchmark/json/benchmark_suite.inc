// 每个 JSON 库的 benchmark 文件先定义以下宏，再包含本文件：
//   SC_BENCH_PREFIX      用例名前缀（如 nlohmann_）
//   SC_BENCH_TYPE        JSON 库类型
//   SC_BENCH_JSON_TEXT() 基准数据（字符串视图）
//   SC_BENCH_MAKE_JSON() 由上面的文本解析出的 JSON
//   SC_BENCH_FLAVOR_*    选择原生写法（nlohmann / jsoncpp / rapidjson）
#pragma once

#include <benchmark/benchmark.h>

#include <cstdint>
#include <map>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "json/json_view.h"
#include "json/json_writer.h"
#include "json/view/json_array_view.h"
#include "json/view/json_object_view.h"

using namespace sc::json;

#define SC_BENCH_CONCAT_IMPL(a, b) a##b
#define SC_BENCH_CONCAT(a, b) SC_BENCH_CONCAT_IMPL(a, b)
#define SC_BENCH_NAME(x) SC_BENCH_CONCAT(SC_BENCH_PREFIX, x)
#define SC_BENCH_STR_IMPL(x) #x
#define SC_BENCH_STR(x) SC_BENCH_STR_IMPL(x)
#define SC_BENCH_REGISTER(fn) BENCHMARK(fn)->Name(SC_BENCH_STR(fn))

#if defined(SC_BENCH_FLAVOR_NLOHMANN)
#define SC_BENCH_NATIVE_GET_INT(node) ((node).template get<int>())
#define SC_BENCH_NATIVE_GET_UINT(node) ((node).template get<unsigned int>())
#define SC_BENCH_NATIVE_GET_INT64(node) ((node).template get<std::int64_t>())
#define SC_BENCH_NATIVE_GET_UINT64(node) ((node).template get<std::uint64_t>())
#define SC_BENCH_NATIVE_GET_DOUBLE(node) ((node).template get<double>())
#define SC_BENCH_NATIVE_GET_BOOL(node) ((node).template get<bool>())
#define SC_BENCH_NATIVE_GET_STRING(node) ((node).template get<std::string>())
#define SC_BENCH_NATIVE_SET_INT(doc, node, v) ((node) = (v))
#define SC_BENCH_NATIVE_SET_DOUBLE(doc, node, v) ((node) = (v))
#define SC_BENCH_NATIVE_SET_BOOL(doc, node, v) ((node) = (v))
#define SC_BENCH_NATIVE_SET_STRING(doc, node, v) ((node) = std::string(v))
#define SC_BENCH_NATIVE_ADD_INT(doc, key, v) ((doc)[key] = (v))
#define SC_BENCH_NATIVE_ADD_STRING(doc, key, v) ((doc)[key] = std::string(v))
#define SC_BENCH_NATIVE_ADD_NESTED_INT(doc, parent, key, v) \
  ((doc)[parent][key] = (v))
#define SC_BENCH_NATIVE_ALLOC(doc)
#elif defined(SC_BENCH_FLAVOR_JSONCPP)
#define SC_BENCH_NATIVE_GET_INT(node) ((node).asInt())
#define SC_BENCH_NATIVE_GET_UINT(node) ((node).asUInt())
#define SC_BENCH_NATIVE_GET_INT64(node) ((node).asInt64())
#define SC_BENCH_NATIVE_GET_UINT64(node) ((node).asUInt64())
#define SC_BENCH_NATIVE_GET_DOUBLE(node) ((node).asDouble())
#define SC_BENCH_NATIVE_GET_BOOL(node) ((node).asBool())
#define SC_BENCH_NATIVE_GET_STRING(node) ((node).asString())
#define SC_BENCH_NATIVE_SET_INT(doc, node, v) ((node) = (v))
#define SC_BENCH_NATIVE_SET_DOUBLE(doc, node, v) ((node) = (v))
#define SC_BENCH_NATIVE_SET_BOOL(doc, node, v) ((node) = (v))
#define SC_BENCH_NATIVE_SET_STRING(doc, node, v) ((node) = std::string(v))
#define SC_BENCH_NATIVE_ADD_INT(doc, key, v) ((doc)[key] = (v))
#define SC_BENCH_NATIVE_ADD_STRING(doc, key, v) ((doc)[key] = std::string(v))
#define SC_BENCH_NATIVE_ADD_NESTED_INT(doc, parent, key, v) \
  ((doc)[parent][key] = (v))
#define SC_BENCH_NATIVE_ALLOC(doc)
#else
#define SC_BENCH_NATIVE_GET_INT(node) ((node).GetInt())
#define SC_BENCH_NATIVE_GET_UINT(node) ((node).GetUint())
#define SC_BENCH_NATIVE_GET_INT64(node) ((node).GetInt64())
#define SC_BENCH_NATIVE_GET_UINT64(node) ((node).GetUint64())
#define SC_BENCH_NATIVE_GET_DOUBLE(node) ((node).GetDouble())
#define SC_BENCH_NATIVE_GET_BOOL(node) ((node).GetBool())
#define SC_BENCH_NATIVE_GET_STRING(node) ((node).GetString())
#define SC_BENCH_NATIVE_SET_INT(doc, node, v) ((node).SetInt(v))
#define SC_BENCH_NATIVE_SET_DOUBLE(doc, node, v) ((node).SetDouble(v))
#define SC_BENCH_NATIVE_SET_BOOL(doc, node, v) ((node).SetBool(v))
#define SC_BENCH_NATIVE_SET_STRING(doc, node, v) \
  ((node).SetString((v), SC_BENCH_NATIVE_ALLOC(doc)))
#define SC_BENCH_NATIVE_ADD_INT(doc, key, v)             \
  do {                                                   \
    if ((doc).HasMember(key)) {                          \
      (doc)[key].SetInt(v);                              \
    } else {                                             \
      (doc).AddMember((key), (v), SC_BENCH_NATIVE_ALLOC(doc)); \
    }                                                    \
  } while (false)
#define SC_BENCH_NATIVE_ADD_STRING(doc, key, v)             \
  do {                                                      \
    if ((doc).HasMember(key)) {                             \
      (doc)[key].SetString((v), SC_BENCH_NATIVE_ALLOC(doc)); \
    } else {                                                \
      (doc).AddMember((key), rapidjson::StringRef(v),       \
                      SC_BENCH_NATIVE_ALLOC(doc));          \
    }                                                       \
  } while (false)
#define SC_BENCH_NATIVE_ADD_NESTED_INT(doc, parent, key, v) \
  do {                                                      \
    if (!(doc).HasMember(parent)) {                         \
      (doc).AddMember((parent),                             \
                      rapidjson::Value(rapidjson::kObjectType), \
                      SC_BENCH_NATIVE_ALLOC(doc));          \
    }                                                       \
    if ((doc)[parent].HasMember(key)) {                     \
      (doc)[parent][key].SetInt(v);                         \
    } else {                                                \
      (doc)[parent].AddMember((key), (v),                   \
                              SC_BENCH_NATIVE_ALLOC(doc));  \
    }                                                       \
  } while (false)
#endif

std::map<std::string, std::int64_t> SC_BENCH_NATIVE_OBJECT_MAP(
    const SC_BENCH_TYPE& json);
std::int64_t SC_BENCH_NATIVE_OBJECT_SUM(const SC_BENCH_TYPE& json);
std::pair<std::vector<std::int64_t>, std::int64_t> SC_BENCH_NATIVE_ARRAY_SCAN(
    const SC_BENCH_TYPE& json);

// ---------------- 1. 读：标量 ----------------

static void SC_BENCH_NAME(AccessScalarOurs)(benchmark::State& state) {
  const auto json = SC_BENCH_MAKE_JSON();
  JsonView<SC_BENCH_TYPE> view(json);
  for (auto _ : state) {
    int i = 0;
    unsigned int u = 0;
    std::int64_t i64 = 0;
    std::uint64_t u64 = 0;
    double d = 0;
    bool flag = false;
    std::string s;
    view.Put<int, Required>("a.b"_path, i);
    view.Put<unsigned int, Required>("u"_path, u);
    view.Put<std::int64_t, Required>("i64"_path, i64);
    view.Put<std::uint64_t, Required>("u64"_path, u64);
    view.Put<double, Required>("d"_path, d);
    view.Put<bool, Required>("flag"_path, flag);
    view.Put<std::string, Required>("s"_path, s);
    benchmark::DoNotOptimize(i);
    benchmark::DoNotOptimize(u);
    benchmark::DoNotOptimize(i64);
    benchmark::DoNotOptimize(u64);
    benchmark::DoNotOptimize(d);
    benchmark::DoNotOptimize(flag);
    benchmark::DoNotOptimize(s);
  }
}

static void SC_BENCH_NAME(AccessScalarNative)(benchmark::State& state) {
  const auto json = SC_BENCH_MAKE_JSON();
  for (auto _ : state) {
    const int i = SC_BENCH_NATIVE_GET_INT(json["a"]["b"]);
    const unsigned int u = SC_BENCH_NATIVE_GET_UINT(json["u"]);
    const std::int64_t i64 = SC_BENCH_NATIVE_GET_INT64(json["i64"]);
    const std::uint64_t u64 = SC_BENCH_NATIVE_GET_UINT64(json["u64"]);
    const double d = SC_BENCH_NATIVE_GET_DOUBLE(json["d"]);
    const bool flag = SC_BENCH_NATIVE_GET_BOOL(json["flag"]);
    const std::string s = SC_BENCH_NATIVE_GET_STRING(json["s"]);
    benchmark::DoNotOptimize(i);
    benchmark::DoNotOptimize(u);
    benchmark::DoNotOptimize(i64);
    benchmark::DoNotOptimize(u64);
    benchmark::DoNotOptimize(d);
    benchmark::DoNotOptimize(flag);
    benchmark::DoNotOptimize(s);
  }
}

SC_BENCH_REGISTER(SC_BENCH_NAME(AccessScalarOurs));
SC_BENCH_REGISTER(SC_BENCH_NAME(AccessScalarNative));

// ---------------- 2. 读：数组 ----------------

static void SC_BENCH_NAME(AccessArrayOurs)(benchmark::State& state) {
  const auto json = SC_BENCH_MAKE_JSON();
  JsonView<SC_BENCH_TYPE> view(json);
  for (auto _ : state) {
    auto copied =
        view.Get<std::vector<std::int64_t>, Required>("list"_path);
    const auto range = view.ViewArray<Required>("list"_path);
    std::int64_t sum = 0;
    for (const auto& element : range) {
      sum += element.GetInt64();
    }
    benchmark::DoNotOptimize(copied);
    benchmark::DoNotOptimize(sum);
    benchmark::DoNotOptimize(range.size());
  }
}

static void SC_BENCH_NAME(AccessArrayNative)(benchmark::State& state) {
  const auto json = SC_BENCH_MAKE_JSON();
  for (auto _ : state) {
    auto [copied, sum] = SC_BENCH_NATIVE_ARRAY_SCAN(json);
    benchmark::DoNotOptimize(copied);
    benchmark::DoNotOptimize(sum);
  }
}

SC_BENCH_REGISTER(SC_BENCH_NAME(AccessArrayOurs));
SC_BENCH_REGISTER(SC_BENCH_NAME(AccessArrayNative));

// ---------------- 3. 读：对象 ----------------

static void SC_BENCH_NAME(AccessObjectOurs)(benchmark::State& state) {
  const auto json = SC_BENCH_MAKE_JSON();
  JsonView<SC_BENCH_TYPE> view(json);
  for (auto _ : state) {
    auto copied = view.Get<std::map<std::string, std::int64_t>, Required>(
        "obj"_path);
    std::int64_t sum = 0;
    for (auto&& [key, value] : view.ViewObject<Required>("obj"_path)) {
      sum += value.GetInt64();
      sum += static_cast<std::int64_t>(key.size());
    }
    benchmark::DoNotOptimize(copied);
    benchmark::DoNotOptimize(sum);
  }
}

static void SC_BENCH_NAME(AccessObjectNative)(benchmark::State& state) {
  const auto json = SC_BENCH_MAKE_JSON();
  for (auto _ : state) {
    auto copied = SC_BENCH_NATIVE_OBJECT_MAP(json);
    const std::int64_t sum = SC_BENCH_NATIVE_OBJECT_SUM(json);
    benchmark::DoNotOptimize(copied);
    benchmark::DoNotOptimize(sum);
  }
}

SC_BENCH_REGISTER(SC_BENCH_NAME(AccessObjectOurs));
SC_BENCH_REGISTER(SC_BENCH_NAME(AccessObjectNative));

// ---------------- 4. 写：标量 ----------------

static void SC_BENCH_NAME(WriteScalarOurs)(benchmark::State& state) {
  JsonOwner<SC_BENCH_TYPE> owner(SC_BENCH_JSON_TEXT());
  JsonWriter<SC_BENCH_TYPE> writer(owner);
  for (auto _ : state) {
    writer["i"_path] = 42;
    writer["d"_path] = 3.5;
    writer["flag"_path] = true;
    writer["s"_path] = std::string("hello");
    benchmark::DoNotOptimize(owner.json());
  }
}

static void SC_BENCH_NAME(WriteScalarNative)(benchmark::State& state) {
  SC_BENCH_TYPE json = SC_BENCH_MAKE_JSON();
  for (auto _ : state) {
    SC_BENCH_NATIVE_SET_INT(json, json["i"], 42);
    SC_BENCH_NATIVE_SET_DOUBLE(json, json["d"], 3.5);
    SC_BENCH_NATIVE_SET_BOOL(json, json["flag"], true);
    SC_BENCH_NATIVE_SET_STRING(json, json["s"], "hello");
    benchmark::DoNotOptimize(json);
  }
}

SC_BENCH_REGISTER(SC_BENCH_NAME(WriteScalarOurs));
SC_BENCH_REGISTER(SC_BENCH_NAME(WriteScalarNative));

// ---------------- 5. 写：新建子节点 ----------------

static void SC_BENCH_NAME(WriteChildOurs)(benchmark::State& state) {
  JsonOwner<SC_BENCH_TYPE> owner(SC_BENCH_JSON_TEXT());
  for (auto _ : state) {
    auto* i = JsonTraits<SC_BENCH_TYPE>::AddInt(owner, owner.json(), "p", 42);
    auto* s =
        JsonTraits<SC_BENCH_TYPE>::AddString(owner, owner.json(), "q", "ti");
    benchmark::DoNotOptimize(i);
    benchmark::DoNotOptimize(s);
  }
}

static void SC_BENCH_NAME(WriteChildNative)(benchmark::State& state) {
  SC_BENCH_TYPE json = SC_BENCH_MAKE_JSON();
  for (auto _ : state) {
    SC_BENCH_NATIVE_ADD_INT(json, "p", 42);
    SC_BENCH_NATIVE_ADD_STRING(json, "q", "ti");
    benchmark::DoNotOptimize(json);
  }
}

SC_BENCH_REGISTER(SC_BENCH_NAME(WriteChildOurs));
SC_BENCH_REGISTER(SC_BENCH_NAME(WriteChildNative));

// ---------------- 6. 写：多段路径 ----------------

static void SC_BENCH_NAME(WriteNestedOurs)(benchmark::State& state) {
  JsonOwner<SC_BENCH_TYPE> owner(SC_BENCH_JSON_TEXT());
  JsonWriter<SC_BENCH_TYPE> writer(owner);
  for (auto _ : state) {
    writer["a.b"_path] = 1;
    writer.Set<Required>("new.deep"_path, 2);
    benchmark::DoNotOptimize(owner.json());
  }
}

static void SC_BENCH_NAME(WriteNestedNative)(benchmark::State& state) {
  SC_BENCH_TYPE json = SC_BENCH_MAKE_JSON();
  for (auto _ : state) {
    SC_BENCH_NATIVE_SET_INT(json, json["a"]["b"], 1);
    SC_BENCH_NATIVE_ADD_NESTED_INT(json, "new", "deep", 2);
    benchmark::DoNotOptimize(json);
  }
}

SC_BENCH_REGISTER(SC_BENCH_NAME(WriteNestedOurs));
SC_BENCH_REGISTER(SC_BENCH_NAME(WriteNestedNative));

// ---------------- 7. 读：结构化解析 ----------------

struct SC_BENCH_NAME(BenchStruct) {
  unsigned int u = 0;
  double d = 0;
  bool flag = false;
  std::string s;
};

static void SC_BENCH_NAME(AccessStructOurs)(benchmark::State& state) {
  const auto json = SC_BENCH_MAKE_JSON();
  JsonView<SC_BENCH_TYPE> view(json);
  for (auto _ : state) {
    auto value = view.Get<SC_BENCH_NAME(BenchStruct)>(""_path);
    benchmark::DoNotOptimize(value);
  }
}

static void SC_BENCH_NAME(AccessStructNative)(benchmark::State& state) {
  const auto json = SC_BENCH_MAKE_JSON();
  for (auto _ : state) {
    SC_BENCH_NAME(BenchStruct) value;
    value.u = SC_BENCH_NATIVE_GET_UINT(json["u"]);
    value.d = SC_BENCH_NATIVE_GET_DOUBLE(json["d"]);
    value.flag = SC_BENCH_NATIVE_GET_BOOL(json["flag"]);
    value.s = SC_BENCH_NATIVE_GET_STRING(json["s"]);
    benchmark::DoNotOptimize(value);
  }
}

SC_BENCH_REGISTER(SC_BENCH_NAME(AccessStructOurs));
SC_BENCH_REGISTER(SC_BENCH_NAME(AccessStructNative));
