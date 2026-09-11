//
// Created by jiangyuyun3 on 2026/9/11.
//

#ifndef SCCOMMON_JSON_VIEW_JSON_VIEW_H
#define SCCOMMON_JSON_VIEW_JSON_VIEW_H

#include "../json_detail.h"
#include "../json_error.h"
#include "../json_path.h"

#include <cstddef>
#include <stdexcept>
#include <string_view>
#include <type_traits>

namespace sc::json {

template <typename Json, typename ErrorHandler>
class JsonArrayView;

template <typename Json, typename ErrorHandler>
class JsonObjectView;

// 对单个 JSON 节点的轻量引用：2 个指针（节点 + 错误处理器），可拷贝、可移动、可判空。
// 不持有 Json 所有权，生命周期由底层 JSON 数据保证。
template <typename Json, typename ErrorHandler = DefaultJsonParserErrorHandler>
class JsonView {
public:
    JsonView() noexcept = default;  // 无效 view
    JsonView(const Json& node, ErrorHandler& handler) noexcept : node_(&node), handler_(&handler) {}

    [[nodiscard]] bool valid() const noexcept { return node_ != nullptr; }
    explicit operator bool() const noexcept { return valid(); }
    const Json& json() const { return *node_; }  // 前置条件：valid()

    // 语义与 JsonParser::Acquire 一致，相对当前节点解析。
    // 无效 view 上的 Acquire 等同 Optional：写默认值，不触发错误回调。
    template <template <typename> class Extractor, typename ExistencePolicy = Optional, std::size_t n>
    Extractor<Json>::ValueType Acquire(const Parts<n>& path, Extractor<Json>::ValueType def = {}) {
        static_assert(std::is_same_v<ExistencePolicy, Required> ||
                              std::is_same_v<ExistencePolicy, Optional>,
                "JsonView: unsupported ExistencePolicy");
        if (!valid()) {
            return def;
        }
        const Json* node = Find<ExistencePolicy>(path);
        if (!node) {
            return def;
        }
        using Val = Extractor<Json>::ValueType;
        Val value{};
        if (!Extractor<Json>::Extract(*node, value)) {
            if constexpr (std::is_same_v<ExistencePolicy, Required>) {
                handler_->OnTypeMismatch(detail::PathString(path));
            }
            return def;
        }
        return value;
    }

    // 派生 view。无效 view 上无论 Policy 都返回空 view。
    template <typename ExistencePolicy = Optional, std::size_t n>
    JsonView View(const Parts<n>& path) const {
        static_assert(std::is_same_v<ExistencePolicy, Required> ||
                              std::is_same_v<ExistencePolicy, Optional>,
                "JsonView: unsupported ExistencePolicy");
        if (!valid()) {
            return JsonView{};
        }
        const Json* node = Find<ExistencePolicy>(path);
        return node ? JsonView{*node, *handler_} : JsonView{};
    }

    template <typename ExistencePolicy = Optional, std::size_t n>
    JsonArrayView<Json, ErrorHandler> ViewArray(const Parts<n>& path) const;

    template <typename ExistencePolicy = Optional, std::size_t n>
    JsonObjectView<Json, ErrorHandler> ViewObject(const Parts<n>& path) const;

private:
    // 从当前节点开始解析 path；Required 且解析失败时交给 OnPathResolveError。
    template <typename ExistencePolicy, std::size_t n>
    const Json* Find(const Parts<n>& path) const {
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
