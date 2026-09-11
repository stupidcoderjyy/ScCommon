//
// Created by jiangyuyun3 on 2026/9/7.
//

#ifndef SCCOMMON_JSON_PARSER_N_H
#define SCCOMMON_JSON_PARSER_N_H

#include "json_detail.h"  //NOLINT
#include "json_error.h"
#include "json_extractor.h"  //NOLINT
#include "json_path.h"
#include "json_traits.h"
#include "view/json_views.h"

#include <cstddef>
#include <map>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

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

template <typename Json, typename ErrorHandler = DefaultJsonParserErrorHandler>
class JsonParser {
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

    // ---- View 系列：无状态只读，不修改 parser，可并发/可交叉嵌套 ----
    // 策略语义与 Acquire 一致：Optional（默认）失败返回空 view/range；
    // Required 时路径缺失走 OnPathResolveError、类型不符走 OnTypeMismatch，
    // handler 返回后同样返回空 view/range。
    template <typename ExistencePolicy = Optional, std::size_t n>
    JsonView<Json, ErrorHandler> View(const Parts<n>& path) {
        static_assert(std::is_same_v<ExistencePolicy, Required> ||
                              std::is_same_v<ExistencePolicy, Optional>,
                "JsonParser: unsupported ExistencePolicy");
        std::size_t actual = 0;
        const Json* node = Resolve<ExistencePolicy>(path, actual);
        if (!node) {
            return JsonView<Json, ErrorHandler>{};
        }
        return JsonView<Json, ErrorHandler>(*node, error_handler_);
    }

    template <typename ExistencePolicy = Optional, std::size_t n>
    JsonArrayView<Json, ErrorHandler> ViewArray(const Parts<n>& path) {
        static_assert(std::is_same_v<ExistencePolicy, Required> ||
                              std::is_same_v<ExistencePolicy, Optional>,
                "JsonParser: unsupported ExistencePolicy");
        std::size_t actual = 0;
        const Json* node = Resolve<ExistencePolicy>(path, actual);
        if (!node) {
            return JsonArrayView<Json, ErrorHandler>::Empty();
        }
        if (!JsonTraits<Json>::IsArray(*node)) {
            if constexpr (std::is_same_v<ExistencePolicy, Required>) {
                error_handler_.OnTypeMismatch(detail::PathString(path));
            }
            return JsonArrayView<Json, ErrorHandler>::Empty();
        }
        return JsonArrayView<Json, ErrorHandler>(*node, error_handler_);
    }

    template <typename ExistencePolicy = Optional, std::size_t n>
    JsonObjectView<Json, ErrorHandler> ViewObject(const Parts<n>& path) {
        static_assert(std::is_same_v<ExistencePolicy, Required> ||
                              std::is_same_v<ExistencePolicy, Optional>,
                "JsonParser: unsupported ExistencePolicy");
        std::size_t actual = 0;
        const Json* node = Resolve<ExistencePolicy>(path, actual);
        if (!node) {
            return JsonObjectView<Json, ErrorHandler>::Empty();
        }
        if (!JsonTraits<Json>::IsObject(*node)) {
            if constexpr (std::is_same_v<ExistencePolicy, Required>) {
                error_handler_.OnTypeMismatch(detail::PathString(path));
            }
            return JsonObjectView<Json, ErrorHandler>::Empty();
        }
        return JsonObjectView<Json, ErrorHandler>(*node, error_handler_);
    }

    template <template <typename J> class Extractor,
            typename ExistencePolicy = Optional,
            std::size_t n>
    Extractor<Json>::ValueType Acquire(
            const Parts<n>& path, Extractor<Json>::ValueType default_val = {}) {
        static_assert(std::is_same_v<ExistencePolicy, Required> ||
                              std::is_same_v<ExistencePolicy, Optional>,
                "JsonParser: unsupported ExistencePolicy");
        using Val = Extractor<Json>::ValueType;
        std::size_t actual = 0;
        const Json* node = Resolve<ExistencePolicy>(path, actual);
        if (!node) {
            // Required：Resolve 内已交给 OnPathResolveError（handler 返回时给默认值）
            return default_val;
        }
        Val value{};
        if (!Extractor<Json>::Extract(*node, value)) {
            if constexpr (std::is_same_v<ExistencePolicy, Required>) {
                error_handler_.OnTypeMismatch(detail::PathString(path));
            }
            return default_val;
        }
        return value;
    }

