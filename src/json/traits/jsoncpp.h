//
// Created by PC on 2026/9/13.
//

#ifndef SCCOMMON_JSONCPP_H
#define SCCOMMON_JSONCPP_H

#include <json/json.h>

#include <cstddef>
#include <cstdint>
#include <iterator>
#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>

#include "../json_traits.h"

namespace sc::json {

template <>
struct JsonTypeTraits<Json::Value> {
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
  static bool IsNull(const Json& json) { return json.isNull(); }
};

template <>
struct JsonAccessTraits<Json::Value> {
  using Json = Json::Value;

  static bool GetInt(const Json& json, int& out) {
    if (!json.isInt()) {
      return false;
    }
    out = json.asInt();
    return true;
  }
  static bool GetUint(const Json& json, unsigned int& out) {
    if (!json.isUInt()) {
      return false;
    }
    out = json.asUInt();
    return true;
  }
  static bool GetInt64(const Json& json, std::int64_t& out) {
    if (!json.isInt64()) {
      return false;
    }
    out = json.asInt64();
    return true;
  }
  static bool GetUint64(const Json& json, std::uint64_t& out) {
    if (!json.isUInt64()) {
      return false;
    }
    out = json.asUInt64();
    return true;
  }
  static bool GetDouble(const Json& json, double& out) {
    if (!json.isDouble()) {
      return false;
    }
    out = json.asDouble();
    return true;
  }
  static bool GetString(const Json& json, std::string& out) {
    if (!json.isString()) {
      return false;
    }
    out = json.asString();
    return true;
  }
  static bool GetBool(const Json& json, bool& out) {
    if (!json.isBool()) {
      return false;
    }
    out = json.asBool();
    return true;
  }

  static const Json* TryGetChild(const Json& json, std::string_view key) {
    return json.isObject() ? json.find(key.data(), key.data() + key.size())
                           : nullptr;
  }

  static std::size_t GetArraySize(const Json& json) { return json.size(); }

  static const Json& GetArrayElement(const Json& json, std::size_t index) {
    return json[static_cast<Json::ArrayIndex>(index)];
  }
};

template <>
struct JsonOtherTraits<Json::Value> {
  using Json = Json::Value;

  static std::string ToString(const Json& json) {
    // 紧凑序列化。jsoncpp 默认把非 ASCII 转义成 \uXXXX，这里改为直接输出
    // UTF-8， 与 nlohmann / rapidjson 的输出保持一致（需要 jsoncpp >= 1.9.4）
    ::Json::StreamWriterBuilder builder;
    builder["indentation"] = "";
    builder["emitUTF8"] = true;
    return ::Json::writeString(builder, json);
  }

  static void ToStream(std::ostream& os, const Json& json) {
    ::Json::StreamWriterBuilder builder;
    builder["indentation"] = "";
    builder["emitUTF8"] = true;
    std::unique_ptr<::Json::StreamWriter> writer(builder.newStreamWriter());
    writer->write(json, &os);
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

  static ::Json::Value Create() { return {::Json::objectValue}; }
};

template <>
struct JsonWriteTraits<Json::Value> {
  static ::Json::Value* AddChild(JsonOwnerStorage<::Json::Value>&,
                                 const ::Json::Value& parent,
                                 std::string_view name) noexcept {
    try {
      auto& node = const_cast<::Json::Value&>(parent);
      if (!node.isObject()) {
        return nullptr;
      }
      const std::string key(name);
      if (node.isMember(key)) {
        return &node[key];
      }
      node[key] = ::Json::Value(::Json::objectValue);
      return &node[key];
    } catch (...) {
      return nullptr;
    }
  }

  static bool RemoveChild(JsonOwnerStorage<::Json::Value>&,
                          ::Json::Value& parent,
                          std::string_view name) noexcept {
    try {
      if (!parent.isObject()) {
        return false;
      }
      const std::string key(name);
      if (!parent.isMember(key)) {
        return false;
      }
      parent.removeMember(key);
      return true;
    } catch (...) {
      return false;
    }
  }

