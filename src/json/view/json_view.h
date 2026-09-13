//
// Created by jiangyuyun3 on 2026/9/11.
//

#ifndef SCCOMMON_JSON_VIEW_JSON_VIEW_H
#define SCCOMMON_JSON_VIEW_JSON_VIEW_H

#include <cstddef>
#include <stdexcept>
#include <string_view>
#include <type_traits>

#include "../json_access_mixin.h"
#include "../json_detail.h"
#include "../json_error.h"
#include "../json_path.h"
#include "../json_traits_mixin.h"

namespace sc::json {

template <typename Json, typename ErrorHandler>
class JsonArrayView;

template <typename Json, typename ErrorHandler>
class JsonObjectView;

// 对单个 JSON 节点的轻量引用：2 个指针（节点 +
// 错误处理器），可拷贝、可移动、可判空。 不持有 Json 所有权，生命周期由底层
// JSON 数据保证。
template <typename Json, typename ErrorHandler = DefaultJsonParserErrorHandler>
class JsonView
    : public JsonAccessMixin<JsonView<Json, ErrorHandler>, Json, ErrorHandler>,
      public JsonTraitsMixin<JsonView<Json, ErrorHandler>, Json> {
 public:
  JsonView() noexcept = default;  // 无效 view
  JsonView(const Json& node, ErrorHandler& handler) noexcept
      : node_(&node), handler_(&handler) {}

  [[nodiscard]] bool valid() const noexcept { return node_ != nullptr; }
  explicit operator bool() const noexcept { return valid(); }
  const Json& json() const { return *node_; }  // 前置条件：valid()

  // Acquire / AcquireArray / AcquireMap 由 JsonAccessMixin 提供（与 JsonParser
  // 同一份实现）。 语义：相对当前节点解析；无效 view 上的 Acquire 等同
  // Optional，返回默认值且不触发错误回调。

  // 派生 view。无效 view 上无论 Policy 都返回空 view。
  template <typename ExistencePolicy = Optional, typename Path>
    requires PathLike<Path>
  JsonView View(const Path& path) const {
    static_assert(std::is_same_v<ExistencePolicy, Required> ||
                      std::is_same_v<ExistencePolicy, Optional>,
                  "JsonView: unsupported ExistencePolicy");
    if (!valid()) {
      return JsonView{};
    }
    const Json* node = Find<ExistencePolicy>(path);
    return node ? JsonView{*node, *handler_} : JsonView{};
  }

  template <typename ExistencePolicy = Optional, typename Path>
    requires PathLike<Path>
  JsonArrayView<Json, ErrorHandler> ViewArray(const Path& path) const;

  template <typename ExistencePolicy = Optional, typename Path>
    requires PathLike<Path>
  JsonObjectView<Json, ErrorHandler> ViewObject(const Path& path) const;

 private:
  friend class JsonAccessMixin<JsonView, Json, ErrorHandler>;
  friend class JsonTraitsMixin<JsonView, Json>;

  ErrorHandler& error_handler() const noexcept { return *handler_; }

  const Json* node() const noexcept { return node_; }

  // 从当前节点开始解析 path；Required 且解析失败时交给 OnPathResolveError。
  template <typename ExistencePolicy, typename Path>
    requires PathLike<Path>
  const Json* Find(const Path& path) const {
    const Json* cur = node_;
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
        std::runtime_error e(detail::ResolveErrorMessage(path, actual, failed));
        handler_->OnPathResolveError(e);
      }
    }
    return cur;
  }

  const Json* node_ = nullptr;
  ErrorHandler* handler_ = nullptr;
};

}  // namespace sc::json

#endif  // SCCOMMON_JSON_VIEW_JSON_VIEW_H
