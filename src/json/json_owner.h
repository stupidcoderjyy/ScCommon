//
// Created by jiangyuyun3 on 2026/10/14.
//

#ifndef SCCOMMON_JSON_OWNER_H
#define SCCOMMON_JSON_OWNER_H

#include "json_traits.h"
#include "util/json_error.h"

namespace sc::json {

template <typename Json, JsonErrorHandler E = DefaultJsonErrorHandler>
struct JsonOwnerStorage {
  JsonOwnerStorage() = default;
  explicit JsonOwnerStorage(OwnedJsonT<Json>&& json)
      : json_(std::move(json)), valid_(true) {}
  explicit JsonOwnerStorage(std::string_view text,
                            E& handler = GetDefaultErrorHandler<E>()) {
    JsonParseTraits<Json, E>::Parse(text, json_, handler);
    valid_ = true;
  }
  explicit JsonOwnerStorage(std::istream& is,
                            E& handler = GetDefaultErrorHandler<E>()) {
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
  [[nodiscard]] Json& json() { return json_; }
  [[nodiscard]] const Json& json() const { return json_; }

  explicit operator bool() const { return valid_; }

  OwnedJsonT<Json> json_;
  bool valid_ = false;
};

// 主模板
template <typename Json, JsonErrorHandler E = DefaultJsonErrorHandler>
struct JsonOwner : JsonOwnerStorage<Json, E> {
  using JsonOwnerStorage<Json, E>::JsonOwnerStorage;
};

template <typename Json, JsonErrorHandler E = DefaultJsonErrorHandler>
std::ostream& operator<<(std::ostream& os, const JsonOwner<Json, E>& owner) {
  owner.ToStream(os);
  return os;
}

}  // namespace sc::json

#endif  // SCCOMMON_JSON_OWNER_H