  static ::Json::Value* AddInt(JsonOwnerStorage<::Json::Value>&,
                               const ::Json::Value& parent,
                               std::string_view name, int value) noexcept {
    return SetAt(parent, name, value);
  }
  static ::Json::Value* AddUint(JsonOwnerStorage<::Json::Value>&,
                                const ::Json::Value& parent,
                                std::string_view name,
                                unsigned int value) noexcept {
    return SetAt(parent, name, value);
  }
  static ::Json::Value* AddInt64(JsonOwnerStorage<::Json::Value>&,
                                 const ::Json::Value& parent,
                                 std::string_view name,
                                 std::int64_t value) noexcept {
    return SetAt(parent, name, ::Json::Value(value));
  }
  static ::Json::Value* AddUint64(JsonOwnerStorage<::Json::Value>&,
                                  const ::Json::Value& parent,
                                  std::string_view name,
                                  std::uint64_t value) noexcept {
    return SetAt(parent, name, ::Json::Value(value));
  }
  static ::Json::Value* AddDouble(JsonOwnerStorage<::Json::Value>&,
                                  const ::Json::Value& parent,
                                  std::string_view name,
                                  double value) noexcept {
    return SetAt(parent, name, value);
  }
  static ::Json::Value* AddBool(JsonOwnerStorage<::Json::Value>&,
                                const ::Json::Value& parent,
                                std::string_view name, bool value) noexcept {
    return SetAt(parent, name, value);
  }
  static ::Json::Value* AddNull(JsonOwnerStorage<::Json::Value>&,
                                const ::Json::Value& parent,
                                std::string_view name) noexcept {
    return SetAt(parent, name, ::Json::Value(::Json::nullValue));
  }
  static ::Json::Value* AddString(JsonOwnerStorage<::Json::Value>&,
                                  const ::Json::Value& parent,
                                  std::string_view name,
                                  std::string_view value) noexcept {
    return SetAt(parent, name, std::string(value));
  }

  static ::Json::Value* AddList(JsonOwnerStorage<::Json::Value>&,
                                const ::Json::Value& parent,
                                std::string_view name) noexcept {
    return SetAt(parent, name, ::Json::Value(::Json::arrayValue));
  }

  template <typename T>
  static ::Json::Value* Append(JsonOwnerStorage<::Json::Value>&,
                               ::Json::Value& array, const T& value) noexcept {
    try {
      if constexpr (std::is_same_v<T, int> || std::is_same_v<T, unsigned int> ||
                    std::is_same_v<T, std::int64_t> ||
                    std::is_same_v<T, std::uint64_t> ||
                    std::is_same_v<T, double> || std::is_same_v<T, bool>) {
        array.append(value);
        return &array[(array.size() - 1)];
      } else {
        return nullptr;
      }
    } catch (...) {
      return nullptr;
    }
  }

  static ::Json::Value* AppendObject(JsonOwnerStorage<::Json::Value>&,
                                     ::Json::Value& array) noexcept {
    try {
      array.append(::Json::Value(::Json::objectValue));
      return &array[(array.size() - 1)];
    } catch (...) {
      return nullptr;
    }
  }

 private:
  template <typename T>
  static ::Json::Value* SetAt(const ::Json::Value& parent,
                              std::string_view name, T&& value) noexcept {
    try {
      auto& node = const_cast<::Json::Value&>(parent);
      if (!node.isObject()) {
        return nullptr;
      }
      const std::string key(name);
      ::Json::Value& child = node[key];
      child = std::forward<T>(value);
      return &child;
    } catch (...) {
      return nullptr;
    }
  }
};

template <JsonErrorHandler E>
struct JsonParseTraits<Json::Value, E> {
  static void Parse(std::string_view text, ::Json::Value& out, E& handler) {
    ::Json::CharReaderBuilder builder;
    std::unique_ptr<::Json::CharReader> reader(builder.newCharReader());
    ::Json::Value parsed;
    std::string errors;
    if (!reader->parse(text.data(), text.data() + text.size(), &parsed,
                       &errors)) {
      std::runtime_error e("jsoncpp parse error: " + errors);
      handler.OnJsonParseError(e.what());
      throw std::move(e);
    }
    out = std::move(parsed);
  }

  static void Parse(std::istream& is, ::Json::Value& out, E& handler) {
    ::Json::Value parsed;
    std::string errors;
    if (::Json::CharReaderBuilder builder;
        !::Json::parseFromStream(builder, is, &parsed, &errors)) {
      std::runtime_error e("jsoncpp parse error: " + errors);
      handler.OnJsonParseError(e.what());
      throw std::move(e);
    }
    out = std::move(parsed);
  }
};

}  // namespace sc::json

#endif  // SCCOMMON_JSONCPP_H
