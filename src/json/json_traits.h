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

template <typename Json>
struct JsonTraits {
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

  static std::string ToString(const Json&);
  static void ToStream(std::ostream& os, const Json&);

  // 迭代器接口，由各库特化提供实际类型
  using MemberIterator = void;
  static MemberIterator MemberBegin(const Json&) noexcept;
  static MemberIterator MemberEnd(const Json&) noexcept;
};

// 从字符串/输入流构造json，构造出json的所有权归JsonParser所有
// 发生错误调用ErrorHandler::OnJsonParseError
template <typename Json, typename ErrorHandler = DefaultJsonErrorHandler>
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
