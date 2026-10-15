//
// Created by PC on 2026/9/13.
//

#ifndef SCCOMMON_NLOHMANN_H
#define SCCOMMON_NLOHMANN_H

#include <exception>
#include <istream>
#include <iterator>
#include <limits>
#include <nlohmann/json.hpp>
#include <string>
#include <string_view>
#include <utility>

#include "../json_owner.h"
#include "../json_traits.h"

namespace sc::json {

template <>
struct JsonTypeTraits<nlohmann::json> {
  using Json = nlohmann::json;

  static bool IsObject(const Json& json) { return json.is_object(); }
  static bool IsArray(const Json& json) { return json.is_array(); }
  static bool IsInt(const Json& json) {
    if (json.is_number_unsigned()) {
      return json.get<std::uint64_t>() <=
             static_cast<std::uint64_t>(std::numeric_limits<int>::max());
    }
    if (json.is_number_integer()) {
      const std::int64_t v = json.get<std::int64_t>();
      return v >= std::numeric_limits<int>::min() &&
             v <= std::numeric_limits<int>::max();
    }
    return false;
  }
  static bool IsUint(const Json& json) {
    if (json.is_number_unsigned()) {
      return json.get<std::uint64_t>() <=
             static_cast<std::uint64_t>(
                 std::numeric_limits<unsigned int>::max());
    }
    if (json.is_number_integer()) {
      const std::int64_t v = json.get<std::int64_t>();
      return v >= 0 && v <= static_cast<std::int64_t>(
                                std::numeric_limits<unsigned int>::max());
    }
    return false;
  }
  static bool IsInt64(const Json& json) {
    if (json.is_number_unsigned()) {
      return json.get<std::uint64_t>() <=
             static_cast<std::uint64_t>(
                 (std::numeric_limits<std::int64_t>::max)());
    }
    return json.is_number_integer();
  }
  static bool IsUint64(const Json& json) {
    if (json.is_number_integer()) {
      return json.get<std::int64_t>() >= 0;
    }
    return json.is_number_unsigned();
  }
  static bool IsDouble(const Json& json) { return json.is_number_float(); }
  static bool IsString(const Json& json) { return json.is_string(); }
  static bool IsBool(const Json& json) { return json.is_boolean(); }
  static bool IsNull(const Json& json) { return json.is_null(); }

};

template <>
struct JsonAccessTraits<nlohmann::json> {
  using Json = nlohmann::json;

  static int GetInt(const Json& json) { return json.get<int>(); }
  static unsigned int GetUint(const Json& json) {
    return json.get<unsigned int>();
  }
  static std::int64_t GetInt64(const Json& json) {
    return json.get<std::int64_t>();
  }
  static std::uint64_t GetUint64(const Json& json) {
    return json.get<std::uint64_t>();
  }
  static double GetDouble(const Json& json) { return json.get<double>(); }
  static std::string GetString(const Json& json) {
    return json.get<std::string>();
  }
  static bool GetBool(const Json& json) { return json.get<bool>(); }

  static const Json* TryGetChild(const Json& json, std::string_view key) {
    if (!json.is_object()) {
      return nullptr;
    }
    const auto it = json.find(std::string(key));
    return it == json.end() ? nullptr : &*it;
  }
  static std::size_t GetArraySize(const Json& json) { return json.size(); }
  static const Json& GetArrayElement(const Json& json, std::size_t index) {
    return json.at(index);
  }

};

template <>
struct JsonOtherTraits<nlohmann::json> {
  using Json = nlohmann::json;

  static std::string ToString(const Json& json) { return json.dump(); }

  static void ToStream(std::ostream& os, const Json& json) { os << json; }

  // Json迭代器 (C++20)
  struct MemberIterator {
    // 距离类型
    using difference_type = std::ptrdiff_t;
    // 指定迭代器能力
    using iterator_concept = std::forward_iterator_tag;
    // 类型
    using value_type = std::pair<std::string_view, const Json*>;

    value_type operator*() const {
      return {std::string_view(it.key()), &it.value()};
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
  };

  static MemberIterator MemberBegin(const Json& json) {
    return {.it = json.begin()};
  }
  static MemberIterator MemberEnd(const Json& json) {
    return {.it = json.end()};
  }
};

template <>
struct JsonWriteTraits<nlohmann::json> {
  template <JsonErrorHandler ErrorHandler>
  static nlohmann::json* AddChild(JsonOwner<nlohmann::json, ErrorHandler>&,
                                  const nlohmann::json& parent,
                                  std::string_view name) noexcept {
    try {
      auto& node = const_cast<nlohmann::json&>(parent);
      if (!node.is_object()) {
        return nullptr;
      }
      const std::string key(name);
      if (auto it = node.find(key); it != node.end()) {
        return &*it;
      }
      node[key] = nlohmann::json::object();
      return &node[key];
    } catch (...) {
      return nullptr;
    }
  }
};

template <JsonErrorHandler ErrorHandler>
class JsonParser<nlohmann::json, ErrorHandler> {
 public:
  JsonParser() = default;

  explicit JsonParser(std::string_view json, ErrorHandler error_handler = {}) {
    try {
      json_ = nlohmann::json::parse(json.data(), json.data() + json.size());
    } catch (std::exception& e) {
      error_handler.OnJsonParseError(json, e);
      throw;
    }
  }
  explicit JsonParser(std::istream& is, ErrorHandler error_handler = {}) {
    try {
      json_ = nlohmann::json::parse(is);
    } catch (std::exception& e) {
      // 流式解析无法回读原始文本，交给 handler 的 json 文本为空
      error_handler.OnJsonParseError(std::string_view{}, e);
      throw;
    }
  }
  JsonParser(const JsonParser&) = delete;
  JsonParser(JsonParser&&) = delete;
  JsonParser& operator=(const JsonParser&) = delete;
  JsonParser& operator=(JsonParser&&) = delete;

  [[nodiscard]] const nlohmann::json& json() const { return json_; }

 private:
  nlohmann::json json_;
};

// JsonOwner 的 nlohmann 偏特化：只补三种构造，特殊成员函数继承自
// JsonOwnerStorage
template <JsonErrorHandler ErrorHandler>
struct JsonOwner<nlohmann::json, ErrorHandler>
    : JsonOwnerStorage<nlohmann::json, ErrorHandler> {
  explicit JsonOwner(std::string_view json, ErrorHandler handler = {}) {
    this->err_handler() = std::move(handler);
    try {
      this->json_ =
          nlohmann::json::parse(json.data(), json.data() + json.size());
    } catch (std::exception& e) {
      this->err_handler().OnJsonParseError(json, e);
      throw;
    }
    this->MarkValid();
  }

  explicit JsonOwner(std::istream& is, ErrorHandler handler = {}) {
    this->err_handler() = std::move(handler);
    try {
      this->json_ = nlohmann::json::parse(is);
    } catch (std::exception& e) {
      // 流式解析无法回读原始文本，交给 handler 的 json 文本为空
      this->err_handler().OnJsonParseError(std::string_view{}, e);
      throw;
    }
    this->MarkValid();
  }

  // 夺舍外部 json：不解析、不抛解析异常，只接管所有权
  explicit JsonOwner(nlohmann::json&& json, ErrorHandler handler = {}) {
    this->err_handler() = std::move(handler);
    this->json_ = std::move(json);
    this->MarkValid();
  }
};

}  // namespace sc::json

#endif  // SCCOMMON_NLOHMANN_H
