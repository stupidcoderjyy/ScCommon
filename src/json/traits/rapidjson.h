//
// Created by PC on 2026/9/13.
//

#ifndef SCCOMMON_RAPIDJSON_H
#define SCCOMMON_RAPIDJSON_H

#include <rapidjson/document.h>
#include <rapidjson/istreamwrapper.h>
#include <rapidjson/ostreamwrapper.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

#include <cstddef>
#include <iterator>
#include <limits>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>

#include "../json_owner.h"
#include "../json_traits.h"

namespace sc::json {

template <>
struct JsonTypeTraits<rapidjson::Value> {
  using Json = rapidjson::Value;

  static bool IsObject(const Json& json) { return json.IsObject(); }
  static bool IsArray(const Json& json) { return json.IsArray(); }
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
  static bool IsNull(const Json& json) { return json.IsNull(); }
};

template <>
struct JsonAccessTraits<rapidjson::Value> {
  using Json = rapidjson::Value;


  static bool GetInt(const Json& json, int& out) {
    if (json.IsInt()) {
      out = json.GetInt();
      return true;
    }
    if (json.IsUint()) {
      const unsigned int v = json.GetUint();
      if (v > static_cast<unsigned int>((std::numeric_limits<int>::max)())) {
        return false;
      }
      out = static_cast<int>(v);
      return true;
    }
    if (json.IsInt64()) {
      const std::int64_t v = json.GetInt64();
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
    if (json.IsUint()) {
      out = json.GetUint();
      return true;
    }
    if (json.IsInt()) {
      const int v = json.GetInt();
      if (v < 0) {
        return false;
      }
      out = static_cast<unsigned int>(v);
      return true;
    }
    if (json.IsInt64()) {
      const std::int64_t v = json.GetInt64();
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
    if (json.IsInt64()) {
      out = json.GetInt64();
      return true;
    }
    if (json.IsInt()) {
      out = json.GetInt();
      return true;
    }
    if (json.IsUint()) {
      out = json.GetUint();
      return true;
    }
    if (json.IsUint64()) {
      const std::uint64_t v = json.GetUint64();
      if (v > static_cast<std::uint64_t>(
                  (std::numeric_limits<std::int64_t>::max)())) {
        return false;
      }
      out = static_cast<std::int64_t>(v);
      return true;
    }
    return false;
  }
  static bool GetUint64(const Json& json, std::uint64_t& out) {
    if (json.IsUint64()) {
      out = json.GetUint64();
      return true;
    }
    if (json.IsUint()) {
      out = json.GetUint();
      return true;
    }
    if (json.IsInt()) {
      const int v = json.GetInt();
      if (v < 0) {
        return false;
      }
      out = static_cast<std::uint64_t>(v);
      return true;
    }
    if (json.IsInt64()) {
      const std::int64_t v = json.GetInt64();
      if (v < 0) {
        return false;
      }
      out = static_cast<std::uint64_t>(v);
      return true;
    }
    return false;
  }
  static bool GetDouble(const Json& json, double& out) {
    if (!json.IsDouble()) {
      return false;
    }
    out = json.GetDouble();
    return true;
  }
  static bool GetString(const Json& json, std::string& out) {
    if (!json.IsString()) {
      return false;
    }
    out = json.GetString();
    return true;
  }
  static bool GetBool(const Json& json, bool& out) {
    if (!json.IsBool()) {
      return false;
    }
    out = json.GetBool();
    return true;
  }

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
};

template <>
struct JsonOtherTraits<rapidjson::Value> {
  using Json = rapidjson::Value;

  static std::string ToString(const Json& json) {
    rapidjson::StringBuffer buffer;
    rapidjson::Writer writer(buffer);
    json.Accept(writer);
    return buffer.GetString();
  }

  static void ToStream(std::ostream& os, const Json& json) {
    rapidjson::OStreamWrapper wrapper(os);
    rapidjson::Writer writer(wrapper);
    json.Accept(writer);
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
      return {
          std::string_view(it->name.GetString(), it->name.GetStringLength()),
          &it->value};
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

    Json::ConstMemberIterator it{};
  };

  static MemberIterator MemberBegin(const Json& json) {
    return {.it = json.MemberBegin()};
  }
  static MemberIterator MemberEnd(const Json& json) {
    return {.it = json.MemberEnd()};
  }
};

template <>
struct JsonWriteTraits<rapidjson::Value> {
  using Json = rapidjson::Value;

  template <JsonErrorHandler ErrorHandler>
  static Json* AddChild(JsonOwner<Json, ErrorHandler>& owner,
                        const Json& parent, std::string_view name) noexcept {
    return AddValue(owner, parent, name, [](Json& node) { node.SetObject(); });
  }
  template <JsonErrorHandler ErrorHandler>
  static Json* AddInt(JsonOwner<Json, ErrorHandler>& owner, const Json& parent,
                      std::string_view name, int value) noexcept {
    return AddValue(owner, parent, name,
                    [value](Json& node) { node.SetInt(value); });
  }
  template <JsonErrorHandler ErrorHandler>
  static Json* AddUint(JsonOwner<Json, ErrorHandler>& owner, const Json& parent,
                       std::string_view name, unsigned int value) noexcept {
    return AddValue(owner, parent, name,
                    [value](Json& node) { node.SetUint(value); });
  }
  template <JsonErrorHandler ErrorHandler>
  static Json* AddInt64(JsonOwner<Json, ErrorHandler>& owner,
                        const Json& parent, std::string_view name,
                        std::int64_t value) noexcept {
    return AddValue(owner, parent, name,
                    [value](Json& node) { node.SetInt64(value); });
  }
  template <JsonErrorHandler ErrorHandler>
  static Json* AddUint64(JsonOwner<Json, ErrorHandler>& owner,
                         const Json& parent, std::string_view name,
                         std::uint64_t value) noexcept {
    return AddValue(owner, parent, name,
                    [value](Json& node) { node.SetUint64(value); });
  }
  template <JsonErrorHandler ErrorHandler>
  static Json* AddDouble(JsonOwner<Json, ErrorHandler>& owner,
                         const Json& parent, std::string_view name,
                         double value) noexcept {
    return AddValue(owner, parent, name,
                    [value](Json& node) { node.SetDouble(value); });
  }
  template <JsonErrorHandler ErrorHandler>
  static Json* AddBool(JsonOwner<Json, ErrorHandler>& owner, const Json& parent,
                       std::string_view name, bool value) noexcept {
    return AddValue(owner, parent, name,
                    [value](Json& node) { node.SetBool(value); });
  }
  template <JsonErrorHandler ErrorHandler>
  static Json* AddNull(JsonOwner<Json, ErrorHandler>& owner, const Json& parent,
                       std::string_view name) noexcept {
    return AddValue(owner, parent, name, [](Json& node) { node.SetNull(); });
  }
  template <JsonErrorHandler ErrorHandler>
  static Json* AddString(JsonOwner<Json, ErrorHandler>& owner,
                         const Json& parent, std::string_view name,
                         std::string_view value) noexcept {
    auto& allocator = owner.GetAllocator();
    return AddValue(owner, parent, name, [&allocator, value](Json& node) {
      node.SetString(value.data(),
                     static_cast<rapidjson::SizeType>(value.size()), allocator);
    });
  }

  template <JsonErrorHandler ErrorHandler>
  static Json* AddList(JsonOwner<Json, ErrorHandler>& owner,
                       const Json& parent, std::string_view name) noexcept {
    return AddValue(owner, parent, name,
                    [](Json& node) { node.SetArray(); });
  }

  template <JsonErrorHandler ErrorHandler, typename T>
  static Json* Append(JsonOwner<Json, ErrorHandler>& owner, Json& array,
                      const T& value) noexcept {
    try {
      if constexpr (std::is_same_v<T, int> ||
                    std::is_same_v<T, unsigned int> ||
                    std::is_same_v<T, std::int64_t> ||
                    std::is_same_v<T, std::uint64_t> ||
                    std::is_same_v<T, double> || std::is_same_v<T, bool>) {
        array.PushBack(value, owner.GetAllocator());
        return &array[array.Size() - 1];
      } else {
        return nullptr;
      }
    } catch (...) {
      return nullptr;
    }
  }

  template <JsonErrorHandler ErrorHandler>
  static Json* AppendObject(JsonOwner<Json, ErrorHandler>& owner,
                            Json& array) noexcept {
    try {
      rapidjson::Value child(rapidjson::kObjectType);
      array.PushBack(child, owner.GetAllocator());
      return &array[array.Size() - 1];
    } catch (...) {
      return nullptr;
    }
  }

 private:
  template <JsonErrorHandler ErrorHandler, typename Setter>
  static Json* AddValue(JsonOwner<Json, ErrorHandler>& owner,
                        const Json& parent, std::string_view name,
                        Setter setter) noexcept {
    try {
      if (!parent.IsObject()) {
        return nullptr;
      }
      auto& node = const_cast<Json&>(parent);
      const rapidjson::Value key(rapidjson::StringRef(
          name.data(), static_cast<rapidjson::SizeType>(name.size())));
      if (auto it = node.FindMember(key); it != node.MemberEnd()) {
        setter(it->value);
        return &it->value;
      }
      Json child;
      setter(child);
      node.AddMember(
          rapidjson::StringRef(name.data(),
                               static_cast<rapidjson::SizeType>(name.size())),
          child, owner.GetAllocator());
      return &(--node.MemberEnd())->value;
    } catch (...) {
      return nullptr;
    }
  }
};

template <JsonErrorHandler ErrorHandler>
class JsonParser<rapidjson::Value, ErrorHandler> {
 public:
  JsonParser() = default;

  explicit JsonParser(std::string_view json, ErrorHandler error_handler = {}) {
    doc_.emplace();  // Document 自带的解析栈按需构造，只用 root 构造时不分配
    doc_->Parse<rapidjson::kParseDefaultFlags, rapidjson::UTF8<>>(json.data(),
                                                                  json.size());
    if (doc_->HasParseError()) {
      std::runtime_error e("rapidjson parse error at offset " +
                           std::to_string(doc_->GetErrorOffset()));
      error_handler.OnJsonParseError(json, e);
      throw std::move(e);
    }
  }
  explicit JsonParser(std::istream& is, ErrorHandler error_handler = {}) {
    doc_.emplace();
    rapidjson::IStreamWrapper wrapper(is);
    doc_->ParseStream(wrapper);
    if (doc_->HasParseError()) {
      std::runtime_error e("rapidjson parse error at offset " +
                           std::to_string(doc_->GetErrorOffset()));
      // 流式解析无法回读原始文本，交给 handler 的 json 文本为空
      error_handler.OnJsonParseError(std::string_view{}, e);
      throw std::move(e);
    }
  }
  JsonParser(const JsonParser&) = delete;
  JsonParser(JsonParser&&) = delete;
  JsonParser& operator=(const JsonParser&) = delete;
  JsonParser& operator=(JsonParser&&) = delete;

  // Document 是 Value 的子类，且自带分配器；分配器随 JsonParser 存活，
  // 因此返回的 Value 引用在 JsonParser 生命周期内始终有效。
  [[nodiscard]] const rapidjson::Value& json() const { return *doc_; }

 private:
  std::optional<rapidjson::Document> doc_;
};

// rapidjson::Value本身不持有数据
template <>
struct OwnedJson<rapidjson::Value> {
  using Type = rapidjson::Document;
};

template <JsonErrorHandler ErrorHandler>
struct JsonOwner<rapidjson::Value, ErrorHandler>
    : JsonOwnerStorage<rapidjson::Value, ErrorHandler> {
  explicit JsonOwner(std::string_view json, ErrorHandler handler = {}) {
    this->err_handler() = std::move(handler);
    this->json_
        .template Parse<rapidjson::kParseDefaultFlags, rapidjson::UTF8<>>(
            json.data(), json.size());
    if (this->json_.HasParseError()) {
      std::runtime_error e("rapidjson parse error at offset " +
                           std::to_string(this->json_.GetErrorOffset()));
      this->err_handler().OnJsonParseError(json, e);
      throw std::move(e);
    }
    this->MarkValid();
  }

  explicit JsonOwner(std::istream& is, ErrorHandler handler = {}) {
    this->err_handler() = std::move(handler);
    rapidjson::IStreamWrapper wrapper(is);
    this->json_.ParseStream(wrapper);
    if (this->json_.HasParseError()) {
      std::runtime_error e("rapidjson parse error at offset " +
                           std::to_string(this->json_.GetErrorOffset()));
      this->err_handler().OnJsonParseError(std::string_view{}, e);
      throw std::move(e);
    }
    this->MarkValid();
  }

  explicit JsonOwner(rapidjson::Document&& doc, ErrorHandler handler = {}) {
    this->err_handler() = std::move(handler);
    this->json_ = std::move(doc);
    this->MarkValid();
  }

  rapidjson::Document::AllocatorType& GetAllocator() {
    return this->json_.GetAllocator();
  }
};

}  // namespace sc::json

#endif  // SCCOMMON_RAPIDJSON_H
