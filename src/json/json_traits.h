//
// Created by PC on 2026/9/13.
//

#ifndef SCCOMMON_JSON_TRAITS_H
#define SCCOMMON_JSON_TRAITS_H
#include <cstddef>
#include <cstdint>
#include <istream>
#include <string>
#include <string_view>

#include "util/json_error.h"

namespace sc::json {

// NOLINTBEGIN

// JsonOwner 中实际存储的类型（RapidJSON 的 Value 不自带分配器，持有 Document）
template <typename Json>
struct OwnedJson {
  using Type = Json;
};

template <typename Json>
using OwnedJsonT = OwnedJson<Json>::Type;

// 各库特化：把文本/流解析进持有类型，失败时调用 handler 再抛出
template <typename Json, JsonErrorHandler E = DefaultJsonErrorHandler>
struct JsonParseTraits {
  static void Parse(std::string_view text, OwnedJsonT<Json>& out, E& handler);
  static void Parse(std::istream& is, OwnedJsonT<Json>& out, E& handler);
};

// 节点类型判断
template <typename Json>
struct JsonTypeTraits {
  static bool IsObject(const Json&) noexcept;
  static bool IsArray(const Json&) noexcept;
  static bool IsInt(const Json&) noexcept;
  static bool IsUint(const Json&) noexcept;
  static bool IsInt64(const Json&) noexcept;
  static bool IsUint64(const Json&) noexcept;
  static bool IsDouble(const Json&) noexcept;
  static bool IsString(const Json&) noexcept;
  static bool IsBool(const Json&) noexcept;
  static bool IsNull(const Json&) noexcept;
};

// 访问
template <typename Json>
struct JsonAccessTraits {
  static bool GetInt(const Json&, int&) noexcept;
  static bool GetUint(const Json&, unsigned int&) noexcept;
  static bool GetInt64(const Json&, std::int64_t&) noexcept;
  static bool GetUint64(const Json&, std::uint64_t&) noexcept;
  static bool GetDouble(const Json&, double&) noexcept;
  static bool GetString(const Json&, std::string&);
  static bool GetBool(const Json&, bool&) noexcept;

  static const Json* TryGetChild(const Json&, std::string_view key) noexcept;
  static std::size_t GetArraySize(const Json&) noexcept;
  static const Json& GetArrayElement(const Json&, std::size_t index) noexcept;
};

// 其他
template <typename Json>
struct JsonOtherTraits {
  static std::string ToString(const Json&);
  static void ToStream(std::ostream& os, const Json&);

  // 迭代器接口，由各库特化提供实际类型
  using MemberIterator = void;
  static MemberIterator MemberBegin(const Json&) noexcept;
  static MemberIterator MemberEnd(const Json&) noexcept;

  // 创建一个空的Json，等价于"{}"
  static OwnedJsonT<Json> Create();
};

template <typename Json>
struct JsonOwnerStorage;

template <typename Json>
struct JsonWriteTraits {
  static Json* AddChild(JsonOwnerStorage<Json>& owner, const Json& parent,
                        std::string_view name) noexcept;

  static bool RemoveChild(JsonOwnerStorage<Json>& owner, Json& parent,
                          std::string_view name) noexcept;

  static bool Copy(JsonOwnerStorage<Json>& owner, const Json& src,
                   Json& dst) noexcept;

  static bool Move(JsonOwnerStorage<Json>& owner, Json&& src,
                   Json& dst) noexcept;

  static Json* AddInt(JsonOwnerStorage<Json>& owner, const Json& parent,
                      std::string_view name, int value) noexcept;

  static Json* AddUint(JsonOwnerStorage<Json>& owner, const Json& parent,
                       std::string_view name, unsigned int value) noexcept;

  static Json* AddInt64(JsonOwnerStorage<Json>& owner, const Json& parent,
                        std::string_view name, std::int64_t value) noexcept;

  static Json* AddUint64(JsonOwnerStorage<Json>& owner, const Json& parent,
                         std::string_view name, std::uint64_t value) noexcept;

  static Json* AddDouble(JsonOwnerStorage<Json>& owner, const Json& parent,
                         std::string_view name, double value) noexcept;

  static Json* AddBool(JsonOwnerStorage<Json>& owner, const Json& parent,
                       std::string_view name, bool value) noexcept;

  static Json* AddNull(JsonOwnerStorage<Json>& owner, const Json& parent,
                       std::string_view name) noexcept;

  static Json* AddString(JsonOwnerStorage<Json>& owner, const Json& parent,
                         std::string_view name,
                         std::string_view value) noexcept;

  static Json* AddList(JsonOwnerStorage<Json>& owner, const Json& parent,
                       std::string_view name) noexcept;

  template <typename T>
  static Json* Append(JsonOwnerStorage<Json>& owner, Json& array,
                      const T& value) noexcept;

  static Json* AppendObject(JsonOwnerStorage<Json>& owner,
                            Json& array) noexcept;
};

template <typename Json>
struct JsonTraits : JsonTypeTraits<Json>,
                    JsonAccessTraits<Json>,
                    JsonOtherTraits<Json>,
                    JsonWriteTraits<Json> {};

// NOLINTEND

}  // namespace sc::json

#endif  // SCCOMMON_JSON_TRAITS_H