    template <template <typename J> class Extractor,
            typename ExistencePolicy = Optional,
            template <typename...> class List = std::vector,
            std::size_t n>
    List<typename Extractor<Json>::ValueType> AcquireArray(const Parts<n>& path) {
        static_assert(std::is_same_v<ExistencePolicy, Required> ||
                              std::is_same_v<ExistencePolicy, Optional>,
                "JsonParser: unsupported ExistencePolicy");
        using T = Extractor<Json>::ValueType;
        // 把一个json数组填入arr
        // ExistencePolicy只针对数组
        // 内部元素如果出现类型不匹配的情况, 直接调用OnTypeMismatch（需要组装完整路径，如 a.b[2]）
        List<T> arr;
        auto range = ViewArray<ExistencePolicy>(path);  // 缺失 / 类型不符由 view 层按策略处理
        if constexpr (requires { arr.reserve(std::size_t{}); }) {
            arr.reserve(range.size());
        }
        std::size_t i = 0;
        for (auto&& element : range) {
            T value{};
            if (!Extractor<Json>::Extract(element.json(), value)) {
                std::string element_path = detail::PathString(path);
                element_path += '[';
                detail::AppendNumber(element_path, i);
                element_path += ']';
                error_handler_.OnTypeMismatch(element_path);
                return arr;  // 若 ErrorHandler 返回，则返回已填充部分
            }
            arr.push_back(std::move(value));
            ++i;
        }
        return arr;
    }

    template <template <typename J> class Extractor,
            typename ExistencePolicy = Optional,
            template <typename...> class Map = std::map,
            std::size_t n>
    Map<std::string, typename Extractor<Json>::ValueType> AcquireMap(const Parts<n>& path) {
        static_assert(std::is_same_v<ExistencePolicy, Required> ||
                              std::is_same_v<ExistencePolicy, Optional>,
                "JsonParser: unsupported ExistencePolicy");
        using T = Extractor<Json>::ValueType;
        // 把一个json对象填入map
        // ExistencePolicy只针对map
        // 内部元素如果出现类型不匹配的情况, 直接调用OnTypeMismatch（需要组装完整路径，如 a.b.c）
        Map<std::string, T> map;
        for (auto&& [key, value_view] : ViewObject<ExistencePolicy>(path)) {
            T value{};
            if (!Extractor<Json>::Extract(value_view.json(), value)) {
                std::string member_path = detail::PathString(path);
                if (!member_path.empty()) {
                    member_path += '.';
                }
                member_path += key;
                error_handler_.OnTypeMismatch(member_path);
                return map;  // 若 ErrorHandler 返回，则返回已填充部分
            }
            map.emplace(std::string(key), std::move(value));
        }
        return map;
    }

private:
    // 从根节点开始依次取路径段。
    // actual 返回实际解析成功的层数（失败时为已解析的前缀长度）。
    // ExistencePolicy == Required：解析失败时构造异常交给 OnPathResolveError，并返回 nullptr；
    // ExistencePolicy == Optional：解析失败直接返回 nullptr。
    template <typename ExistencePolicy, std::size_t n>
    const Json* Resolve(const Parts<n>& parts, std::size_t& actual) {
        actual = 0;
        const Json* node = &root_;
        for (std::size_t i = 0; i < parts.Size(); ++i) {
            const std::string_view token = parts[i];
            const Json* next = detail::Step<Json>(node, token);
            if (!next) {
                if constexpr (std::is_same_v<ExistencePolicy, Required>) {
                    std::runtime_error e(detail::ResolveErrorMessage(parts, actual, token));
                    error_handler_.OnPathResolveError(e);
                }
                return nullptr;
            }
            node = next;
            ++actual;
        }
        return node;
    }

    ErrorHandler error_handler_;
    JsonBuilder<Json, ErrorHandler> builder_;
    const Json& root_;
};

}  // namespace sc::json

#endif  // SCCOMMON_JSON_PARSER_N_H
