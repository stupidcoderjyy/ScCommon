//
// Created by jiangyuyun3 on 2026/10/15.
//

#include <cstdint>
#include <map>
#include <nlohmann/json.hpp>
#include <string>
#include <string_view>

#define SC_BENCH_PREFIX nlohmann_
#define SC_BENCH_FLAVOR_NLOHMANN
#define SC_BENCH_TYPE nlohmann::json
#define SC_BENCH_JSON_TEXT()                            \
  std::string_view(                                     \
      R"({"a":{"b":42},"u":7,"i64":-9007199254740993,)" \
      R"("u64":9007199254740993,"d":3.5,"flag":true,)"  \
      R"("s":"hello","list":[1,2,3,4,5],"obj":{"x":1,"y":2,"z":3}})")
#define SC_BENCH_MAKE_JSON() SC_BENCH_TYPE::parse(SC_BENCH_JSON_TEXT())
#include "benchmark_suite.inc"

std::map<std::string, std::int64_t> SC_BENCH_NATIVE_OBJECT_MAP(
    const SC_BENCH_TYPE& json) {
  std::map<std::string, std::int64_t> out;
  for (const auto& [key, value] : json["obj"].items()) {
    out.emplace(key, value.template get<std::int64_t>());
  }
  return out;
}

std::int64_t SC_BENCH_NATIVE_OBJECT_SUM(const SC_BENCH_TYPE& json) {
  std::int64_t sum = 0;
  for (const auto& [key, value] : json["obj"].items()) {
    sum += value.template get<std::int64_t>();
    sum += static_cast<std::int64_t>(key.size());
  }
  return sum;
}

std::pair<std::vector<std::int64_t>, std::int64_t> SC_BENCH_NATIVE_ARRAY_SCAN(
    const SC_BENCH_TYPE& json) {
  const auto& list = json["list"];
  std::vector<std::int64_t> copied;
  copied.reserve(list.size());
  std::int64_t sum = 0;
  for (const auto& element : list) {
    copied.push_back(element.template get<std::int64_t>());
    sum += element.template get<std::int64_t>();
  }
  return {std::move(copied), sum};
}
