//
// Created by PC on 2026/9/14.
//

#ifndef SCCOMMON_JSON_VIEW_H
#define SCCOMMON_JSON_VIEW_H

#include <istream>
#include <memory>
#include <string>
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

template <typename Json, typename ErrorHandler>
class JsonArrayView;

template <typename Json, typename ErrorHandler>
class JsonObjectView;

template <typename Json, typename ErrorHandler = DefaultJsonErrorHandler>
class JsonView : public JsonTraitsMixin<JsonView<Json, ErrorHandler>, Json>,
                 public JsonAccessMixin<JsonView<Json, ErrorHandler>, Json> {
 public:
  JsonView() noexcept = default;
  explicit JsonView(const Json& json, ErrorHandler handler = {}) noexcept
      : json_(&json), handler_(std::move(handler)) {}
  explicit JsonView(std::string_view json, ErrorHandler handler = {})
      : parser_(
            std::make_shared<JsonParser<Json, ErrorHandler>>(json, handler)),
        json_(&parser_->json()),
        handler_(std::move(handler)) {}
  explicit JsonView(const std::string& json, ErrorHandler handler = {})
      : JsonView(std::string_view(json), std::move(handler)) {}
  explicit JsonView(std::istream& is, ErrorHandler handler = {})
      : parser_(std::make_shared<JsonParser<Json, ErrorHandler>>(is, handler)),
        json_(&parser_->json()),
        handler_(std::move(handler)) {}

  // view 可拷贝、可移动：拥有解析结果时由 shared_ptr 共享所有权（仅文本构造的
  // view 才有它）
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
    auto* node = Find<ExistencePolicy>(path);
    return node ? JsonView{*node, handler_} : JsonView{};
  }

  template <typename ExistencePolicy = Optional, typename Path>
    requires PathLike<Path>
  JsonArrayView<Json, ErrorHandler> ViewArray(const Path& path) const;

  template <typename ExistencePolicy = Optional, typename Path>
    requires PathLike<Path>
  JsonObjectView<Json, ErrorHandler> ViewObject(const Path& path) const;

  [[nodiscard]] bool valid() const noexcept { return json_; }
  [[nodiscard]] const Json& json() const noexcept { return *json_; }

  explicit operator bool() const noexcept { return valid(); }

 private:
  friend class JsonTraitsMixin<JsonView, Json>;
  friend class JsonAccessMixin<JsonView, Json>;

  [[nodiscard]] ErrorHandler& error_handler() const noexcept {
    return handler_;
  }

  [[nodiscard]] const Json* node() const noexcept { return json_; }

  // 从当前节点开始解析 path；Required 且解析失败时交给 OnPathResolveError。
  template <typename ExistencePolicy, typename Path>
    requires PathLike<Path>
  const Json* Find(const Path& path) const {
    auto* cur = json_;
    std::size_t actual = 0;
    for (std::size_t i = 0; cur && i < path.Size(); ++i) {
      cur = detail::Step<Json>(cur, path[i]);
      if (cur) {
        ++actual;
      }
    }
    if (!cur) {
      if constexpr (std::is_same_v<ExistencePolicy, Required>) {
        const std::string_view failed =
            actual < path.Size() ? path[actual] : std::string_view{};
        handler_.OnPathResolveError(std::runtime_error{
            detail::ResolveErrorMessage(path, actual, failed)});
      }
    }
    return cur;
  }

  std::shared_ptr<JsonParser<Json, ErrorHandler>> parser_;
  const Json* json_{nullptr};
  mutable ErrorHandler handler_{};
};

}  // namespace sc::json

#endif  // SCCOMMON_JSON_VIEW_H
