//
// Created by jiangyuyun3 on 2026/10/15.
//

#include <json/json.h>

#include <cstdint>
#include <map>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>

#define SC_BENCH_PREFIX jsoncpp_
#define SC_BENCH_FLAVOR_JSONCPP
#define SC_BENCH_TYPE Json::Value
#define SC_BENCH_JSON_TEXT()                            \
  std::string_view(                                     \
      R"({"a":{"b":42},"u":7,"i64":-9007199254740993,)" \
      R"("u64":9007199254740993,"d":3.5,"flag":true,)"  \
      R"("s":"hello","list":[1,2,3,4,5],"obj":{"x":1,"y":2,"z":3}})")

namespace {

Json::Value MakeJson() {
  Json::CharReaderBuilder builder;
  Json::Value value;
  std::istringstream stream{std::string(SC_BENCH_JSON_TEXT())};
  std::string errors;
  if (!Json::parseFromStream(builder, stream, &value, &errors)) {
    throw std::runtime_error(errors);
  }
  return value;
}

}  // namespace

#define SC_BENCH_MAKE_JSON() MakeJson()
#include "benchmark_suite.inc"

std::map<std::string, std::int64_t> SC_BENCH_NATIVE_OBJECT_MAP(
    const SC_BENCH_TYPE& json) {
  std::map<std::string, std::int64_t> out;
  const auto& obj = json["obj"];
  for (auto it = obj.begin(); it != obj.end(); ++it) {
    out.emplace(it.name(), it->asInt64());
  }
  return out;
}

std::int64_t SC_BENCH_NATIVE_OBJECT_SUM(const SC_BENCH_TYPE& json) {
  std::int64_t sum = 0;
  const auto& obj = json["obj"];
  for (auto it = obj.begin(); it != obj.end(); ++it) {
    sum += it->asInt64();
    sum += static_cast<std::int64_t>(it.name().size());
  }
  return sum;
}

std::pair<std::vector<std::int64_t>, std::int64_t> SC_BENCH_NATIVE_ARRAY_SCAN(
    const SC_BENCH_TYPE& json) {
  const auto& list = json["list"];
  std::vector<std::int64_t> copied;
  copied.reserve(list.size());
  std::int64_t sum = 0;
  for (auto it = list.begin(); it != list.end(); ++it) {
    copied.push_back(it->asInt64());
    sum += it->asInt64();
  }
  return {std::move(copied), sum};
}
