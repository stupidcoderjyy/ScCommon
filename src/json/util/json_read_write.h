//
// Created by jiangyuyun3 on 2026/9/14.
//

#ifndef SCCOMMON_JSON_READ_WRITE_H
#define SCCOMMON_JSON_READ_WRITE_H
#include <cstdint>
#include <optional>
#include <string>
#include <type_traits>

#include "../json_traits.h"

namespace sc::json {
namespace detail {

// 恒假但依赖模板参数：只有 Extractor 真正实例化到该类型时才会触发断言
template <typename>
inline constexpr bool unsupported_extractor_type = false;

// std::optional 判定：不用 std::__is_optional_v 这类实现私有的名字
template <typename>
struct is_optional : std::false_type {};
template <typename T>
struct is_optional<std::optional<T>> : std::true_type {};
template <typename T>
inline constexpr bool is_optional_v = is_optional<std::remove_cv_t<T>>::value;

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
    } else if constexpr (detail::is_optional_v<T>) {
      if (JsonTraits<Json>::IsNull(json)) {
        val.reset();
        return true;
      }
      using ValType = T::value_type;
      static_assert(!detail::is_optional_v<ValType>,
                    "JsonExtractor: nested std::optional is not supported");
      ValType inner_val{};
      if (!Extractor<Json, ValType>::Extract(json, inner_val)) {
        return false;
      }
      val = std::move(inner_val);
      return true;
    } else {
      static_assert(detail::unsupported_extractor_type<T>,
                    "JsonExtractor: unsupported type; specialize "
                    "sc::json::Extractor<Json, T> (or use another extractor)");
    }
    return true;
  }
};

template <typename Json, typename T>
struct Injector {
  template <JsonErrorHandler ErrorHandler>
  static bool Inject(JsonOwner<Json, ErrorHandler>& owner, const Json& parent,
                     std::string_view name, const T& val) {
    if constexpr (std::is_same_v<T, int>) {
      return JsonTraits<Json>::AddInt(owner, parent, name, val);
    } else if constexpr (std::is_same_v<T, unsigned int>) {
      return JsonTraits<Json>::AddUint(owner, parent, name, val);
    } else if constexpr (std::is_same_v<T, std::int64_t>) {
      return JsonTraits<Json>::AddInt64(owner, parent, name, val);
    } else if constexpr (std::is_same_v<T, std::uint64_t>) {
      return JsonTraits<Json>::AddUint64(owner, parent, name, val);
    } else if constexpr (std::is_same_v<T, double>) {
      return JsonTraits<Json>::AddDouble(owner, parent, name, val);
    } else if constexpr (std::is_same_v<T, std::string>) {
      return JsonTraits<Json>::AddString(owner, parent, name, val);
    } else if constexpr (std::is_same_v<T, bool>) {
      return JsonTraits<Json>::AddBool(owner, parent, name, val);
    } else if constexpr (std::is_same_v<T, std::nullptr_t>) {
      return JsonTraits<Json>::AddNull(owner, parent, name);
    } else if constexpr (detail::is_optional_v<T>) {
      if (!val.has_value()) {
        return JsonTraits<Json>::AddNull(owner, parent, name);
      }
      using ValType = T::value_type;
      static_assert(!detail::is_optional_v<ValType>,
                    "JsonInjector: nested std::optional is not supported");
      return Injector<Json, ValType>::Inject(owner, parent, name, *val);
    } else {
      static_assert(detail::unsupported_extractor_type<T>,
                    "JsonInjector: unsupported type; specialize "
                    "sc::json::Injector<Json, T>");
    }
    return false;
  }
};

} // namespace sc::json

#endif  // SCCOMMON_JSON_READ_WRITE_H
