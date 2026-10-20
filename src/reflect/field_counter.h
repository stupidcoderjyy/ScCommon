//
// Created by jiangyuyun3 on 2026/10/20.
//

#ifndef SCCOMMON_FIELD_COUNTER_H
#define SCCOMMON_FIELD_COUNTER_H

#include <cstddef>
#include <type_traits>
#include <utility>

#include "def.h"

namespace sc::reflect {

namespace detail {

struct Any {
  template <typename T>
  constexpr operator T() const noexcept;  // NOLINT
};

template <typename, typename, typename = void>
struct IsBracesConstructible : std::false_type {};

template <typename T, std::size_t... I>
struct IsBracesConstructible<
    T, std::index_sequence<I...>,
    std::void_t<decltype(T{(static_cast<void>(I), Any{})...})>>
    : std::true_type {};

template <typename T, std::size_t Lo = 0, std::size_t Hi = kMaxFields>
constexpr std::size_t CountFieldsImpl() {
  if constexpr (Lo == Hi) {
    return Lo;
  } else {
    if constexpr (constexpr std::size_t mid = Lo + (Hi - Lo + 1) / 2;
                  IsBracesConstructible<T,
                                        std::make_index_sequence<mid>>::value) {
      static_assert(mid != kMaxFields, "reflect: too many fields");
      return CountFieldsImpl<T, mid, Hi>();
    } else {
      return CountFieldsImpl<T, Lo, mid - 1>();
    }
  }
}

}  // namespace detail

template <typename T>
constexpr std::size_t FieldCount() {
  static_assert(!std::is_reference_v<T>,
                "reflect: FieldCount requires an object type, not a reference");
  return detail::CountFieldsImpl<T>();
}

}  // namespace sc::reflect

#endif  // SCCOMMON_FIELD_COUNTER_H
