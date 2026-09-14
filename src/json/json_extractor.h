//
// Created by jiangyuyun3 on 2026/9/14.
//

#ifndef SCCOMMON_JSON_EXTRACTOR_H
#define SCCOMMON_JSON_EXTRACTOR_H
#include <cstddef>
#include <cstdint>
#include <string>
#include <type_traits>

#include "json_traits.h"

namespace sc::json {

namespace detail {

// 恒假但依赖模板参数：只有 Extractor 真正实例化到该类型时才会触发断言
template <typename>
inline constexpr bool unsupported_extractor_type = false;

}  // namespace detail

template <typename Json, typename T>
struct Extractor {
  using ValueType = T;
  static bool Extract(const Json& json, T& val) {
    if constexpr (std::is_same_v<T, int>) {
      if (!JsonTraits<Json>::IsInt(json)) return false;
      val = JsonTraits<Json>::GetInt(json);
    } else if constexpr (std::is_same_v<T, unsigned int>) {
      if (!JsonTraits<Json>::IsUint(json)) return false;
      val = JsonTraits<Json>::GetUint(json);
    } else if constexpr (std::is_same_v<T, std::int64_t>) {
      if (!JsonTraits<Json>::IsInt64(json)) return false;
      val = JsonTraits<Json>::GetInt64(json);
    } else if constexpr (std::is_same_v<T, std::uint64_t>) {
      if (!JsonTraits<Json>::IsUint64(json)) return false;
      val = JsonTraits<Json>::GetUint64(json);
    } else if constexpr (std::is_same_v<T, double>) {
      if (!JsonTraits<Json>::IsDouble(json)) return false;
      val = JsonTraits<Json>::GetDouble(json);
    } else if constexpr (std::is_same_v<T, std::string>) {
      if (!JsonTraits<Json>::IsString(json)) return false;
      val = JsonTraits<Json>::GetString(json);
    } else if constexpr (std::is_same_v<T, bool>) {
      if (!JsonTraits<Json>::IsBool(json)) return false;
      val = JsonTraits<Json>::GetBool(json);
    } else {
      static_assert(detail::unsupported_extractor_type<T>,
                    "JsonExtractor: unsupported type; specialize "
                    "sc::json::Extractor<Json, T> (or use another extractor)");
    }
    return true;
  }
};

template <typename Json, typename T>
using JsonElement = Extractor<Json, T>;

template <typename Json>
using JsonInt = JsonElement<Json, int>;

template <typename Json>
using JsonInt64 = JsonElement<Json, std::int64_t>;

template <typename Json>
using JsonDouble = JsonElement<Json, double>;

template <typename Json>
using JsonString = JsonElement<Json, std::string>;

template <typename Json>
using JsonBool = JsonElement<Json, bool>;

}  // namespace sc::json

#endif  // SCCOMMON_JSON_EXTRACTOR_H
