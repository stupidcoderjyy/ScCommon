//
// Created by jiangyuyun3 on 2026/10/14.
//

#ifndef SCCOMMON_DESCRIPTOR_H
#define SCCOMMON_DESCRIPTOR_H

#include "count_fields.h"

namespace sc::reflect {

template <typename T, typename F>
struct FieldInfo {
  std::string_view name;  // 成员名称
  F T::* member;          // 成员指针
};

template <typename T>
struct Descriptor;  // 未特化

template <typename /*T*/, typename = void>
struct IsRegistered : std::false_type {};

// 如果 Descriptor<T>::fields() 替换失败，则退回上面那个
template <typename T>
struct IsRegistered<T, std::void_t<decltype(Descriptor<T>::fields())>>
    : std::true_type {};

template <typename T>
inline constexpr bool is_registered_v = IsRegistered<T>::value;

// 已注册类型的字段数
template <typename T>
constexpr std::size_t RegisteredFields() {
  static_assert(is_registered_v<T>, "reflect: type is not registered");
  return std::tuple_size_v<decltype(Descriptor<T>::fields())>;
}

namespace detail {

#define REFLECT_DETAIL_GET_CASE(N, ...)        \
  else if constexpr (n == N) {                 \
    auto& [__VA_ARGS__] = value;               \
    return std::get<I>(std::tie(__VA_ARGS__)); \
  }

template <std::size_t I, typename T>
constexpr auto& GetFieldImpl(T& value) noexcept {
  using U = std::remove_cv_t<T>;
  constexpr auto n = FieldCount<U>();
  static_assert(I < n, "Index out of range");
  static_assert(n > 0, "No fields");
  if constexpr (n == 1) {
    auto& [a] = value;
    return std::get<I>(std::tie(a));
  }
  // clang-format off
  REFLECT_DETAIL_GET_CASE(2, a1, a2)
  REFLECT_DETAIL_GET_CASE(3, a1, a2, a3)
  REFLECT_DETAIL_GET_CASE(4, a1, a2, a3, a4)
  REFLECT_DETAIL_GET_CASE(5, a1, a2, a3, a4, a5)
  REFLECT_DETAIL_GET_CASE(6, a1, a2, a3, a4, a5, a6)
  REFLECT_DETAIL_GET_CASE(7, a1, a2, a3, a4, a5, a6, a7)
  REFLECT_DETAIL_GET_CASE(8, a1, a2, a3, a4, a5, a6, a7, a8)
  REFLECT_DETAIL_GET_CASE(9, a1, a2, a3, a4, a5, a6, a7, a8, a9)
  REFLECT_DETAIL_GET_CASE(10, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10)
  REFLECT_DETAIL_GET_CASE(11, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11)
  REFLECT_DETAIL_GET_CASE(12, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12)
  REFLECT_DETAIL_GET_CASE(13, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13)
  REFLECT_DETAIL_GET_CASE(14, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14)
  REFLECT_DETAIL_GET_CASE(15, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15)
  REFLECT_DETAIL_GET_CASE(16, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16)
  REFLECT_DETAIL_GET_CASE(17, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17)
  REFLECT_DETAIL_GET_CASE(18, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18)
  REFLECT_DETAIL_GET_CASE(19, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19)
  REFLECT_DETAIL_GET_CASE(20, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20)
  REFLECT_DETAIL_GET_CASE(21, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21)
  REFLECT_DETAIL_GET_CASE(22, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22)
  REFLECT_DETAIL_GET_CASE(23, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23)
  REFLECT_DETAIL_GET_CASE(24, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24)
  REFLECT_DETAIL_GET_CASE(25, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25)
  REFLECT_DETAIL_GET_CASE(26, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26)
  REFLECT_DETAIL_GET_CASE(27, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27)
  REFLECT_DETAIL_GET_CASE(28, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28)
  REFLECT_DETAIL_GET_CASE(29, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28, a29)
  REFLECT_DETAIL_GET_CASE(30, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28, a29, a30)
  REFLECT_DETAIL_GET_CASE(31, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28, a29, a30, a31)
  REFLECT_DETAIL_GET_CASE(32, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28, a29, a30, a31, a32)
  REFLECT_DETAIL_GET_CASE(33, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28, a29, a30, a31, a32, a33)
  REFLECT_DETAIL_GET_CASE(34, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28, a29, a30, a31, a32, a33, a34)
  REFLECT_DETAIL_GET_CASE(35, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28, a29, a30, a31, a32, a33, a34, a35)
  REFLECT_DETAIL_GET_CASE(36, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28, a29, a30, a31, a32, a33, a34, a35, a36)
  REFLECT_DETAIL_GET_CASE(37, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28, a29, a30, a31, a32, a33, a34, a35, a36, a37)
  REFLECT_DETAIL_GET_CASE(38, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28, a29, a30, a31, a32, a33, a34, a35, a36, a37, a38)
  REFLECT_DETAIL_GET_CASE(39, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28, a29, a30, a31, a32, a33, a34, a35, a36, a37, a38, a39)
  REFLECT_DETAIL_GET_CASE(40, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28, a29, a30, a31, a32, a33, a34, a35, a36, a37, a38, a39, a40)
  REFLECT_DETAIL_GET_CASE(41, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28, a29, a30, a31, a32, a33, a34, a35, a36, a37, a38, a39, a40, a41)
  REFLECT_DETAIL_GET_CASE(42, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28, a29, a30, a31, a32, a33, a34, a35, a36, a37, a38, a39, a40, a41, a42)
  REFLECT_DETAIL_GET_CASE(43, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28, a29, a30, a31, a32, a33, a34, a35, a36, a37, a38, a39, a40, a41, a42, a43)
  REFLECT_DETAIL_GET_CASE(44, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28, a29, a30, a31, a32, a33, a34, a35, a36, a37, a38, a39, a40, a41, a42, a43, a44)
  REFLECT_DETAIL_GET_CASE(45, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28, a29, a30, a31, a32, a33, a34, a35, a36, a37, a38, a39, a40, a41, a42, a43, a44, a45)
  REFLECT_DETAIL_GET_CASE(46, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28, a29, a30, a31, a32, a33, a34, a35, a36, a37, a38, a39, a40, a41, a42, a43, a44, a45, a46)
  REFLECT_DETAIL_GET_CASE(47, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28, a29, a30, a31, a32, a33, a34, a35, a36, a37, a38, a39, a40, a41, a42, a43, a44, a45, a46, a47)
  REFLECT_DETAIL_GET_CASE(48, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28, a29, a30, a31, a32, a33, a34, a35, a36, a37, a38, a39, a40, a41, a42, a43, a44, a45, a46, a47, a48)
  REFLECT_DETAIL_GET_CASE(49, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28, a29, a30, a31, a32, a33, a34, a35, a36, a37, a38, a39, a40, a41, a42, a43, a44, a45, a46, a47, a48, a49)
  REFLECT_DETAIL_GET_CASE(50, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28, a29, a30, a31, a32, a33, a34, a35, a36, a37, a38, a39, a40, a41, a42, a43, a44, a45, a46, a47, a48, a49, a50)
  REFLECT_DETAIL_GET_CASE(51, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28, a29, a30, a31, a32, a33, a34, a35, a36, a37, a38, a39, a40, a41, a42, a43, a44, a45, a46, a47, a48, a49, a50, a51)
  REFLECT_DETAIL_GET_CASE(52, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28, a29, a30, a31, a32, a33, a34, a35, a36, a37, a38, a39, a40, a41, a42, a43, a44, a45, a46, a47, a48, a49, a50, a51, a52)
  REFLECT_DETAIL_GET_CASE(53, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28, a29, a30, a31, a32, a33, a34, a35, a36, a37, a38, a39, a40, a41, a42, a43, a44, a45, a46, a47, a48, a49, a50, a51, a52, a53)
  REFLECT_DETAIL_GET_CASE(54, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28, a29, a30, a31, a32, a33, a34, a35, a36, a37, a38, a39, a40, a41, a42, a43, a44, a45, a46, a47, a48, a49, a50, a51, a52, a53, a54)
  REFLECT_DETAIL_GET_CASE(55, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28, a29, a30, a31, a32, a33, a34, a35, a36, a37, a38, a39, a40, a41, a42, a43, a44, a45, a46, a47, a48, a49, a50, a51, a52, a53, a54, a55)
  REFLECT_DETAIL_GET_CASE(56, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28, a29, a30, a31, a32, a33, a34, a35, a36, a37, a38, a39, a40, a41, a42, a43, a44, a45, a46, a47, a48, a49, a50, a51, a52, a53, a54, a55, a56)
  REFLECT_DETAIL_GET_CASE(57, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28, a29, a30, a31, a32, a33, a34, a35, a36, a37, a38, a39, a40, a41, a42, a43, a44, a45, a46, a47, a48, a49, a50, a51, a52, a53, a54, a55, a56, a57)
  REFLECT_DETAIL_GET_CASE(58, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28, a29, a30, a31, a32, a33, a34, a35, a36, a37, a38, a39, a40, a41, a42, a43, a44, a45, a46, a47, a48, a49, a50, a51, a52, a53, a54, a55, a56, a57, a58)
  REFLECT_DETAIL_GET_CASE(59, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28, a29, a30, a31, a32, a33, a34, a35, a36, a37, a38, a39, a40, a41, a42, a43, a44, a45, a46, a47, a48, a49, a50, a51, a52, a53, a54, a55, a56, a57, a58, a59)
  REFLECT_DETAIL_GET_CASE(60, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28, a29, a30, a31, a32, a33, a34, a35, a36, a37, a38, a39, a40, a41, a42, a43, a44, a45, a46, a47, a48, a49, a50, a51, a52, a53, a54, a55, a56, a57, a58, a59, a60)
  REFLECT_DETAIL_GET_CASE(61, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28, a29, a30, a31, a32, a33, a34, a35, a36, a37, a38, a39, a40, a41, a42, a43, a44, a45, a46, a47, a48, a49, a50, a51, a52, a53, a54, a55, a56, a57, a58, a59, a60, a61)
  REFLECT_DETAIL_GET_CASE(62, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28, a29, a30, a31, a32, a33, a34, a35, a36, a37, a38, a39, a40, a41, a42, a43, a44, a45, a46, a47, a48, a49, a50, a51, a52, a53, a54, a55, a56, a57, a58, a59, a60, a61, a62)
  REFLECT_DETAIL_GET_CASE(63, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28, a29, a30, a31, a32, a33, a34, a35, a36, a37, a38, a39, a40, a41, a42, a43, a44, a45, a46, a47, a48, a49, a50, a51, a52, a53, a54, a55, a56, a57, a58, a59, a60, a61, a62, a63)
  REFLECT_DETAIL_GET_CASE(64, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28, a29, a30, a31, a32, a33, a34, a35, a36, a37, a38, a39, a40, a41, a42, a43, a44, a45, a46, a47, a48, a49, a50, a51, a52, a53, a54, a55, a56, a57, a58, a59, a60, a61, a62, a63, a64)
  // clang-format on
}

#undef REFLECT_DETAIL_GET_CASE

}  // namespace detail

template <std::size_t I, typename T>
constexpr auto GetFieldInfo() {
  static_assert(I < RegisteredFields<T>(), "reflect: index out of range");
  return std::get<I>(Descriptor<T>::fields());
}

template <std::size_t I, typename T>
constexpr std::string_view GetFieldName() {
  return GetFieldInfo<I, T>().name;
}

template <std::size_t I, typename T>
constexpr auto& GetField(T& value) noexcept {
  if constexpr (is_registered_v<T>) {
    return value.*GetFieldInfo<I, T>().member;
  } else {
    return detail::GetFieldImpl<I>(value);
  }
}

template <std::size_t I, typename T>
constexpr const auto& GetField(const T& value) noexcept {
  if constexpr (is_registered_v<T>) {
    return value.*GetFieldInfo<I, T>().member;
  } else {
    return detail::GetFieldImpl<I>(value);
  }
}

}  // namespace sc::reflect

