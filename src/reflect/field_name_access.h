//
// Created by jiangyuyun3 on 2026/10/20.
//

#ifndef SCCOMMON_FIELD_NAME_ACCESS_H
#define SCCOMMON_FIELD_NAME_ACCESS_H

#include <cstddef>
#include <string_view>

#include "def.h"

namespace sc::reflect {

template <typename>
struct NameAliases {
  static constexpr std::string_view kSource{};
};

namespace detail {
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

constexpr bool IsSpace(char c) noexcept {
  return c == ' ' || c == '\t' || c == '\n' || c == '\r';
}

template <common::FixedString S, typename T>
consteval std::string_view AliasFieldName() noexcept {
  constexpr std::string_view source = NameAliases<T>::kSource;
  std::string_view field{};
  bool is_field = true;
  std::size_t i = 0;
  while (i < source.size()) {
    while (i < source.size() && IsSpace(source[i])) {
      ++i;
    }
    if (i >= source.size()) {
      break;
    }
    std::string_view token{};
    if (source[i] == '"') {
      const auto begin = ++i;
      while (i < source.size() && source[i] != '"') {
        if (source[i] == '\\' && i + 1 < source.size()) {
          ++i;
        }
        ++i;
      }
      token = source.substr(begin, i - begin);
      if (i < source.size()) {
        ++i;
      }
    } else {
      const auto begin = i;
      while (i < source.size() && source[i] != ',') {
        ++i;
      }
      token = source.substr(begin, i - begin);
      while (!token.empty() && IsSpace(token.back())) {
        token.remove_suffix(1);
      }
    }
    while (i < source.size() && source[i] != ',') {
      ++i;
    }
    if (i < source.size()) {
      ++i;
    }
    if (is_field) {
      field = token;
    } else if (token == S.view()) {
      return field;
    }
    is_field = !is_field;
  }
  return {};
}

}  // namespace detail

}  // namespace sc::reflect

#define REGISTER_ALIAS(Type, ...)                             \
  template <>                                                 \
  struct sc::reflect::NameAliases<Type> {                     \
    static constexpr std::string_view kSource = #__VA_ARGS__; \
  };

#endif  // SCCOMMON_FIELD_NAME_ACCESS_H
