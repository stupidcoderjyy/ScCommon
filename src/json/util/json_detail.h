//
// Created by jiangyuyun3 on 2026/9/14.
//

#ifndef SCCOMMON_JSON_DETAIL_H
#define SCCOMMON_JSON_DETAIL_H

#include <charconv>
#include <cstddef>
#include <format>
#include <string>
#include <string_view>

#include "../json_path.h"
#include "../json_traits.h"

namespace sc::json::detail {

inline void AppendNumber(std::string& out, std::size_t value) {
  char buf[24];
  if (const auto [end, ec] = std::to_chars(buf, buf + sizeof(buf), value);
      ec == std::errc()) {
    out.append(buf, end);
  }
}

template <typename T>
struct ArrowProxy {
  T value;
  [[nodiscard]] T* operator->() noexcept { return &value; }
  [[nodiscard]] const T* operator->() const noexcept { return &value; }
};

// 把路径还原成 "a.b[2].c" 形式并追加到 out（Parts<N> 与 RuntimePath 通用）。
template <std::size_t N>
void AppendPath(std::string& out, const Parts<N>& path) {
  bool first = true;
  for (std::size_t i = 0; i < path.Size(); ++i) {
    const std::string_view token = path[i];
    if (const bool is_index = !token.empty() && token.front() == '[';
        !is_index && !first) {
      out += '.';
    }
    out += token;
    first = false;
  }
}

template <std::size_t N>
std::string PathString(const Parts<N>& path) {
  std::string out;
  AppendPath(out, path);
  return out;
}

//
// Json路径解析
//
template <typename Json, bool kCreateOnMissing = false>
const Json* Step(const Json* node, std::string_view token,
                 JsonOwnerStorage<Json>* owner = nullptr) {
  if (!node || token.empty()) {
    return nullptr;
  }
  if (token.front() != '[') {
    if (const Json* child = JsonTraits<Json>::TryGetChild(*node, token);
        child != nullptr) {
      return child;
    }
    if constexpr (kCreateOnMissing) {
      if (owner) {
        return JsonTraits<Json>::AddChild(*owner, *node, token);
      }
    }
    return nullptr;
  }
  if (token.size() < 2 || token.back() != ']') {
    return nullptr;
  }
  const std::string_view digits = token.substr(1, token.size() - 2);
  std::size_t index = 0;
  if (auto [ptr, ec] =
          std::from_chars(digits.data(), digits.data() + digits.size(), index);
      ec != std::errc() || ptr != digits.data() + digits.size()) {
    return nullptr;
  }
  if (!JsonTraits<Json>::IsArray(*node) ||
      index >= JsonTraits<Json>::GetArraySize(*node)) {
    return nullptr;
  }
  return &JsonTraits<Json>::GetArrayElement(*node, index);
}

template <typename Json, std::size_t N, bool kCreateOnMissing = false>
Json* Resolve(const Json* cur, const Parts<N>& path,
              JsonOwnerStorage<Json>* owner = nullptr) {
  Json* node = const_cast<Json*>(cur);
  for (std::size_t i = 0; node != nullptr && i < path.Size(); ++i) {
    node = const_cast<Json*>(
        Step<Json, kCreateOnMissing>(node, path[i], owner));
  }
  return node;
}

template <typename Json, std::size_t N, typename ExistencePolicy,
          JsonErrorHandler E, bool kCreateOnMissing = false>
Json* ResolveWithPolicy(const Json* cur, const Parts<N>& path, E& handler,
                        JsonOwnerStorage<Json>* owner = nullptr) {
  constexpr auto required = std::is_same_v<ExistencePolicy, Required>;
  constexpr auto create = required && kCreateOnMissing;
  if (auto* res = detail::Resolve<Json, N, create>(cur, path, owner); res) {
    return res;
  }
  if constexpr (required) {
    handler.OnPathResolveError(PathString(path));
  }
  return nullptr;
}

}  // namespace sc::json::detail

#endif  // SCCOMMON_JSON_DETAIL_H
