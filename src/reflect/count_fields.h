//
// Created by jiangyuyun3 on 2026/10/14.
//

#ifndef SCCOMMON_PFR_H
#define SCCOMMON_PFR_H

#include "def.h"

namespace sc::reflect {

struct Any {
  template <typename T>
  constexpr operator T() const noexcept; //NOLINT
};

namespace detail {

template <typename, typename, typename = void>
struct IsBracesConstructible : std::false_type {};

template <typename T, size_t... I>
struct IsBracesConstructible<
    T, std::index_sequence<I...>,
    std::void_t<decltype(T{(static_cast<void>(I), Any{})...})>>
    : std::true_type {};

template <typename T, std::size_t Lo = 0, std::size_t Hi = kMaxFields>
constexpr std::size_t CountFieldsImpl() {
  if constexpr (Lo == Hi) {
    return Lo;
  } else {
    if constexpr (constexpr auto mid = Lo + (Hi - Lo + 1) / 2;
                  IsBracesConstructible<T,
                                        std::make_index_sequence<mid>>::value) {
      static_assert(mid != kMaxFields, "Too many fields");
      return CountFieldsImpl<T, mid, Hi>();
    } else {
      return CountFieldsImpl<T, Lo, mid - 1>();
    }
  }
}

}  // namespace detail

template <typename T>
constexpr std::size_t FieldCount() {
  return detail::CountFieldsImpl<T>();
}

}  // namespace sc::reflect

#endif  // SCCOMMON_PFR_H
