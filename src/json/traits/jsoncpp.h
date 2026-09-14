//
// Created by PC on 2026/9/13.
//

#ifndef SCCOMMON_JSONCPP_H
#define SCCOMMON_JSONCPP_H

#include <json/json.h>

#include <cstddef>
#include <cstdint>
#include <istream>
#include <iterator>
#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>

#include "../json_traits.h"

namespace sc::json {

template <>
struct JsonTraits<Json::Value> {
  using Json = Json::Value;

  static bool IsObject(const Json& json) { return json.isObject(); }
  static bool IsArray(const Json& json) { return json.isArray(); }
  static bool IsInt(const Json& json) { return json.isInt(); }
  static bool IsUint(const Json& json) { return json.isUInt(); }
  static bool IsInt64(const Json& json) { return json.isInt64(); }
  static bool IsUint64(const Json& json) { return json.isUInt64(); }
  static bool IsDouble(const Json& json) { return json.isDouble(); }
  static bool IsString(const Json& json) { return json.isString(); }
  static bool IsBool(const Json& json) { return json.isBool(); }

  static int GetInt(const Json& json) { return json.asInt(); }
  static unsigned int GetUint(const Json& json) { return json.asUInt(); }
  static std::int64_t GetInt64(const Json& json) { return json.asInt64(); }
  static std::uint64_t GetUint64(const Json& json) { return json.asUInt64(); }
  static double GetDouble(const Json& json) { return json.asDouble(); }
  static std::string GetString(const Json& json) { return json.asString(); }
  static bool GetBool(const Json& json) { return json.asBool(); }

  static const Json* TryGetChild(const Json& json, std::string_view key) {
    return json.isObject() ? json.find(key.data(), key.data() + key.size())
                           : nullptr;
  }

  static std::size_t GetArraySize(const Json& json) { return json.size(); }

  static const Json& GetArrayElement(const Json& json, std::size_t index) {
    return json[static_cast<Json::ArrayIndex>(index)];
  }

  // Json迭代器 (C++20)
  struct MemberIterator {
    // 距离类型
    using difference_type = std::ptrdiff_t;
    // 指定迭代器能力
    using iterator_concept = std::forward_iterator_tag;
    // 类型
    using value_type = std::pair<std::string_view, const Json*>;

    value_type operator*() const {
      key = it.name();
      return {std::string_view(key), &*it};
    }
    MemberIterator& operator++() {
      ++it;
      return *this;
    }
    // 后置自增
    MemberIterator operator++(int) {
      auto copy = *this;
      ++it;
      return copy;
    }
    bool operator==(const MemberIterator& other) const {
      return it == other.it;
    }

    Json::const_iterator it{};
    // jsoncpp 的 name() 按值返回 std::string，键必须由迭代器持有
    mutable std::string key{};
  };

  static MemberIterator MemberBegin(const Json& json) {
    return {.it = json.begin()};
  }
  static MemberIterator MemberEnd(const Json& json) {
    return {.it = json.end()};
  }
};

template <typename ErrorHandler>
class JsonParser<Json::Value, ErrorHandler> {
 public:
  JsonParser() = default;

  explicit JsonParser(std::string_view json, ErrorHandler error_handler = {}) {
    Json::CharReaderBuilder builder;
    std::unique_ptr<Json::CharReader> reader(builder.newCharReader());
    Json::Value parsed;
    std::string errors;
    if (!reader->parse(json.data(), json.data() + json.size(), &parsed,
                       &errors)) {
      std::runtime_error e("jsoncpp parse error: " + errors);
      error_handler.OnJsonParseError(json, e);
      throw std::move(e);
    }
    json_ = std::move(parsed);
  }
  explicit JsonParser(std::istream& is, ErrorHandler error_handler = {}) {
    Json::Value parsed;
    std::string errors;
    if (Json::CharReaderBuilder builder;
        !Json::parseFromStream(builder, is, &parsed, &errors)) {
      std::runtime_error e("jsoncpp parse error: " + errors);
      // 流式解析无法回读原始文本，交给 handler 的 json 文本为空
      error_handler.OnJsonParseError(std::string_view{}, e);
      throw std::move(e);
    }
    json_ = std::move(parsed);
  }
  JsonParser(const JsonParser&) = delete;
  JsonParser(JsonParser&&) = delete;
  JsonParser& operator=(const JsonParser&) = delete;
  JsonParser& operator=(JsonParser&&) = delete;

  [[nodiscard]] const Json::Value& json() const { return json_; }

 private:
  Json::Value json_;
};

}  // namespace sc::json

#endif  // SCCOMMON_JSONCPP_H
