//
// Created by PC on 2026/9/14.
//

#ifndef SCCOMMON_JSON_VIEW_H
#define SCCOMMON_JSON_VIEW_H

#include "json_owner.h"
#include "mixin/json_access_mixin.h"
#include "mixin/json_traits_mixin.h"
#include "util/json_detail.h"
#include "util/json_error.h"

#if defined(NLOHMANN_JSON_HPP) || defined(INCLUDE_NLOHMANN_JSON_HPP_) || \
    __has_include(<nlohmann/json.hpp>)
#include "traits/nlohmann.h"
#endif
#if defined(RAPIDJSON_DOCUMENT_H_) || __has_include(<rapidjson/document.h>)
#include "traits/rapidjson.h"
#endif
#if defined(JSON_JSON_H_INCLUDED) || __has_include(<json/json.h>)
#include "traits/jsoncpp.h"
#endif

namespace sc::json {

template <typename Json, JsonErrorHandler E>
class JsonArrayView;

template <typename Json, JsonErrorHandler E>
class JsonObjectView;

template <typename Json, JsonErrorHandler E = DefaultJsonErrorHandler>
class JsonView : public JsonTraitsMixin<JsonView<Json, E>, Json>,
                 public JsonAccessMixin<JsonView<Json, E>, Json> {
 public:
  using JsonT = Json;
  using ErrorHandlerT = E;

  JsonView() noexcept = default;
  explicit JsonView(const Json& json,
                    E& handler = GetDefaultErrorHandler<E>()) noexcept
      : json_(&json), handler_(&handler) {}
  template <JsonErrorHandler OwnerE>
  explicit JsonView(const JsonOwner<Json, OwnerE>& owner,
                    E& handler = GetDefaultErrorHandler<E>())
      : json_(owner ? &owner.json() : nullptr), handler_(&handler) {}

  // 禁止临时JsonOwner构造
  template <JsonErrorHandler OwnerE>
  explicit JsonView(JsonOwner<Json, OwnerE>&& owner,
                    E& handler = GetDefaultErrorHandler<E>()) = delete;

  JsonView(const JsonView&) noexcept = default;
  JsonView(JsonView&&) noexcept = default;
  JsonView& operator=(const JsonView&) noexcept = default;
  JsonView& operator=(JsonView&&) noexcept = default;

  template <AccessPolicy Policy = Optional, std::size_t N>
  JsonView View(const Parts<N>& path) const {
    if (!valid()) {
      return JsonView{};
    }
    const Json* node = detail::ResolveWithPolicy<Json, N, Policy>(
        json_, path, error_handler());
    return node ? JsonView{*node, error_handler()} : JsonView{};
  }

  template <AccessPolicy Policy = Optional, std::size_t N>
  JsonArrayView<Json, E> ViewArray(const Parts<N>& path) const;

  template <AccessPolicy Policy = Optional, std::size_t N>
  JsonObjectView<Json, E> ViewObject(const Parts<N>& path) const;

  [[nodiscard]] bool valid() const noexcept { return json_; }
  [[nodiscard]] const Json* node() const noexcept { return json_; }

  explicit operator bool() const noexcept { return valid(); }

 private:
  friend class JsonTraitsMixin<JsonView, Json>;
  friend class JsonAccessMixin<JsonView, Json>;

  [[nodiscard]] E& error_handler() const noexcept { return *handler_; }

  const Json* json_{nullptr};
  E* handler_{nullptr};
};

}  // namespace sc::json

#endif  // SCCOMMON_JSON_VIEW_H
