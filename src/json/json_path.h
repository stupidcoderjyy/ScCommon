//
// Created by PC on 2026/9/14.
//

#ifndef SCCOMMON_JSON_PATH_H
#define SCCOMMON_JSON_PATH_H

#include <algorithm>
#include <array>
#include <concepts>
#include <cstddef>
#include <string>
#include <string_view>
#include <vector>

#include "util/json_path_util.h"

namespace sc::json {

// 编译器路径的表示
template <std::size_t N>
struct Parts {
  std::array<std::string_view, N> parts;
  constexpr std::size_t Size() const noexcept { return N; }  // NOLINT
  constexpr std::string_view operator[](std::size_t i) const noexcept {
    return parts[i];
  }
  constexpr auto Begin() const { return parts.begin(); }
  constexpr auto End() const { return parts.end(); }
};

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

template <std::size_t N>
consteval Parts<N> MakeParts(std::string_view s) {
  Parts<N> r{};
  std::size_t n = 0, i = 0, a, b;
  while (NextToken(s, i, a, b)) r.parts[n++] = s.substr(a, b - a);
  return r;
}

template <FixedString S>
consteval auto operator""_path() {
  constexpr std::string_view view = S.view();
  static_assert(
      !HasEmptySegment(view),
      "JsonPath: empty segment is not allowed, e.g. \"a..b\", \".a\", \"a.\"");
  static_assert(!HasInvalidBracket(view),
                "JsonPath: '[' must be paired with ']' and contain digits "
                "only, e.g. \"a[2]\"");
  constexpr std::size_t N = CountParts(view);
  return MakeParts<N>(view);
}

// 路径概念：Parts<N>（编译期路径）与 RuntimePath（运行期路径）共同满足。
template <typename Path>
concept PathLike = requires(const Path& path, std::size_t index) {
  { path.Size() } -> std::convertible_to<std::size_t>;
  { path[index] } -> std::convertible_to<std::string_view>;
};

class RuntimePath {
 public:
  RuntimePath() = default;
  explicit RuntimePath(std::string_view path) { Parse(path); }

  [[nodiscard]] std::size_t Size() const noexcept { return tokens_.size(); }
  [[nodiscard]] std::string_view operator[](std::size_t index) const noexcept {
    return tokens_[index];
  }
  [[nodiscard]] bool Empty() const noexcept { return tokens_.empty(); }

 private:
  void Parse(std::string_view path) {
    std::size_t pos = 0;
    while (pos < path.size()) {
      const std::size_t dot = path.find('.', pos);
      const std::size_t end = dot == std::string_view::npos ? path.size() : dot;
      AppendSegment(path.substr(pos, end - pos));
      if (dot == std::string_view::npos) {
        break;
      }
      pos = dot + 1;
    }
  }

  // 一个路径段可能同时包含 key 与若干下标："b[2][3]" → "b", "[2]", "[3]"
  void AppendSegment(std::string_view segment) {
    std::size_t bracket = segment.find('[');
    if (const std::string_view key = segment.substr(0, bracket); !key.empty()) {
      tokens_.emplace_back(key);
    } else if (bracket == std::string_view::npos) {
      tokens_.emplace_back(segment);  // 空段：保留空 token，解析阶段报错
      return;
    }
    while (bracket != std::string_view::npos) {
      const std::size_t close = segment.find(']', bracket + 1);
      if (close == std::string_view::npos) {
        tokens_.emplace_back(segment.substr(bracket));  // 未闭合：解析阶段报错
        return;
      }
      tokens_.emplace_back(segment.substr(bracket, close - bracket + 1));
      bracket = segment.find('[', close + 1);
    }
  }

  std::vector<std::string> tokens_;
};

}  // namespace sc::json

#endif  // SCCOMMON_JSON_PATH_H
