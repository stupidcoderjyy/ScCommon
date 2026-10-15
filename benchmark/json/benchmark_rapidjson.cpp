//
// Created by jiangyuyun3 on 2026/10/15.
//

#include <rapidjson/document.h>

#include <cstdint>
#include <map>
#include <stdexcept>
#include <string>
#include <string_view>

#define SC_BENCH_PREFIX rapidjson_
#define SC_BENCH_TYPE rapidjson::Value
#define SC_BENCH_JSON_TEXT()                            \
  std::string_view(                                     \
      R"({"a":{"b":42},"u":7,"i64":-9007199254740993,)" \
      R"("u64":9007199254740993,"d":3.5,"flag":true,)"  \
      R"("s":"hello","list":[1,2,3,4,5],"obj":{"x":1,"y":2,"z":3}})")

// rapidjson::Value 的成员内存归分配器管理，用静态内存池保证拷贝出的
// Value 在基准期间始终有效。
rapidjson::MemoryPoolAllocator<>& TestPool() {
  static rapidjson::MemoryPoolAllocator<> pool;
  return pool;
}

namespace {

rapidjson::Value MakeJson() {
  rapidjson::Document doc;
  doc.Parse(SC_BENCH_JSON_TEXT().data(), SC_BENCH_JSON_TEXT().size());
  if (doc.HasParseError()) {
    throw std::runtime_error("rapidjson parse error");
  }
  return {doc, TestPool()};
}

}  // namespace

#define SC_BENCH_NATIVE_ALLOC(doc) (::TestPool())
#define SC_BENCH_MAKE_JSON() MakeJson()
#include "benchmark_suite.inc"

std::map<std::string, std::int64_t> SC_BENCH_NATIVE_OBJECT_MAP(
    const SC_BENCH_TYPE& json) {
  std::map<std::string, std::int64_t> out;
  const auto& obj = json["obj"];
  for (auto it = obj.MemberBegin(); it != obj.MemberEnd(); ++it) {
    out.emplace(std::string(it->name.GetString(), it->name.GetStringLength()),
                it->value.GetInt64());
  }
  return out;
}

std::int64_t SC_BENCH_NATIVE_OBJECT_SUM(const SC_BENCH_TYPE& json) {
  std::int64_t sum = 0;
  const auto& obj = json["obj"];
  for (auto it = obj.MemberBegin(); it != obj.MemberEnd(); ++it) {
    sum += it->value.GetInt64();
    sum += static_cast<std::int64_t>(it->name.GetStringLength());
  }
  return sum;
}

std::pair<std::vector<std::int64_t>, std::int64_t> SC_BENCH_NATIVE_ARRAY_SCAN(
    const SC_BENCH_TYPE& json) {
  const auto& list = json["list"];
  std::vector<std::int64_t> copied;
  copied.reserve(list.Size());
  std::int64_t sum = 0;
  for (auto it = list.Begin(); it != list.End(); ++it) {
    copied.push_back(it->GetInt64());
    sum += it->GetInt64();
  }
  return {std::move(copied), sum};
}
