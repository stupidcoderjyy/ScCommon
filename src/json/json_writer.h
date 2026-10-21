//
// Created by jiangyuyun3 on 2026/10/15.
//

#ifndef SCCOMMON_JSON_WRITER_H
#define SCCOMMON_JSON_WRITER_H

#include "json_owner.h"
#include "mixin/json_write_mixin.h"

namespace sc::json {

template <typename Json, JsonErrorHandler E = DefaultJsonErrorHandler>
class JsonWriter : public JsonWriteMixin<JsonWriter<Json, E>, Json> {
 public:
  using Self = JsonWriter;
  using OwnerType = JsonOwnerStorage<Json>;

  template <JsonErrorHandler OwnerE>
  explicit JsonWriter(JsonOwner<Json, OwnerE>& owner,
                      E& handler = GetDefaultErrorHandler<E>())
      : owner_(owner.valid() ? &owner : nullptr), handler_(&handler) {}
  JsonWriter() = delete;

  [[nodiscard]] const Json* node() const noexcept {
    return owner_ ? &owner_->json() : nullptr;
  }

  [[nodiscard]] OwnerType* owner() noexcept { return owner_; }
  [[nodiscard]] E& error_handler() const noexcept { return *handler_; }

 private:
  OwnerType* owner_;
  E* handler_{nullptr};
};

}  // namespace sc::json

#endif  // SCCOMMON_JSON_WRITER_H
