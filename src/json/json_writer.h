//
// Created by jiangyuyun3 on 2026/10/15.
//

#ifndef SCCOMMON_JSON_WRITER_H
#define SCCOMMON_JSON_WRITER_H

#include <optional>
#include <utility>

#include "json_owner.h"
#include "mixin/json_write_mixin.h"

namespace sc::json {

template <typename Json, JsonErrorHandler E = DefaultJsonErrorHandler>
class JsonWriter : public JsonWriteMixin<JsonWriter<Json, E>, Json, E> {
 public:
  using Self = JsonWriter;
  using OwnerType = JsonOwner<Json, E>;

  explicit JsonWriter(OwnerType& owner)
      : owner_(owner.valid() ? &owner : nullptr) {}
  JsonWriter() = delete;

  [[nodiscard]] const Json* node() const noexcept {
    return owner_ ? &owner_->json() : nullptr;
  }

  [[nodiscard]] OwnerType* owner() noexcept { return owner_; }

 private:
  OwnerType* owner_;
};

}  // namespace sc::json

#endif  // SCCOMMON_JSON_WRITER_H
