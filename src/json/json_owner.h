//
// Created by jiangyuyun3 on 2026/10/14.
//

#ifndef SCCOMMON_JSON_OWNER_H
#define SCCOMMON_JSON_OWNER_H

#include "util/json_error.h"

namespace sc::json {

// 在JsonOwner中实际存储的类型
template <typename Json>
struct OwnedJson {
  using Type = Json;
};

template <typename Json>
using OwnedJsonT = OwnedJson<Json>::Type;

template<typename Json, typename ErrorHandler = DefaultJsonErrorHandler>
class JsonOwnerStorage {
public:
  JsonOwnerStorage() = default;
  JsonOwnerStorage(const JsonOwnerStorage&) noexcept = delete;
  JsonOwnerStorage& operator=(const JsonOwnerStorage&) noexcept = delete;
  JsonOwnerStorage(JsonOwnerStorage&& o) noexcept: json_(std::move(o.json_)), err_handler_(std::move(o.err_handler_)), valid_(o.valid_) {
    o.valid_ = false;
  }
  JsonOwnerStorage& operator=(JsonOwnerStorage&& o) noexcept {
    if (&o != this) {
      json_ = std::move(o.json_);
      err_handler_ = std::move(o.err_handler_);
      valid_ = o.valid_;
      o.valid_ = false;
    }
    return *this;
  }

  void MarkValid() { valid_ = true; }

  [[nodiscard]] std::string ToString() const {
    return valid_ ? JsonTraits<Json>::ToString(View()) : std::string{};
  }

  void ToStream(std::ostream& os) const {
    JsonTraits<Json>::ToStream(os, View());
  }

  [[nodiscard]] bool Valid() const noexcept { return valid_; }
  Json* Ptr() { return valid_ ? &json_ : nullptr; }
  const Json* ConstPtr() const { return valid_ ? &json_ : nullptr; }
  const Json& View() const { return json_; }
  ErrorHandler& err_handler() { return err_handler_; }

  explicit operator bool() const { return valid_; }

  OwnedJsonT<Json> json_;
  ErrorHandler err_handler_;
  bool valid_ = false;
};

// 主模板
template <typename Json, typename ErrorHandler = DefaultJsonErrorHandler>
class JsonOwner : public JsonOwnerStorage<Json, ErrorHandler> {};

template <typename Json, typename ErrorHandler = DefaultJsonErrorHandler>
std::ostream& operator<<(std::ostream& os, const JsonOwner<Json, ErrorHandler>& owner) {
  owner.ToStream(os);
  return os;
}

}

#endif  // SCCOMMON_JSON_OWNER_H
