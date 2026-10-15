//
// Created by jiangyuyun3 on 2026/10/15.
//

#ifndef SCCOMMON_JSON_WRITER_H
#define SCCOMMON_JSON_WRITER_H

#include <optional>
#include <utility>

#include "json_owner.h"
#include "json_traits.h"
#include "util/json_detail.h"
#include "util/json_read_write.h"

namespace sc::json {

template <typename Json,
          JsonErrorHandler ErrorHandler = DefaultJsonErrorHandler>
class JsonWriter {
 public:
  using Self = JsonWriter;

  explicit JsonWriter(Json&& json, ErrorHandler handler = ErrorHandler{})
      : owned_(std::in_place, std::move(json), std::move(handler)),
        owner_(&*owned_) {}
  explicit JsonWriter(JsonOwner<Json, ErrorHandler>& owner) : owner_(&owner) {}
  JsonWriter() = delete;

  template <typename ExistencePolicy = Required, std::size_t N, typename T>
  Self& Set(const Parts<N>& path, const T& value) {
    if constexpr (path.Size() == 0) {
      if constexpr (std::is_same_v<ExistencePolicy, Required>) {
        owner_->err_handler().OnPathResolveError(detail::PathString(path));
      }
      return *this;
    } else {
      Json* parent = nullptr;
      if constexpr (path.Size() == 1) {
        parent = &owner_->json();
      } else {
        const auto sub = path.template Sub<0, N - 1>();
        parent = detail::ResolveWithPolicy<Json, N - 1, ExistencePolicy,
                                           ErrorHandler, true>(
            &owner_->json(), sub, owner_->err_handler(), owner_);
      }
      if (parent == nullptr) {
        return *this;
      }
      if (!Injector<Json, T>::Inject(*owner_, *parent, path[path.Size() - 1],
                                     value)) {
        if constexpr (std::is_same_v<ExistencePolicy, Required>) {
          owner_->err_handler().OnTypeMismatch(detail::PathString(path));
        }
      }
      return *this;
    }
  }

  // 下标取路径返回可赋值代理：w[path] = value 与 Set(path, value) 完全等价
  template <std::size_t N, typename ExistencePolicy = Required>
  class Proxy {
   public:
    Proxy(Self& writer, const Parts<N>& path) : writer_(&writer), path_(path) {}

    template <typename T>
    Proxy& operator=(const T& value) {
      writer_->template Set<ExistencePolicy>(path_, value);
      return *this;
    }

   private:
    Self* writer_;
    Parts<N> path_;
  };

  template <typename ExistencePolicy = Required, std::size_t N>
  Proxy<N, ExistencePolicy> operator[](const Parts<N>& path) {
    return Proxy<N, ExistencePolicy>(*this, path);
  }

 private:
  std::optional<JsonOwner<Json, ErrorHandler>> owned_;
  JsonOwner<Json, ErrorHandler>* owner_;
};

}  // namespace sc::json

#endif  // SCCOMMON_JSON_WRITER_H
