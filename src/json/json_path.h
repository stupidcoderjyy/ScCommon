//
// Created by PC on 2026/9/14.
//

#ifndef SCCOMMON_JSON_PATH_H
#define SCCOMMON_JSON_PATH_H

#include <algorithm>
#include <concepts>
#include <cstddef>
#include <string>
#include <string_view>
#include <vector>

#include "common/template_string.h"
#include "util/json_path_util.h"

namespace sc::json {

using common::FixedString;
using common::Parts;

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


}  // namespace sc::json

#endif  // SCCOMMON_JSON_PATH_H
