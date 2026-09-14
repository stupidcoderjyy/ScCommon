//
// Created by PC on 2026/9/14.
//

#ifndef SCCOMMON_JSON_PATH_UTIL_H
#define SCCOMMON_JSON_PATH_UTIL_H

namespace sc::json {

consteval bool HasEmptySegment(std::string_view s) {
  if (s.empty()) return false;
  if (s.front() == '.' || s.back() == '.') return true;
  return s.find("..") != std::string_view::npos;
}

consteval bool HasInvalidBracket(std::string_view s) {
  std::size_t i = 0;
  while (i < s.size()) {
    const std::size_t open = s.find('[', i);
    if (open == std::string_view::npos) return false;
    const std::size_t close = s.find(']', open + 1);
    if (close == std::string_view::npos) return true;  // 未闭合
    const std::string_view digits = s.substr(open + 1, close - open - 1);
    if (digits.empty()) return true;  // "[]"
    for (const char c : digits) {
      if (c < '0' || c > '9') return true;  // "[abc]"
    }
    i = close + 1;
  }
  return false;
}

consteval bool NextToken(std::string_view s, std::size_t& i, std::size_t& start,
                         std::size_t& end) {
  if (i >= s.size()) return false;

  if (s[i] == '[') {
    const std::size_t close = s.find(']', i);
    start = i;
    end = close == std::string_view::npos ? s.size() : close + 1;
    i = end;
  } else {
    start = i;
    while (i < s.size() && s[i] != '.' && s[i] != '[') ++i;
    end = i;
  }

  if (i < s.size() && s[i] == '.') ++i;  // 跳过一个分隔点
  return true;
}

consteval std::size_t CountParts(std::string_view s) {
  std::size_t n = 0, i = 0, a, b;
  while (NextToken(s, i, a, b)) ++n;
  return n;
}

}  // namespace sc::json

#endif  // SCCOMMON_JSON_PATH_UTIL_H
