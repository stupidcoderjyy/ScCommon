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

// 从成员指针取出成员类型
template <typename M>
struct MemberType;
template <typename C, typename M>
struct MemberType<M C::*> {
  using Type = M;
};
template <typename M>
using member_type_t = typename MemberType<M>::Type;

// 序列容器：有 value_type 且能 push_back（排除 string 这类“值类型”）
template <typename T, typename = void>
struct is_sequence : std::false_type {};
template <typename T>
struct is_sequence<T, std::void_t<typename T::value_type,
                                  decltype(std::declval<T&>().push_back(
                                      std::declval<typename T::value_type>()))>>
    : std::true_type {};
template <typename T>
inline constexpr bool is_sequence_v =
    !std::is_same_v<std::remove_cv_t<T>, std::string> &&
    !std::is_same_v<std::remove_cv_t<T>, std::wstring> && is_sequence<T>::value;

// 映射容器：有 key_type 与 mapped_type
template <typename T, typename = void>
struct is_map : std::false_type {};
template <typename T>
struct is_map<T, std::void_t<typename T::key_type, typename T::mapped_type>>
    : std::true_type {};
template <typename T>
inline constexpr bool is_map_v = is_map<T>::value;

}  // namespace detail

template <typename Json, typename T>
struct Extractor {
  using ValueType = T;
  static bool Extract(const Json& json, T& val) {
    if constexpr (std::is_same_v<T, int>) {
      return JsonTraits<Json>::GetInt(json, val);
    } else if constexpr (std::is_same_v<T, unsigned int>) {
      return JsonTraits<Json>::GetUint(json, val);
    } else if constexpr (std::is_same_v<T, std::int64_t>) {
      return JsonTraits<Json>::GetInt64(json, val);
    } else if constexpr (std::is_same_v<T, std::uint64_t>) {
      return JsonTraits<Json>::GetUint64(json, val);
    } else if constexpr (std::is_same_v<T, double>) {
      return JsonTraits<Json>::GetDouble(json, val);
    } else if constexpr (std::is_same_v<T, std::string>) {
      return JsonTraits<Json>::GetString(json, val);
    } else if constexpr (std::is_same_v<T, bool>) {
      return JsonTraits<Json>::GetBool(json, val);
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
  template <JsonErrorHandler E>
  static bool Inject(JsonOwner<Json, E>& owner, const Json& parent,
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

}  // namespace sc::json

#endif  // SCCOMMON_JSON_READ_WRITE_H
