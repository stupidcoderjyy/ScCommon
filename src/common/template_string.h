//
// Created by jiangyuyun3 on 2026/10/15.
//

#ifndef SCCOMMON_TEMPLATE_STRING_H
#define SCCOMMON_TEMPLATE_STRING_H
#include <algorithm>
#include <array>
#include <cstddef>
#include <span>
#include <string_view>

namespace sc::common {

// 编译器路径的表示
template <std::size_t N>
struct Parts {
  static constexpr std::size_t kSize = N;
  std::array<std::string_view, N> parts;
  constexpr std::size_t Size() const noexcept { return N; }  // NOLINT
  constexpr std::string_view operator[](std::size_t i) const noexcept {
    return parts[i];
  }
  constexpr auto Begin() const { return parts.begin(); }
  constexpr auto End() const { return parts.end(); }
  template <std::size_t kStart, std::size_t kOffset>
  [[nodiscard]] constexpr Parts<kOffset> Sub() const noexcept {
    Parts<kOffset> sub{};
    for (std::size_t i = 0; i < kOffset; ++i) {
      sub.parts[i] = parts[kStart + i];
    }
    return sub;
  }
};

template <typename... Ts>
Parts(Ts&&...) -> Parts<sizeof...(Ts)>;

// 用于提供编译期string_view
template <std::size_t N>
struct FixedString {
  std::array<char, N> data{};
  consteval FixedString(const char (&s)[N]) {  // NOLINT 必须支持隐式转换
    std::copy_n(s, N, data.begin());
  }
  [[nodiscard]] consteval std::string_view view() const {
    return {data.data(), N - 1};
  }
};

// 推导指引
// 缺少指引的话必须写 FixedString<4> str = "1234";
template <std::size_t N>
FixedString(const char (&)[N]) -> FixedString<N>;

}

#endif  // SCCOMMON_TEMPLATE_STRING_H
