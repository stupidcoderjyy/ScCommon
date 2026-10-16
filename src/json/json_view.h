//
// Created by PC on 2026/9/14.
//

#ifndef SCCOMMON_JSON_VIEW_H
#define SCCOMMON_JSON_VIEW_H

#include <type_traits>
#include <utility>

#include "json_path.h"
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
                 public JsonAccessMixin<JsonView<Json, E>, Json, E> {
 public:
  using JsonT = Json;
  using ErrorHandlerT = E;

  JsonView() noexcept = default;
  explicit JsonView(const Json& json) noexcept : json_(&json) {}
  template <JsonErrorHandler OwnerE>
  explicit JsonView(const JsonOwner<Json, OwnerE>& owner)
      : json_(owner ? &owner.json() : nullptr) {}

  // 禁止临时JsonOwner构造
  template <JsonErrorHandler OwnerE>
  explicit JsonView(JsonOwner<Json, OwnerE>&& owner) = delete;

  JsonView(const JsonView&) noexcept = default;
  JsonView(JsonView&&) noexcept = default;
  JsonView& operator=(const JsonView&) noexcept = default;
  JsonView& operator=(JsonView&&) noexcept = default;

  template <AccessPolicy Policy = Optional, std::size_t N>
  JsonView View(const Parts<N>& path,
                E& handler = GetDefaultErrorHandler<E>()) const {
    if (!valid()) {
      return JsonView{};
    }
    const Json* node =
        detail::ResolveWithPolicy<Json, N, Policy>(json_, path, handler);
    return node ? JsonView{*node} : JsonView{};
  }

  template <AccessPolicy Policy = Optional, std::size_t N>
  JsonArrayView<Json, E> ViewArray(
      const Parts<N>& path, E& handler = GetDefaultErrorHandler<E>()) const;

  template <AccessPolicy Policy = Optional, std::size_t N>
  JsonObjectView<Json, E> ViewObject(
      const Parts<N>& path, E& handler = GetDefaultErrorHandler<E>()) const;

  [[nodiscard]] bool valid() const noexcept { return json_; }
  [[nodiscard]] const Json* node() const noexcept { return json_; }

  explicit operator bool() const noexcept { return valid(); }

 private:
  friend class JsonTraitsMixin<JsonView, Json>;
  friend class JsonAccessMixin<JsonView, Json, E>;

  const Json* json_{nullptr};
};

}  // namespace sc::json

#endif  // SCCOMMON_JSON_VIEW_H
