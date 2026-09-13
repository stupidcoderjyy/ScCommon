//
// Created by jiangyuyun3 on 2026/9/7.
//

#ifndef SCCOMMON_JSON_PARSER_N_H
#define SCCOMMON_JSON_PARSER_N_H

#include <string_view>
#include <utility>

#include "json_access_mixin.h"
#include "json_error.h"
#include "json_extractor.h"  //NOLINT
#include "json_path.h"
#include "json_traits.h"
#include "json_traits_mixin.h"
#include "view/json_views.h"

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

// JsonParser = “根节点上的 JsonView” + 解析结果的所有权。
//   - 从已有节点构造：只持有 const Json&（零拷贝），节点须比 parser 存活更久；
//   - 从文本构造：解析结果由 JsonBuilder 持有，生命周期与 parser 一致；
//   - View 系列与 Acquire 系列分别来自 JsonView / JsonAccessMixin，
//     因此 JsonParser 与 JsonView 的接口完全对称，实现只有一份。
template <typename Json, typename ErrorHandler = DefaultJsonParserErrorHandler>
class JsonParser
    : public JsonAccessMixin<JsonParser<Json, ErrorHandler>, Json,
                             ErrorHandler>,
      public JsonTraitsMixin<JsonParser<Json, ErrorHandler>, Json> {
 public:
  JsonParser(const JsonParser&) = delete;
  JsonParser(JsonParser&&) = delete;
  JsonParser& operator=(const JsonParser&) = delete;
  JsonParser& operator=(JsonParser&&) = delete;

  explicit JsonParser(std::string_view json, ErrorHandler error_handler = {})
      : error_handler_(std::move(error_handler)),
        builder_(json, error_handler_),
        root_(builder_.json()) {}

  explicit JsonParser(const Json& root, ErrorHandler handler = {})
      : error_handler_(std::move(handler)), root_(root) {}

  const Json& json() const { return root_; }

  // ---- View 系列：无状态只读，不修改 parser，可并发、可交叉嵌套 ----
  // 策略语义与 Acquire 一致：Optional（默认）失败返回空 view/range；
  // Required 时路径缺失走 OnPathResolveError、类型不符走 OnTypeMismatch，
  // handler 返回后同样返回空 view/range。
  template <typename ExistencePolicy = Optional, typename Path>
    requires PathLike<Path>
  JsonView<Json, ErrorHandler> View(const Path& path) {
    return root_view().template View<ExistencePolicy>(path);
  }

  template <typename ExistencePolicy = Optional, typename Path>
    requires PathLike<Path>
  JsonArrayView<Json, ErrorHandler> ViewArray(const Path& path) {
    return root_view().template ViewArray<ExistencePolicy>(path);
  }

  template <typename ExistencePolicy = Optional, typename Path>
    requires PathLike<Path>
  JsonObjectView<Json, ErrorHandler> ViewObject(const Path& path) {
    return root_view().template ViewObject<ExistencePolicy>(path);
  }

 private:
  friend class JsonAccessMixin<JsonParser, Json, ErrorHandler>;
  friend class JsonTraitsMixin<JsonParser, Json>;

  JsonView<Json, ErrorHandler> root_view() noexcept {
    return JsonView<Json, ErrorHandler>(root_, error_handler_);
  }

  ErrorHandler& error_handler() noexcept { return error_handler_; }

  const Json* node() const noexcept { return &root_; }

  ErrorHandler error_handler_;
  JsonBuilder<Json, ErrorHandler> builder_;
  const Json& root_;
};

}  // namespace sc::json

#endif  // SCCOMMON_JSON_PARSER_N_H
