//
// Created by PC on 2026/9/14.
//

#ifndef SCCOMMON_JSON_VIEW_H
#define SCCOMMON_JSON_VIEW_H

#include <string_view>
#include <type_traits>
#include <utility>

#include "json_access_mixin.h"
#include "json_path.h"
#include "json_traits_mixin.h"
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

template <typename Json, JsonErrorHandler ErrorHandler>
class JsonArrayView;

template <typename Json, JsonErrorHandler ErrorHandler>
class JsonObjectView;

template <typename Json,
          JsonErrorHandler ErrorHandler = DefaultJsonErrorHandler>
class JsonView : public JsonTraitsMixin<JsonView<Json, ErrorHandler>, Json>,
                 public JsonAccessMixin<JsonView<Json, ErrorHandler>, Json> {
 public:
  JsonView() noexcept = default;
  explicit JsonView(const Json& json, ErrorHandler handler = {}) noexcept
      : json_(&json), handler_(std::move(handler)) {}
  explicit JsonView(const JsonOwner<Json, ErrorHandler>& owner)
      : json_(owner ? &owner.json() : nullptr), handler_(owner.err_handler()) {}

  // 禁止临时JsonOwner构造
  explicit JsonView(JsonOwner<Json, ErrorHandler>&& json) = delete;

  JsonView(const JsonView&) noexcept = default;
  JsonView(JsonView&&) noexcept = default;
  JsonView& operator=(const JsonView&) noexcept = default;
  JsonView& operator=(JsonView&&) noexcept = default;

  template <typename ExistencePolicy = Optional, typename Path>
    requires PathLike<Path>
  JsonView View(const Path& path) const {
    static_assert(std::is_same_v<ExistencePolicy, Required> ||
                      std::is_same_v<ExistencePolicy, Optional>,
                  "JsonView: unsupported ExistencePolicy");
    if (!valid()) {
      return JsonView{};
    }
    const Json* node = detail::ResolveWithPolicy<Json, Path, ExistencePolicy>(
        json_, path, handler_);
    return node ? JsonView{*node, handler_} : JsonView{};
  }

  template <typename ExistencePolicy = Optional, typename Path>
    requires PathLike<Path>
  JsonArrayView<Json, ErrorHandler> ViewArray(const Path& path) const;

  template <typename ExistencePolicy = Optional, typename Path>
    requires PathLike<Path>
  JsonObjectView<Json, ErrorHandler> ViewObject(const Path& path) const;

  [[nodiscard]] bool valid() const noexcept { return json_; }
  [[nodiscard]] const Json* node() const noexcept { return json_; }

  explicit operator bool() const noexcept { return valid(); }

 private:
  friend class JsonTraitsMixin<JsonView, Json>;
  friend class JsonAccessMixin<JsonView, Json>;

  [[nodiscard]] ErrorHandler& error_handler() const noexcept {
    return handler_;
  }

  const Json* json_{nullptr};
  mutable ErrorHandler handler_{};
};

}  // namespace sc::json

#endif  // SCCOMMON_JSON_VIEW_H