// clang-format off
#define REFLECT_FIELD(Type, Field) \
  ::sc::reflect::FieldInfo<Type, decltype(Type::Field)>(#Field, &Type::Field)

// ---------------- 快速注册宏 ----------------
#define SC_REFLECT_FIELDS_1(Type, a) REFLECT_FIELD(Type, a)
#define SC_REFLECT_FIELDS_2(Type, a, ...) REFLECT_FIELD(Type, a), SC_REFLECT_FIELDS_1(Type, __VA_ARGS__)
#define SC_REFLECT_FIELDS_3(Type, a, ...) REFLECT_FIELD(Type, a), SC_REFLECT_FIELDS_2(Type, __VA_ARGS__)
#define SC_REFLECT_FIELDS_4(Type, a, ...) REFLECT_FIELD(Type, a), SC_REFLECT_FIELDS_3(Type, __VA_ARGS__)
#define SC_REFLECT_FIELDS_5(Type, a, ...) REFLECT_FIELD(Type, a), SC_REFLECT_FIELDS_4(Type, __VA_ARGS__)
#define SC_REFLECT_FIELDS_6(Type, a, ...) REFLECT_FIELD(Type, a), SC_REFLECT_FIELDS_5(Type, __VA_ARGS__)
#define SC_REFLECT_FIELDS_7(Type, a, ...) REFLECT_FIELD(Type, a), SC_REFLECT_FIELDS_6(Type, __VA_ARGS__)
#define SC_REFLECT_FIELDS_8(Type, a, ...) REFLECT_FIELD(Type, a), SC_REFLECT_FIELDS_7(Type, __VA_ARGS__)
#define SC_REFLECT_FIELDS_9(Type, a, ...) REFLECT_FIELD(Type, a), SC_REFLECT_FIELDS_8(Type, __VA_ARGS__)
#define SC_REFLECT_FIELDS_10(Type, a, ...) REFLECT_FIELD(Type, a), SC_REFLECT_FIELDS_9(Type, __VA_ARGS__)
#define SC_REFLECT_FIELDS_11(Type, a, ...) REFLECT_FIELD(Type, a), SC_REFLECT_FIELDS_10(Type, __VA_ARGS__)
#define SC_REFLECT_FIELDS_12(Type, a, ...) REFLECT_FIELD(Type, a), SC_REFLECT_FIELDS_11(Type, __VA_ARGS__)
#define SC_REFLECT_FIELDS_13(Type, a, ...) REFLECT_FIELD(Type, a), SC_REFLECT_FIELDS_12(Type, __VA_ARGS__)
#define SC_REFLECT_FIELDS_14(Type, a, ...) REFLECT_FIELD(Type, a), SC_REFLECT_FIELDS_13(Type, __VA_ARGS__)
#define SC_REFLECT_FIELDS_15(Type, a, ...) REFLECT_FIELD(Type, a), SC_REFLECT_FIELDS_14(Type, __VA_ARGS__)
#define SC_REFLECT_FIELDS_16(Type, a, ...) REFLECT_FIELD(Type, a), SC_REFLECT_FIELDS_15(Type, __VA_ARGS__)
#define SC_REFLECT_FIELDS_17(Type, a, ...) REFLECT_FIELD(Type, a), SC_REFLECT_FIELDS_16(Type, __VA_ARGS__)
#define SC_REFLECT_FIELDS_18(Type, a, ...) REFLECT_FIELD(Type, a), SC_REFLECT_FIELDS_17(Type, __VA_ARGS__)
#define SC_REFLECT_FIELDS_19(Type, a, ...) REFLECT_FIELD(Type, a), SC_REFLECT_FIELDS_18(Type, __VA_ARGS__)
#define SC_REFLECT_FIELDS_20(Type, a, ...) REFLECT_FIELD(Type, a), SC_REFLECT_FIELDS_19(Type, __VA_ARGS__)
#define SC_REFLECT_FIELDS_21(Type, a, ...) REFLECT_FIELD(Type, a), SC_REFLECT_FIELDS_20(Type, __VA_ARGS__)
#define SC_REFLECT_FIELDS_22(Type, a, ...) REFLECT_FIELD(Type, a), SC_REFLECT_FIELDS_21(Type, __VA_ARGS__)
#define SC_REFLECT_FIELDS_23(Type, a, ...) REFLECT_FIELD(Type, a), SC_REFLECT_FIELDS_22(Type, __VA_ARGS__)
#define SC_REFLECT_FIELDS_24(Type, a, ...) REFLECT_FIELD(Type, a), SC_REFLECT_FIELDS_23(Type, __VA_ARGS__)
#define SC_REFLECT_FIELDS_25(Type, a, ...) REFLECT_FIELD(Type, a), SC_REFLECT_FIELDS_24(Type, __VA_ARGS__)
#define SC_REFLECT_FIELDS_26(Type, a, ...) REFLECT_FIELD(Type, a), SC_REFLECT_FIELDS_25(Type, __VA_ARGS__)
#define SC_REFLECT_FIELDS_27(Type, a, ...) REFLECT_FIELD(Type, a), SC_REFLECT_FIELDS_26(Type, __VA_ARGS__)
#define SC_REFLECT_FIELDS_28(Type, a, ...) REFLECT_FIELD(Type, a), SC_REFLECT_FIELDS_27(Type, __VA_ARGS__)
#define SC_REFLECT_FIELDS_29(Type, a, ...) REFLECT_FIELD(Type, a), SC_REFLECT_FIELDS_28(Type, __VA_ARGS__)
#define SC_REFLECT_FIELDS_30(Type, a, ...) REFLECT_FIELD(Type, a), SC_REFLECT_FIELDS_29(Type, __VA_ARGS__)
#define SC_REFLECT_FIELDS_31(Type, a, ...) REFLECT_FIELD(Type, a), SC_REFLECT_FIELDS_30(Type, __VA_ARGS__)
#define SC_REFLECT_FIELDS_32(Type, a, ...) REFLECT_FIELD(Type, a), SC_REFLECT_FIELDS_31(Type, __VA_ARGS__)
#define SC_REFLECT_FIELDS_33(Type, a, ...) REFLECT_FIELD(Type, a), SC_REFLECT_FIELDS_32(Type, __VA_ARGS__)
#define SC_REFLECT_FIELDS_34(Type, a, ...) REFLECT_FIELD(Type, a), SC_REFLECT_FIELDS_33(Type, __VA_ARGS__)
#define SC_REFLECT_FIELDS_35(Type, a, ...) REFLECT_FIELD(Type, a), SC_REFLECT_FIELDS_34(Type, __VA_ARGS__)
#define SC_REFLECT_FIELDS_36(Type, a, ...) REFLECT_FIELD(Type, a), SC_REFLECT_FIELDS_35(Type, __VA_ARGS__)
#define SC_REFLECT_FIELDS_37(Type, a, ...) REFLECT_FIELD(Type, a), SC_REFLECT_FIELDS_36(Type, __VA_ARGS__)
#define SC_REFLECT_FIELDS_38(Type, a, ...) REFLECT_FIELD(Type, a), SC_REFLECT_FIELDS_37(Type, __VA_ARGS__)
#define SC_REFLECT_FIELDS_39(Type, a, ...) REFLECT_FIELD(Type, a), SC_REFLECT_FIELDS_38(Type, __VA_ARGS__)
#define SC_REFLECT_FIELDS_40(Type, a, ...) REFLECT_FIELD(Type, a), SC_REFLECT_FIELDS_39(Type, __VA_ARGS__)
#define SC_REFLECT_FIELDS_41(Type, a, ...) REFLECT_FIELD(Type, a), SC_REFLECT_FIELDS_40(Type, __VA_ARGS__)
#define SC_REFLECT_FIELDS_42(Type, a, ...) REFLECT_FIELD(Type, a), SC_REFLECT_FIELDS_41(Type, __VA_ARGS__)
#define SC_REFLECT_FIELDS_43(Type, a, ...) REFLECT_FIELD(Type, a), SC_REFLECT_FIELDS_42(Type, __VA_ARGS__)
#define SC_REFLECT_FIELDS_44(Type, a, ...) REFLECT_FIELD(Type, a), SC_REFLECT_FIELDS_43(Type, __VA_ARGS__)
#define SC_REFLECT_FIELDS_45(Type, a, ...) REFLECT_FIELD(Type, a), SC_REFLECT_FIELDS_44(Type, __VA_ARGS__)
#define SC_REFLECT_FIELDS_46(Type, a, ...) REFLECT_FIELD(Type, a), SC_REFLECT_FIELDS_45(Type, __VA_ARGS__)
#define SC_REFLECT_FIELDS_47(Type, a, ...) REFLECT_FIELD(Type, a), SC_REFLECT_FIELDS_46(Type, __VA_ARGS__)
#define SC_REFLECT_FIELDS_48(Type, a, ...) REFLECT_FIELD(Type, a), SC_REFLECT_FIELDS_47(Type, __VA_ARGS__)
#define SC_REFLECT_FIELDS_49(Type, a, ...) REFLECT_FIELD(Type, a), SC_REFLECT_FIELDS_48(Type, __VA_ARGS__)
#define SC_REFLECT_FIELDS_50(Type, a, ...) REFLECT_FIELD(Type, a), SC_REFLECT_FIELDS_49(Type, __VA_ARGS__)
#define SC_REFLECT_FIELDS_51(Type, a, ...) REFLECT_FIELD(Type, a), SC_REFLECT_FIELDS_50(Type, __VA_ARGS__)
#define SC_REFLECT_FIELDS_52(Type, a, ...) REFLECT_FIELD(Type, a), SC_REFLECT_FIELDS_51(Type, __VA_ARGS__)
#define SC_REFLECT_FIELDS_53(Type, a, ...) REFLECT_FIELD(Type, a), SC_REFLECT_FIELDS_52(Type, __VA_ARGS__)
#define SC_REFLECT_FIELDS_54(Type, a, ...) REFLECT_FIELD(Type, a), SC_REFLECT_FIELDS_53(Type, __VA_ARGS__)
#define SC_REFLECT_FIELDS_55(Type, a, ...) REFLECT_FIELD(Type, a), SC_REFLECT_FIELDS_54(Type, __VA_ARGS__)
#define SC_REFLECT_FIELDS_56(Type, a, ...) REFLECT_FIELD(Type, a), SC_REFLECT_FIELDS_55(Type, __VA_ARGS__)
#define SC_REFLECT_FIELDS_57(Type, a, ...) REFLECT_FIELD(Type, a), SC_REFLECT_FIELDS_56(Type, __VA_ARGS__)
#define SC_REFLECT_FIELDS_58(Type, a, ...) REFLECT_FIELD(Type, a), SC_REFLECT_FIELDS_57(Type, __VA_ARGS__)
#define SC_REFLECT_FIELDS_59(Type, a, ...) REFLECT_FIELD(Type, a), SC_REFLECT_FIELDS_58(Type, __VA_ARGS__)
#define SC_REFLECT_FIELDS_60(Type, a, ...) REFLECT_FIELD(Type, a), SC_REFLECT_FIELDS_59(Type, __VA_ARGS__)
#define SC_REFLECT_FIELDS_61(Type, a, ...) REFLECT_FIELD(Type, a), SC_REFLECT_FIELDS_60(Type, __VA_ARGS__)
#define SC_REFLECT_FIELDS_62(Type, a, ...) REFLECT_FIELD(Type, a), SC_REFLECT_FIELDS_61(Type, __VA_ARGS__)
#define SC_REFLECT_FIELDS_63(Type, a, ...) REFLECT_FIELD(Type, a), SC_REFLECT_FIELDS_62(Type, __VA_ARGS__)
#define SC_REFLECT_FIELDS_64(Type, a, ...) REFLECT_FIELD(Type, a), SC_REFLECT_FIELDS_63(Type, __VA_ARGS__)

#define SC_REFLECT_NARGS_IMPL(_1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32, _33, _34, _35, _36, _37, _38, _39, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _60, _61, _62, _63, _64, N, ...) N
#define SC_REFLECT_NARGS(...) SC_REFLECT_NARGS_IMPL(__VA_ARGS__, 64, 63, 62, 61, 60, 59, 58, 57, 56, 55, 54, 53, 52, 51, 50, 49, 48, 47, 46, 45, 44, 43, 42, 41, 40, 39, 38, 37, 36, 35, 34, 33, 32, 31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1)

#define SC_REFLECT_CAT_(a, b) a##b
#define SC_REFLECT_CAT(a, b) SC_REFLECT_CAT_(a, b)
#define SC_REFLECT_FIELD_LIST(Type, ...)                    \
  SC_REFLECT_CAT(SC_REFLECT_FIELDS_, SC_REFLECT_NARGS(__VA_ARGS__))(Type, __VA_ARGS__)

#define REGISTER_FIELDS(Type, ...)                                   \
  template <>                                                        \
  struct sc::reflect::Descriptor<Type> {                             \
    static constexpr auto fields() {                                 \
      return std::tuple{SC_REFLECT_FIELD_LIST(Type, __VA_ARGS__)};   \
    }                                                                \
  };
// clang-format on

#endif  // SCCOMMON_DESCRIPTOR_H
