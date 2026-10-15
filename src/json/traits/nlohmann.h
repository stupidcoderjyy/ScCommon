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

  static bool GetInt(const Json& json, int& out) {
    if (json.is_number_unsigned()) {
      const std::uint64_t v = json.get<std::uint64_t>();
      if (v > static_cast<std::uint64_t>((std::numeric_limits<int>::max)())) {
        return false;
      }
      out = static_cast<int>(v);
      return true;
    }
    if (json.is_number_integer()) {
      const std::int64_t v = json.get<std::int64_t>();
      if (v < (std::numeric_limits<int>::min)() ||
          v > (std::numeric_limits<int>::max)()) {
        return false;
      }
      out = static_cast<int>(v);
      return true;
    }
    return false;
  }
  static bool GetUint(const Json& json, unsigned int& out) {
    if (json.is_number_unsigned()) {
      const std::uint64_t v = json.get<std::uint64_t>();
      if (v > static_cast<std::uint64_t>(
                  (std::numeric_limits<unsigned int>::max)())) {
        return false;
      }
      out = static_cast<unsigned int>(v);
      return true;
    }
    if (json.is_number_integer()) {
      const std::int64_t v = json.get<std::int64_t>();
      if (v < 0 || v > static_cast<std::int64_t>(
                           (std::numeric_limits<unsigned int>::max)())) {
        return false;
      }
      out = static_cast<unsigned int>(v);
      return true;
    }
    return false;
  }
  static bool GetInt64(const Json& json, std::int64_t& out) {
    if (json.is_number_unsigned()) {
      const std::uint64_t v = json.get<std::uint64_t>();
      if (v > static_cast<std::uint64_t>(
                  (std::numeric_limits<std::int64_t>::max)())) {
        return false;
      }
      out = static_cast<std::int64_t>(v);
      return true;
    }
    if (!json.is_number_integer()) {
      return false;
    }
    out = json.get<std::int64_t>();
    return true;
  }
  static bool GetUint64(const Json& json, std::uint64_t& out) {
    if (json.is_number_unsigned()) {
      out = json.get<std::uint64_t>();
      return true;
    }
    if (json.is_number_integer()) {
      const std::int64_t v = json.get<std::int64_t>();
      if (v < 0) {
        return false;
      }
      out = static_cast<std::uint64_t>(v);
      return true;
    }
    return false;
  }
  static bool GetDouble(const Json& json, double& out) {
    if (!json.is_number_float()) {
      return false;
    }
    out = json.get<double>();
    return true;
  }
  static bool GetString(const Json& json, std::string& out) {
    if (!json.is_string()) {
      return false;
    }
    out = json.get<std::string>();
    return true;
  }
  static bool GetBool(const Json& json, bool& out) {
    if (!json.is_boolean()) {
      return false;
    }
    out = json.get<bool>();
    return true;
  }

  static const Json* TryGetChild(const Json& json, std::string_view key) {
    if (!json.is_object()) {
      return nullptr;
    }
    const auto it = json.find(key);
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
  using Json = nlohmann::json;

  template <JsonErrorHandler E>
  static Json* AddChild(JsonOwner<Json, E>&, const Json& parent,
                        std::string_view name) noexcept {
    try {
      auto& node = const_cast<Json&>(parent);
      if (!node.is_object()) {
        return nullptr;
      }
      const std::string key(name);
      if (auto it = node.find(key); it != node.end()) {
        return &*it;
      }
      node[key] = Json::object();
      return &node[key];
    } catch (...) {
      return nullptr;
    }
  }

  template <JsonErrorHandler E>
  static Json* AddInt(JsonOwner<Json, E>&, const Json& parent,
                      std::string_view name, int value) noexcept {
    return SetAt(parent, name, value);
  }
  template <JsonErrorHandler E>
  static Json* AddUint(JsonOwner<Json, E>&, const Json& parent,
                       std::string_view name, unsigned int value) noexcept {
    return SetAt(parent, name, value);
  }
  template <JsonErrorHandler E>
  static Json* AddInt64(JsonOwner<Json, E>&, const Json& parent,
                        std::string_view name, std::int64_t value) noexcept {
    return SetAt(parent, name, value);
  }
  template <JsonErrorHandler E>
  static Json* AddUint64(JsonOwner<Json, E>&, const Json& parent,
                         std::string_view name, std::uint64_t value) noexcept {
    return SetAt(parent, name, value);
  }
  template <JsonErrorHandler E>
  static Json* AddDouble(JsonOwner<Json, E>&, const Json& parent,
                         std::string_view name, double value) noexcept {
    return SetAt(parent, name, value);
  }
  template <JsonErrorHandler E>
  static Json* AddBool(JsonOwner<Json, E>&, const Json& parent,
                       std::string_view name, bool value) noexcept {
    return SetAt(parent, name, value);
  }
  template <JsonErrorHandler E>
  static Json* AddNull(JsonOwner<Json, E>&, const Json& parent,
                       std::string_view name) noexcept {
    return SetAt(parent, name, nullptr);
  }
  template <JsonErrorHandler E>
  static Json* AddString(JsonOwner<Json, E>&, const Json& parent,
                         std::string_view name,
                         std::string_view value) noexcept {
    return SetAt(parent, name, std::string(value));
  }

  template <JsonErrorHandler E>
  static Json* AddList(JsonOwner<Json, E>&, const Json& parent,
                       std::string_view name) noexcept {
    return SetAt(parent, name, Json::array());
  }

  template <JsonErrorHandler E, typename T>
  static Json* Append(JsonOwner<Json, E>&, Json& array,
                      const T& value) noexcept {
    try {
      if constexpr (std::is_same_v<T, int> || std::is_same_v<T, unsigned int> ||
                    std::is_same_v<T, std::int64_t> ||
                    std::is_same_v<T, std::uint64_t> ||
                    std::is_same_v<T, double> || std::is_same_v<T, bool>) {
        array.push_back(value);
        return &array.back();
      } else {
        return nullptr;
      }
    } catch (...) {
      return nullptr;
    }
  }

  template <JsonErrorHandler E>
  static Json* AppendObject(JsonOwner<Json, E>&, Json& array) noexcept {
    try {
      array.push_back(Json::object());
      return &array.back();
    } catch (...) {
      return nullptr;
    }
  }

 private:
  template <typename T>
  static Json* SetAt(const Json& parent, std::string_view name,
                     T&& value) noexcept {
    try {
      auto& node = const_cast<Json&>(parent);
      if (!node.is_object()) {
        return nullptr;
      }
      const std::string key(name);
      if (auto it = node.find(key); it != node.end()) {
        *it = std::forward<T>(value);
        return &*it;
      }
      return &node.emplace(key, std::forward<T>(value)).first.value();
    } catch (...) {
      return nullptr;
    }
  }
};

template <JsonErrorHandler E>
struct JsonParseTraits<nlohmann::json, E> {
  static void Parse(std::string_view text, nlohmann::json& out, E& handler) {
    try {
      out = nlohmann::json::parse(text.data(), text.data() + text.size());
    } catch (std::exception& e) {
      handler.OnJsonParseError(e.what());
      throw;
    }
  }

  static void Parse(std::istream& is, nlohmann::json& out, E& handler) {
    try {
      out = nlohmann::json::parse(is);
    } catch (std::exception& e) {
      handler.OnJsonParseError(e.what());
      throw;
    }
  }
};

template <JsonErrorHandler E>
struct JsonOwner<nlohmann::json, E> : JsonOwnerStorage<nlohmann::json, E> {
  using JsonOwnerStorage<nlohmann::json, E>::JsonOwnerStorage;
};

}  // namespace sc::json

#endif  // SCCOMMON_NLOHMANN_H
