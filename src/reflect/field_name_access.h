//
// Created by jiangyuyun3 on 2026/10/20.
//

#ifndef SCCOMMON_FIELD_NAME_ACCESS_H
#define SCCOMMON_FIELD_NAME_ACCESS_H

#include <memory>
#include <string_view>

namespace sc::reflect::detail {

template <typename T>
struct FakeObjectWrapper {
  const T value;
};

template <typename T>
extern const FakeObjectWrapper<T> kFakeObject;

template <typename T>
constexpr const T& FakeObject() noexcept {
  return kFakeObject<T>.value;
}

#ifndef SC_REFLECT_FUNCTION_SIGNATURE
#if defined(__FUNCSIG__)
#define SC_REFLECT_FUNCTION_SIGNATURE __FUNCSIG__
#elif defined(__PRETTY_FUNCTION__) || defined(__GNUC__) || defined(__clang__)
#define SC_REFLECT_FUNCTION_SIGNATURE __PRETTY_FUNCTION__
#else
#define SC_REFLECT_FUNCTION_SIGNATURE ""
#endif
#endif

template <typename T, auto Ptr>
consteval const char* Signature() noexcept {
  return SC_REFLECT_FUNCTION_SIGNATURE;
}

constexpr bool IsIdentifierChar(char c) noexcept {
  return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'z') ||
         (c >= 'A' && c <= 'Z') || c == '_';
}

consteval std::string_view ParseFieldName(std::string_view signature) noexcept {
  const auto amp = signature.rfind('&');
  if (amp == std::string_view::npos) {
    return {};
  }
  const auto tail = signature.substr(amp + 1);
  std::size_t begin = std::string_view::npos;
  std::size_t end = std::string_view::npos;
  for (std::size_t i = 0; i < tail.size(); ++i) {
    if (!IsIdentifierChar(tail[i])) {
      continue;
    }
    if (i > 0 && IsIdentifierChar(tail[i - 1])) {
      continue;
    }
    std::size_t run_end = i;
    while (run_end < tail.size() && IsIdentifierChar(tail[run_end])) {
      ++run_end;
    }
    const bool after_dot = i >= 1 && tail[i - 1] == '.';
    const bool after_scope = i >= 2 && tail.compare(i - 2, 2, "::") == 0;
    if (after_dot || after_scope) {
      begin = i;
      end = run_end;
    }
    i = run_end - 1;
  }
  if (begin == std::string_view::npos) {
    return {};
  }
  return tail.substr(begin, end - begin);
}

template <typename T, auto kPtr>
consteval std::string_view ParseFieldNameChecked() noexcept {
  constexpr auto name = ParseFieldName(Signature<T, kPtr>());
  static_assert(!name.empty(),
                "sc::reflect: unsupported field name signature format");
  return name;
}

}  // namespace sc::reflect::detail

#endif  // SCCOMMON_FIELD_NAME_ACCESS_H
