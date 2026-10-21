//
// Created by jiangyuyun3 on 2026/10/20.
//

#ifndef SCCOMMON_FIELD_ACCESS_H
#define SCCOMMON_FIELD_ACCESS_H

#include <cstddef>
#include <memory>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <utility>

#include "field_counter.h"
#include "field_index_access.h"
#include "field_name_access.h"

namespace sc::reflect {

template <typename T, typename F>
struct FieldInfo {
  std::string_view name;
  F& (*get)(T&);
  const F& (*get_const)(const T&);

  F& Get(T& value) const { return get(value); }
  const F& Get(const T& value) const { return get_const(value); }
};

namespace detail {

template <typename T, bool = std::is_aggregate_v<T> &&
                             !std::is_reference_v<T> && !std::is_union_v<T>>
struct IsReflectable : std::false_type {};

template <typename T>
struct IsReflectable<T, true> : std::bool_constant<(FieldCount<T>() > 0)> {};

}  // namespace detail

template <typename T>
inline constexpr bool reflectable_v = detail::IsReflectable<T>::value;

template <std::size_t I, typename T>
consteval std::string_view FieldName() noexcept {
  return detail::ParseFieldNameChecked<
      T, std::addressof(detail::GetFieldAt<I, T>(detail::FakeObject<T>()))>();
}

namespace detail {

template <typename T, std::size_t I>
consteval auto MakeFieldInfo() {
  using F = FieldType<I, T>;
  return FieldInfo<T, F>{
      FieldName<I, T>(), static_cast<F& (*)(T&)>(&GetFieldAt<I, T>),
      static_cast<const F& (*)(const T&)>(&GetFieldAt<I, T>)};
}

template <typename T, std::size_t... I>
consteval auto MakeFieldInfos(std::index_sequence<I...>) {
  return std::tuple{MakeFieldInfo<T, I>()...};
}

}  // namespace detail

template <typename T>
struct Descriptor {
  static constexpr auto fields() {
    static_assert(reflectable_v<T>,
                  "reflect: only non-empty aggregate types are reflectable "
                  "(no base classes, private members, bit-fields or unions)");
    return detail::MakeFieldInfos<T>(
        std::make_index_sequence<FieldCount<T>()>{});
  }
};

template <common::FixedString S, typename T, std::size_t... I>
consteval std::size_t FindFieldIndex(std::index_sequence<I...>) {
  constexpr auto fields = Descriptor<T>::fields();
  auto i = sizeof...(I);
  ((std::get<I>(fields).name == S.view() ? (i = I, true) : false), ...);
  if (i != sizeof...(I)) {
    return i;
  }
  constexpr auto alias = detail::AliasFieldName<S, T>();
  ((std::get<I>(fields).name == alias ? (i = I, true) : false), ...);
  return i;
}

template <common::FixedString S, typename T>
consteval auto GetFieldInfo() {
  static_assert(reflectable_v<T>,
                "reflect: only non-empty aggregate types are reflectable "
                "(no base classes, private members, bit-fields or unions)");
  constexpr auto n = FieldCount<T>();
  constexpr auto i = FindFieldIndex<S, T>(std::make_index_sequence<n>{});
  static_assert(i < n, "reflect: no field with the given name");
  return std::get<i>(Descriptor<T>::fields());
}

template <std::size_t I, typename T>
consteval auto GetFieldInfo() {
  static_assert(reflectable_v<T>,
                "reflect: only non-empty aggregate types are reflectable "
                "(no base classes, private members, bit-fields or unions)");
  static_assert(I < FieldCount<T>(), "reflect: index out of range");
  return std::get<I>(Descriptor<T>::fields());
}

template <std::size_t I, typename T>
consteval std::string_view GetFieldName() {
  return GetFieldInfo<I, T>().name;
}

template <common::FixedString S, typename T>
consteval std::string_view GetFieldName() {
  return GetFieldInfo<S, T>().name;
}

template <std::size_t I, typename T>
constexpr auto& GetField(T& value) noexcept {
  return detail::GetFieldAt<I, T>(value);
}

template <std::size_t I, typename T>
constexpr const auto& GetField(const T& value) noexcept {
  return detail::GetFieldAt<I, T>(value);
}

template <common::FixedString S, typename T>
constexpr auto& GetField(T& value) noexcept {
  return GetFieldInfo<S, T>().Get(value);
}

template <common::FixedString S, typename T>
constexpr const auto& GetField(const T& value) noexcept {
  return GetFieldInfo<S, T>().Get(value);
}

}  // namespace sc::reflect

#endif  // SCCOMMON_FIELD_ACCESS_H
