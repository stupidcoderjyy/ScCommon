//
// Created by jiangyuyun3 on 2026/9/11.
//

#ifndef SCCOMMON_JSON_TRAITS_MIXIN_H
#define SCCOMMON_JSON_TRAITS_MIXIN_H

#include <cstddef>
#include <string>
#include <string_view>

#include "json_traits.h"

namespace sc::json {

// 把 JsonTraits<Json> 的节点操作直接暴露为成员函数，
// JsonParser 与 JsonView 都继承它，于是不必再写
// JsonTraits<Json>::X(parser.json())。
//
// Derived 需要提供：
//   const Json* node() const noexcept;    // 当前节点；无效 view 返回 nullptr
//
// 无效节点（无效 view）上的行为：
//   判定类（Is*）返回 false；TryGetChild 返回 nullptr；GetArraySize 返回 0；
//   取值类（Get*）返回默认构造值；GetArrayElement 返回静态空节点；
//   MemberBegin/MemberEnd 返回默认迭代器（构成空区间）。
template <typename Derived, typename Json>
class JsonTraitsMixin {
 public:
  // ---------------- 类型判定 ----------------
  [[nodiscard]] bool IsObject() const {
    return Get<bool, &JsonTraits<Json>::IsObject>();
  }
  [[nodiscard]] bool IsArray() const {
    return Get<bool, &JsonTraits<Json>::IsArray>();
  }
  [[nodiscard]] bool IsInt() const {
    return Get<bool, &JsonTraits<Json>::IsInt>();
  }
  [[nodiscard]] bool IsUint() const {
    return Get<bool, &JsonTraits<Json>::IsUint>();
  }
  [[nodiscard]] bool IsInt64() const {
    return Get<bool, &JsonTraits<Json>::IsInt64>();
  }
  [[nodiscard]] bool IsUint64() const {
    return Get<bool, &JsonTraits<Json>::IsUint64>();
  }
  [[nodiscard]] bool IsDouble() const {
    return Get<bool, &JsonTraits<Json>::IsDouble>();
  }
  [[nodiscard]] bool IsString() const {
    return Get<bool, &JsonTraits<Json>::IsString>();
  }
  [[nodiscard]] bool IsBool() const {
    return Get<bool, &JsonTraits<Json>::IsBool>();
  }

  // ---------------- 结构与子节点 ----------------
  [[nodiscard]] const Json* TryGetChild(std::string_view key) const {
    const Json* n = node();
    return n != nullptr ? JsonTraits<Json>::TryGetChild(*n, key) : nullptr;
  }

  [[nodiscard]] std::size_t GetArraySize() const {
    return Get<std::size_t, &JsonTraits<Json>::GetArraySize>();
  }

  [[nodiscard]] const Json& GetArrayElement(std::size_t index) const {
    const Json* n = node();
    return n != nullptr ? JsonTraits<Json>::GetArrayElement(*n, index)
                        : EmptyNode();
  }

  [[nodiscard]] typename JsonTraits<Json>::MemberIterator MemberBegin() const {
    const Json* n = node();
    return n != nullptr ? JsonTraits<Json>::MemberBegin(*n)
                        : typename JsonTraits<Json>::MemberIterator{};
  }

  [[nodiscard]] typename JsonTraits<Json>::MemberIterator MemberEnd() const {
    const Json* n = node();
    return n != nullptr ? JsonTraits<Json>::MemberEnd(*n)
                        : typename JsonTraits<Json>::MemberIterator{};
  }

  // ---------------- 取值 ----------------
  [[nodiscard]] int GetInt() const {
    return Get<int, &JsonTraits<Json>::GetInt>();
  }
  [[nodiscard]] unsigned int GetUint() const {
    return Get<unsigned int, &JsonTraits<Json>::GetUint>();
  }
  [[nodiscard]] std::int64_t GetInt64() const {
    return Get<std::int64_t, &JsonTraits<Json>::GetInt64>();
  }
  [[nodiscard]] std::uint64_t GetUint64() const {
    return Get<std::uint64_t, &JsonTraits<Json>::GetUint64>();
  }
  [[nodiscard]] double GetDouble() const {
    return Get<double, &JsonTraits<Json>::GetDouble>();
  }
  [[nodiscard]] std::string GetString() const {
    return Get<std::string, &JsonTraits<Json>::GetString>();
  }
  [[nodiscard]] bool GetBool() const {
    return Get<bool, &JsonTraits<Json>::GetBool>();
  }

 private:
  // 统一的读取封装：无效节点返回 T{}（指针类型即 nullptr）
  template <typename T, T (*Fn)(const Json&)>
  [[nodiscard]] T Get() const {
    const Json* n = node();
    return n != nullptr ? Fn(*n) : T{};
  }

  const Json* node() const noexcept {
    return static_cast<const Derived*>(this)->node();
  }

  // 无效节点上 GetArrayElement 的安全兜底
  static const Json& EmptyNode() {
    static const Json empty{};
    return empty;
  }
};

}  // namespace sc::json

#endif  // SCCOMMON_JSON_TRAITS_MIXIN_H
