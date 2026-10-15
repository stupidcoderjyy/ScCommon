//
// Created by jiangyuyun3 on 2026/10/15.
//

#ifndef SCCOMMON_JSON_WRITER_H
#define SCCOMMON_JSON_WRITER_H

#include <utility>

#include "json_owner.h"
#include "json_traits.h"
#include "util/json_detail.h"

namespace sc::json {

template <typename Json, JsonErrorHandler ErrorHandler = DefaultJsonErrorHandler>
class JsonWriter {
 public:
  using Self = JsonWriter;

  explicit JsonWriter(Json& json, ErrorHandler handler = ErrorHandler{})
      : json_(&json), handler_(std::move(handler)) {}
  explicit JsonWriter(JsonOwner<Json, ErrorHandler>& owner)
      : json_(owner ? &owner.json() : nullptr), handler_(owner.err_handler()) {}
  JsonWriter() = delete;

  [[nodiscard]] bool valid() const noexcept { return json_; }
  [[nodiscard]] const Json* node() const noexcept { return json_; }
  [[nodiscard]] ErrorHandler& err_handler() noexcept { return handler_; }

  template <typename ExistencePolicy = Required, typename Path, typename T>
    requires PathLike<Path>
  Self& Set(const Path& path, const T& value) {
    if (Json* target =
            detail::ResolveWithPolicy<Json, Path, ExistencePolicy, ErrorHandler,
                                      true>(json_, path, handler_);
        target != nullptr) {
      *target = value;
    }
    return *this;
  }

  // 下标取路径返回可赋值代理：w[path] = value 与 Set(path, value) 完全等价
  template <typename Path, typename ExistencePolicy = Required>
    requires PathLike<Path>
  class Proxy {
   public:
    Proxy(Self& writer, const Path& path) : writer_(&writer), path_(path) {}

    template <typename T>
    Proxy& operator=(const T& value) {
      writer_->template Set<ExistencePolicy>(path_, value);
      return *this;
    }

   private:
    Self* writer_;
    Path path_;
  };

  template <typename ExistencePolicy = Required, typename Path>
    requires PathLike<Path>
  Proxy<Path, ExistencePolicy> operator[](const Path& path) {
    return Proxy<Path, ExistencePolicy>(*this, path);
  }

 private:

  Json* json_;
  ErrorHandler handler_;
};

}  // namespace sc::json

#endif  // SCCOMMON_JSON_WRITER_H
