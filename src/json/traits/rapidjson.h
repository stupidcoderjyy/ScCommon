//
// Created by jiangyuyun3 on 2026/9/8.
//

#ifndef SCCOMMON_RAPIDJSON_N_H
#define SCCOMMON_RAPIDJSON_N_H

#include <rapidjson/document.h>

#include <cstddef>
#include <iterator>
#include <limits>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>

#include "../json_traits.h"

template <>
struct sc::json::JsonTraits<rapidjson::Value> {
  using Json = rapidjson::Value;

  static bool IsObject(const Json& json) { return json.IsObject(); }
  static bool IsArray(const Json& json) { return json.IsArray(); }
  // Is<整型> 语义：JSON
  // 整数值能无损转换到目标类型即为真（不要求存储标志一致）。
  static bool IsInt(const Json& json) {
    return json.IsInt() ||
           (json.IsUint() &&
            json.GetUint() <=
                static_cast<unsigned int>(std::numeric_limits<int>::max())) ||
           (json.IsInt64() &&
            json.GetInt64() >= std::numeric_limits<int>::min() &&
            json.GetInt64() <= std::numeric_limits<int>::max());
  }
  static bool IsUint(const Json& json) {
    return json.IsUint() || (json.IsInt() && json.GetInt() >= 0) ||
           (json.IsInt64() && json.GetInt64() >= 0 &&
            json.GetInt64() <= static_cast<std::int64_t>(
                                   std::numeric_limits<unsigned int>::max()));
  }
  static bool IsInt64(const Json& json) {
    return json.IsInt() || json.IsUint() || json.IsInt64() ||
           (json.IsUint64() &&
            json.GetUint64() <= static_cast<std::uint64_t>((
                                    std::numeric_limits<std::int64_t>::max)()));
  }
  static bool IsUint64(const Json& json) {
    return json.IsUint() || json.IsUint64() ||
           (json.IsInt() && json.GetInt() >= 0) ||
           (json.IsInt64() && json.GetInt64() >= 0);
  }
  static bool IsDouble(const Json& json) { return json.IsDouble(); }
  static bool IsString(const Json& json) { return json.IsString(); }
  static bool IsBool(const Json& json) { return json.IsBool(); }

  static const Json* TryGetChild(const Json& json, std::string_view key) {
    if (!json.IsObject()) {
      return nullptr;
    }
    // StringRef 构造出「常量字符串」视图，不拷贝、不分配
    const Json name(rapidjson::StringRef(
        key.data(), static_cast<rapidjson::SizeType>(key.size())));
    const auto it = json.FindMember(name);
    return it == json.MemberEnd() ? nullptr : &it->value;
  }
  static std::size_t GetArraySize(const Json& json) { return json.Size(); }
  static const Json& GetArrayElement(const Json& json, std::size_t index) {
    return json[static_cast<rapidjson::SizeType>(index)];
  }
  struct MemberIterator {
    using difference_type = std::ptrdiff_t;
    using value_type = std::pair<std::string_view, const Json*>;
    using reference = value_type;
    using pointer = void;
    using iterator_concept = std::forward_iterator_tag;
    using iterator_category = std::forward_iterator_tag;

    Json::ConstMemberIterator it{};

    value_type operator*() const {
      return {
          std::string_view(it->name.GetString(), it->name.GetStringLength()),
          &it->value};
    }
    MemberIterator& operator++() {
      ++it;
      return *this;
    }
    MemberIterator operator++(int) {
      MemberIterator copy = *this;
      ++it;
      return copy;
    }
    bool operator==(const MemberIterator& other) const {
      return it == other.it;
    }
  };

  static MemberIterator MemberBegin(const Json& json) {
    return {json.MemberBegin()};
  }
  static MemberIterator MemberEnd(const Json& json) {
    return {json.MemberEnd()};
  }

  static int GetInt(const Json& json) {
    if (json.IsInt64()) {
      return static_cast<int>(json.GetInt64());
    }
    if (json.IsUint()) {
      return static_cast<int>(json.GetUint());
    }
    return json.GetInt();
  }
  static unsigned int GetUint(const Json& json) {
    if (json.IsUint()) {
      return json.GetUint();
    }
    if (json.IsInt64()) {
      return static_cast<unsigned int>(json.GetInt64());
    }
    return static_cast<unsigned int>(json.GetInt());
  }
  static std::int64_t GetInt64(const Json& json) {
    if (json.IsInt64()) {
      return json.GetInt64();
    }
    if (json.IsUint()) {
      return json.GetUint();
    }
    return json.GetInt();
  }
  static std::uint64_t GetUint64(const Json& json) {
    if (json.IsUint64()) {
      return json.GetUint64();
    }
    if (json.IsUint()) {
      return json.GetUint();
    }
    if (json.IsInt64()) {
      return static_cast<std::uint64_t>(json.GetInt64());
    }
    return static_cast<std::uint64_t>(json.GetInt());
  }
  static double GetDouble(const Json& json) { return json.GetDouble(); }
  static std::string GetString(const Json& json) { return json.GetString(); }
  static bool GetBool(const Json& json) { return json.GetBool(); }
};

template <typename ErrorHandler>
class sc::json::JsonBuilder<rapidjson::Value, ErrorHandler> {
 public:
  JsonBuilder() = default;

  explicit JsonBuilder(std::string_view json, ErrorHandler error_handler = {}) {
    doc_.emplace();  // Document 自带的解析栈按需构造，只用 root 构造时不分配
    // 长度版解析：直接读输入缓冲，不必先拷贝成 NUL 结尾的 std::string
    doc_->Parse<rapidjson::kParseDefaultFlags, rapidjson::UTF8<>>(json.data(),
                                                                  json.size());
    if (doc_->HasParseError()) {
      std::runtime_error e("rapidjson parse error at offset " +
                           std::to_string(doc_->GetErrorOffset()));
      error_handler.OnJsonParseError(json, e);
      throw std::move(e);
    }
  }

  JsonBuilder(const JsonBuilder&) = delete;
  JsonBuilder(JsonBuilder&&) = delete;
  JsonBuilder& operator=(const JsonBuilder&) = delete;
  JsonBuilder& operator=(JsonBuilder&&) = delete;

  // Document 是 Value 的子类，且自带分配器；分配器随 JsonBuilder 存活，
  // 因此返回的 Value 引用在 JsonParser 生命周期内始终有效。
  [[nodiscard]] const rapidjson::Value& json() const { return *doc_; }

 private:
  std::optional<rapidjson::Document> doc_;
};

#endif  // SCCOMMON_RAPIDJSON_N_H
