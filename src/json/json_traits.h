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
  static int GetInt(const Json&);
  static unsigned int GetUint(const Json&);
  static std::int64_t GetInt64(const Json&);
  static std::uint64_t GetUint64(const Json&);
  static double GetDouble(const Json&);
  static std::string GetString(const Json&);
  static bool GetBool(const Json&);

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
};

template <typename Json, JsonErrorHandler>
struct JsonOwner;

template <typename Json>
struct JsonWriteTraits {
  template <JsonErrorHandler ErrorHandler>
  static Json* AddChild(JsonOwner<Json, ErrorHandler>& owner,
                        const Json& parent, std::string_view name) noexcept;

  template <JsonErrorHandler ErrorHandler>
  static Json* AddInt(JsonOwner<Json, ErrorHandler>& owner, const Json& parent,
                      std::string_view name, int value) noexcept;

  template <JsonErrorHandler ErrorHandler>
  static Json* AddUint(JsonOwner<Json, ErrorHandler>& owner, const Json& parent,
                       std::string_view name, unsigned int value) noexcept;

  template <JsonErrorHandler ErrorHandler>
  static Json* AddInt64(JsonOwner<Json, ErrorHandler>& owner, const Json& parent,
                        std::string_view name, std::int64_t value) noexcept;

  template <JsonErrorHandler ErrorHandler>
  static Json* AddUint64(JsonOwner<Json, ErrorHandler>& owner,
                         const Json& parent, std::string_view name,
                         std::uint64_t value) noexcept;

  template <JsonErrorHandler ErrorHandler>
  static Json* AddDouble(JsonOwner<Json, ErrorHandler>& owner,
                         const Json& parent, std::string_view name,
                         double value) noexcept;

  template <JsonErrorHandler ErrorHandler>
  static Json* AddBool(JsonOwner<Json, ErrorHandler>& owner, const Json& parent,
                       std::string_view name, bool value) noexcept;

  template <JsonErrorHandler ErrorHandler>
  static Json* AddNull(JsonOwner<Json, ErrorHandler>& owner, const Json& parent,
                       std::string_view name) noexcept;

  template <JsonErrorHandler ErrorHandler>
  static Json* AddString(JsonOwner<Json, ErrorHandler>& owner,
                         const Json& parent, std::string_view name,
                        std::string_view value) noexcept;
};

template <typename Json>
struct JsonTraits : JsonTypeTraits<Json>,
                    JsonAccessTraits<Json>,
                    JsonOtherTraits<Json>,
                    JsonWriteTraits<Json> {};

// 从字符串/输入流构造json，构造出json的所有权归JsonParser所有
// 发生错误调用ErrorHandler::OnJsonParseError
template <typename Json,
          JsonErrorHandler ErrorHandler = DefaultJsonErrorHandler>
class JsonParser {
 public:
  JsonParser() = default;
  JsonParser(std::string_view json, ErrorHandler error_handler = {});
  JsonParser(std::istream& is, ErrorHandler error_handler = {});
  JsonParser(const JsonParser&) = delete;
  JsonParser(JsonParser&&) = delete;
  JsonParser& operator=(const JsonParser&) = delete;
  JsonParser& operator=(JsonParser&&) = delete;

  const Json& json() const { return json_; }

 private:
  Json json_;
};

// NOLINTEND

}  // namespace sc::json

#endif  // SCCOMMON_JSON_TRAITS_H
