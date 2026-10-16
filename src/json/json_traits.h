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
  static Json Create();
};

template <typename Json, JsonErrorHandler>
struct JsonOwner;

template <typename Json>
struct JsonWriteTraits {
  template <JsonErrorHandler E>
  static Json* AddChild(JsonOwner<Json, E>& owner, const Json& parent,
                        std::string_view name) noexcept;

  template <JsonErrorHandler E>
  static bool RemoveChild(JsonOwner<Json, E>& owner, Json& parent,
                          std::string_view name) noexcept;

  template <JsonErrorHandler E>
  static Json* AddInt(JsonOwner<Json, E>& owner, const Json& parent,
                      std::string_view name, int value) noexcept;

  template <JsonErrorHandler E>
  static Json* AddUint(JsonOwner<Json, E>& owner, const Json& parent,
                       std::string_view name, unsigned int value) noexcept;

  template <JsonErrorHandler E>
  static Json* AddInt64(JsonOwner<Json, E>& owner, const Json& parent,
                        std::string_view name, std::int64_t value) noexcept;

  template <JsonErrorHandler E>
  static Json* AddUint64(JsonOwner<Json, E>& owner, const Json& parent,
                         std::string_view name, std::uint64_t value) noexcept;

  template <JsonErrorHandler E>
  static Json* AddDouble(JsonOwner<Json, E>& owner, const Json& parent,
                         std::string_view name, double value) noexcept;

  template <JsonErrorHandler E>
  static Json* AddBool(JsonOwner<Json, E>& owner, const Json& parent,
                       std::string_view name, bool value) noexcept;

  template <JsonErrorHandler E>
  static Json* AddNull(JsonOwner<Json, E>& owner, const Json& parent,
                       std::string_view name) noexcept;

  template <JsonErrorHandler E>
  static Json* AddString(JsonOwner<Json, E>& owner, const Json& parent,
                         std::string_view name,
                         std::string_view value) noexcept;

  template <JsonErrorHandler E>
  static Json* AddList(JsonOwner<Json, E>& owner, const Json& parent,
                       std::string_view name) noexcept;

  template <JsonErrorHandler E, typename T>
  static Json* Append(JsonOwner<Json, E>& owner, Json& array,
                      const T& value) noexcept;

  template <JsonErrorHandler E>
  static Json* AppendObject(JsonOwner<Json, E>& owner, Json& array) noexcept;
};

template <typename Json>
struct JsonTraits : JsonTypeTraits<Json>,
                    JsonAccessTraits<Json>,
                    JsonOtherTraits<Json>,
                    JsonWriteTraits<Json> {};

// NOLINTEND

}  // namespace sc::json

#endif  // SCCOMMON_JSON_TRAITS_H
