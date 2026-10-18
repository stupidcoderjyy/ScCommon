//
// Created by jiangyuyun3 on 2026/10/14.
//

#ifndef SCCOMMON_JSON_OWNER_H
#define SCCOMMON_JSON_OWNER_H

#include "json_traits.h"
#include "util/json_error.h"

namespace sc::json {

template <typename Json>
struct JsonOwnerStorage {
  JsonOwnerStorage() : json_(JsonOtherTraits<Json>::Create()), valid_(true) {}
  explicit JsonOwnerStorage(OwnedJsonT<Json>&& json)
      : json_(std::move(json)), valid_(true) {}
  template <JsonErrorHandler E>
  explicit JsonOwnerStorage(std::string_view text, E& handler) {
    JsonParseTraits<Json, E>::Parse(text, json_, handler);
    valid_ = true;
  }
  template <JsonErrorHandler E>
  explicit JsonOwnerStorage(std::istream& is, E& handler) {
    JsonParseTraits<Json, E>::Parse(is, json_, handler);
    valid_ = true;
  }
  JsonOwnerStorage(const JsonOwnerStorage&) noexcept = delete;
  JsonOwnerStorage& operator=(const JsonOwnerStorage&) noexcept = delete;
  JsonOwnerStorage(JsonOwnerStorage&& o) noexcept
      : json_(std::move(o.json_)), valid_(o.valid_) {
    o.valid_ = false;
  }
  JsonOwnerStorage& operator=(JsonOwnerStorage&& o) noexcept {
    if (&o != this) {
      json_ = std::move(o.json_);
      valid_ = o.valid_;
      o.valid_ = false;
    }
    return *this;
  }

  void MarkValid() { valid_ = true; }

  [[nodiscard]] std::string ToString() const {
    return valid_ ? JsonTraits<Json>::ToString(json()) : std::string{};
  }

  void ToStream(std::ostream& os) const {
    JsonTraits<Json>::ToStream(os, json());
  }

  [[nodiscard]] bool valid() const noexcept { return valid_; }
  [[nodiscard]] auto& json() { return json_; }
  [[nodiscard]] const auto& json() const { return json_; }
  [[nodiscard]] auto& GetAllocator() noexcept { return json_.GetAllocator(); }

  explicit operator bool() const { return valid_; }

 private:
  OwnedJsonT<Json> json_;
  bool valid_ = false;
};

// 主模板
template <typename Json, JsonErrorHandler E = DefaultJsonErrorHandler>
struct JsonOwner : JsonOwnerStorage<Json> {
  using Base = JsonOwnerStorage<Json>;

  explicit JsonOwner(E& handler = GetDefaultErrorHandler<E>())
      : Base(), handler_(&handler) {}
  explicit JsonOwner(OwnedJsonT<Json>&& json,
                     E& handler = GetDefaultErrorHandler<E>())
      : Base(std::move(json)), handler_(&handler) {}
  explicit JsonOwner(std::string_view text,
                     E& handler = GetDefaultErrorHandler<E>())
      : Base(text, handler), handler_(&handler) {}
  explicit JsonOwner(std::istream& is, E& handler = GetDefaultErrorHandler<E>())
      : Base(is, handler), handler_(&handler) {}

  JsonOwner(const JsonOwner&) noexcept = delete;
  JsonOwner& operator=(const JsonOwner&) noexcept = delete;
  JsonOwner(JsonOwner&& other) noexcept
      : Base(std::move(other)), handler_(other.handler_) {}
  JsonOwner& operator=(JsonOwner&& other) noexcept {
    if (this != &other) {
      Base::operator=(std::move(other));
      handler_ = other.handler_;
    }
    return *this;
  }

  [[nodiscard]] E& error_handler() const noexcept { return *handler_; }

 private:
  E* handler_{nullptr};
};

template <typename Json, JsonErrorHandler E = DefaultJsonErrorHandler>
std::ostream& operator<<(std::ostream& os, const JsonOwner<Json, E>& owner) {
  owner.ToStream(os);
  return os;
}

}  // namespace sc::json

#endif  // SCCOMMON_JSON_OWNER_H
