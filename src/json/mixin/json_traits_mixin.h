//
// Created by PC on 2026/9/13.
//

#ifndef SCCOMMON_JSON_TRAITS_MIXIN_H
#define SCCOMMON_JSON_TRAITS_MIXIN_H

#include <cstddef>
#include <string>
#include <string_view>

#include "../json_traits.h"

namespace sc::json {

template <typename Derived, typename Json>
class JsonTraitsMixin {
 public:
  [[nodiscard]] bool IsObject() const {
    return ValueOr<bool, &JsonTraits<Json>::IsObject>();
  }
  [[nodiscard]] bool IsArray() const {
    return ValueOr<bool, &JsonTraits<Json>::IsArray>();
  }
  [[nodiscard]] bool IsInt() const {
    return ValueOr<bool, &JsonTraits<Json>::IsInt>();
  }
  [[nodiscard]] bool IsUint() const {
    return ValueOr<bool, &JsonTraits<Json>::IsUint>();
  }
  [[nodiscard]] bool IsInt64() const {
    return ValueOr<bool, &JsonTraits<Json>::IsInt64>();
  }
  [[nodiscard]] bool IsUint64() const {
    return ValueOr<bool, &JsonTraits<Json>::IsUint64>();
  }
  [[nodiscard]] bool IsDouble() const {
    return ValueOr<bool, &JsonTraits<Json>::IsDouble>();
  }
  [[nodiscard]] bool IsString() const {
    return ValueOr<bool, &JsonTraits<Json>::IsString>();
  }
  [[nodiscard]] bool IsBool() const {
    return ValueOr<bool, &JsonTraits<Json>::IsBool>();
  }
  [[nodiscard]] bool IsNull() const {
    return ValueOr<bool, &JsonTraits<Json>::IsNull>();
  }

  [[nodiscard]] int GetInt() const {
    auto* n = node();
    int out{};
    return n != nullptr && JsonTraits<Json>::GetInt(*n, out) ? out : int{};
  }
  [[nodiscard]] unsigned int GetUint() const {
    auto* n = node();
    unsigned int out{};
    return n != nullptr && JsonTraits<Json>::GetUint(*n, out) ? out : 0u;
  }
  [[nodiscard]] std::int64_t GetInt64() const {
    auto* n = node();
    std::int64_t out{};
    return n != nullptr && JsonTraits<Json>::GetInt64(*n, out)
               ? out
               : std::int64_t{};
  }
  [[nodiscard]] std::uint64_t GetUint64() const {
    auto* n = node();
    std::uint64_t out{};
    return n != nullptr && JsonTraits<Json>::GetUint64(*n, out)
               ? out
               : std::uint64_t{};
  }
  [[nodiscard]] double GetDouble() const {
    auto* n = node();
    double out{};
    return n != nullptr && JsonTraits<Json>::GetDouble(*n, out) ? out : 0.0;
  }
  [[nodiscard]] std::string GetString() const {
    auto* n = node();
    std::string out;
    if (n != nullptr && JsonTraits<Json>::GetString(*n, out)) {
      return out;
    }
    return {};
  }
  [[nodiscard]] bool GetBool() const {
    auto* n = node();
    bool out = false;
    return n != nullptr && JsonTraits<Json>::GetBool(*n, out) ? out : false;
  }

  [[nodiscard]] const Json* TryGetChild(std::string_view key) const {
    auto* n = node();
    return n ? JsonTraits<Json>::TryGetChild(*n, key) : nullptr;
  }

  [[nodiscard]] std::size_t GetArraySize() const {
    return ValueOr<std::size_t, &JsonTraits<Json>::GetArraySize>();
  }

  [[nodiscard]] const Json& GetArrayElement(std::size_t index) const {
    auto* n = node();
    return n ? JsonTraits<Json>::GetArrayElement(*n, index) : EmptyNode();
  }

  [[nodiscard]] std::string ToString() const {
    return ValueOr<std::string, &JsonTraits<Json>::ToString>();
  }

  [[nodiscard]] JsonTraits<Json>::MemberIterator MemberBegin() const {
    auto* n = node();
    return n ? JsonTraits<Json>::MemberBegin(*n)
             : typename JsonTraits<Json>::MemberIterator{};
  }

  [[nodiscard]] JsonTraits<Json>::MemberIterator MemberEnd() const {
    auto* n = node();
    return n ? JsonTraits<Json>::MemberEnd(*n)
             : typename JsonTraits<Json>::MemberIterator{};
  }

 private:
  const Json* node() const noexcept {
    return static_cast<const Derived*>(this)->node();
  }

  template <typename T, T (*Getter)(const Json&)>
  [[nodiscard]] T ValueOr() const {
    auto* n = node();
    return n ? Getter(*n) : T{};
  }

  static const Json& EmptyNode() {
    static const Json empty{};
    return empty;
  }
};

}  // namespace sc::json

#endif  // SCCOMMON_JSON_TRAITS_MIXIN_H
