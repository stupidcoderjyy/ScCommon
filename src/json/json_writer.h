//
// Created by jiangyuyun3 on 2026/10/15.
//

#ifndef SCCOMMON_JSON_WRITER_H
#define SCCOMMON_JSON_WRITER_H

#include <optional>
#include <utility>

#include "json_owner.h"
#include "json_write_mixin.h"

namespace sc::json {

template <typename Json,
          JsonErrorHandler ErrorHandler = DefaultJsonErrorHandler>
class JsonWriter
    : public JsonWriteMixin<JsonWriter<Json, ErrorHandler>, Json, ErrorHandler> {
 public:
  using Self = JsonWriter;
  using OwnerType = JsonOwner<Json, ErrorHandler>;

  explicit JsonWriter(Json&& json, ErrorHandler handler = ErrorHandler{})
      : owned_(std::in_place, std::move(json), std::move(handler)),
        owner_(&*owned_) {}
  explicit JsonWriter(OwnerType& owner) : owner_(&owner) {}
  JsonWriter() = delete;

  [[nodiscard]] bool valid() const noexcept { return owner_; }
  [[nodiscard]] const Json* node() const noexcept {
    return owner_ ? &owner_->json() : nullptr;
  }
  [[nodiscard]] OwnerType& owner() noexcept { return *owner_; }

 private:
  std::optional<OwnerType> owned_;
  OwnerType* owner_;
};

}  // namespace sc::json

#endif  // SCCOMMON_JSON_WRITER_H
