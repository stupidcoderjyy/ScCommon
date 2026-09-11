//
// Created by jiangyuyun3 on 2026/9/11.
//

#ifndef SCCOMMON_JSON_ACCESS_MIXIN_H
#define SCCOMMON_JSON_ACCESS_MIXIN_H

#include "json_detail.h"
#include "json_error.h"
#include "json_extractor.h"
#include "json_path.h"

#include <cstddef>
#include <string>
#include <type_traits>
#include <utility>

namespace sc::json {

// Acquire / AcquireArray / AcquireMap 的唯一实现（CRTP mixin）。
// JsonParser 与 JsonView 都继承它，因此两边接口完全对称，实现只有一份。
//
// 每个取值接口有两种写法：
//   1) 显式提取器：Acquire<JsonInt, Required>(path, out)
//   2) 由 out 类型推导：Acquire(path, out)
//      写法 2 没有显式模板实参，因此在泛型 lambda / 模板里**不需要写 .template**。
//
// 路径参数满足 PathLike：编译期路径 Parts<N>（"_path" 字面量）与运行期路径 RuntimePath
// 都可以；运行期路径没有编译期校验，非法路径由 OnPathResolveError 在解析时报告。
//
// Derived 需要提供（均以“当前节点”为起点解析）：
//   template <typename Policy, typename Path> auto View(const Path&) const;
//   template <typename Policy, typename Path> auto ViewArray(const Path&) const;
//   template <typename Policy, typename Path> auto ViewObject(const Path&) const;
//   ErrorHandler& error_handler();        // 供元素级错误回调使用（可私有 + friend）
template <typename Derived, typename Json, typename ErrorHandler>
class JsonAccessMixin {
public:
    // ---------------- Acquire ----------------
    // 写法 1：显式提取器
    template <template <typename> class Extractor,
            typename ExistencePolicy = Optional,
            typename T,
            typename Path>
        requires PathLike<Path>
    Derived& Acquire(const Path& path, T& out, T default_val = {}) {
        static_assert(std::is_same_v<ExistencePolicy, Required> ||
                              std::is_same_v<ExistencePolicy, Optional>,
                "JsonAccessMixin: unsupported ExistencePolicy");
        static_assert(std::is_same_v<T, typename Extractor<Json>::ValueType>,
                "JsonAccessMixin: out type must match Extractor<Json>::ValueType");
        return AcquireWith<ExistencePolicy, Extractor<Json>>(path, out, std::move(default_val));
    }

    // 写法 2：由 out 类型推导提取器（Extractor<Json, T>），无需显式模板实参
    template <typename ExistencePolicy = Optional, typename T, typename Path>
        requires PathLike<Path>
    Derived& Acquire(const Path& path, T& out, T default_val = {}) {
        static_assert(std::is_same_v<ExistencePolicy, Required> ||
                              std::is_same_v<ExistencePolicy, Optional>,
                "JsonAccessMixin: unsupported ExistencePolicy");
        return AcquireWith<ExistencePolicy, Extractor<Json, T>>(path, out, std::move(default_val));
    }

    // ---------------- AcquireArray ----------------
    // 写法 1：显式提取器；容器类型由 arr 推导
    template <template <typename> class Extractor,
            typename ExistencePolicy = Optional,
            template <typename...> class List,
            typename T,
            typename Path>
        requires PathLike<Path>
    Derived& AcquireArray(const Path& path, List<T>& arr) {
        static_assert(std::is_same_v<ExistencePolicy, Required> ||
                              std::is_same_v<ExistencePolicy, Optional>,
                "JsonAccessMixin: unsupported ExistencePolicy");
        static_assert(std::is_same_v<T, typename Extractor<Json>::ValueType>,
                "JsonAccessMixin: AcquireArray element type must match Extractor<Json>::ValueType");
        return AcquireArrayWith<ExistencePolicy, Extractor<Json>>(path, arr);
    }

    // 写法 2：由 arr 元素类型推导提取器
    template <typename ExistencePolicy = Optional,
            template <typename...> class List,
            typename T,
            typename Path>
        requires PathLike<Path>
    Derived& AcquireArray(const Path& path, List<T>& arr) {
        static_assert(std::is_same_v<ExistencePolicy, Required> ||
                              std::is_same_v<ExistencePolicy, Optional>,
                "JsonAccessMixin: unsupported ExistencePolicy");
        return AcquireArrayWith<ExistencePolicy, Extractor<Json, T>>(path, arr);
    }

    // ---------------- AcquireMap ----------------
    // 写法 1：显式提取器；容器类型由 map 推导
    template <template <typename> class Extractor,
            typename ExistencePolicy = Optional,
            template <typename...> class Map,
            typename T,
            typename Path>
        requires PathLike<Path>
    Derived& AcquireMap(const Path& path, Map<std::string, T>& map) {
        static_assert(std::is_same_v<ExistencePolicy, Required> ||
                              std::is_same_v<ExistencePolicy, Optional>,
                "JsonAccessMixin: unsupported ExistencePolicy");
        static_assert(std::is_same_v<T, typename Extractor<Json>::ValueType>,
                "JsonAccessMixin: AcquireMap value type must match Extractor<Json>::ValueType");
        return AcquireMapWith<ExistencePolicy, Extractor<Json>>(path, map);
    }

    // 写法 2：由 map 值类型推导提取器
    template <typename ExistencePolicy = Optional,
            template <typename...> class Map,
            typename T,
            typename Path>
        requires PathLike<Path>
    Derived& AcquireMap(const Path& path, Map<std::string, T>& map) {
        static_assert(std::is_same_v<ExistencePolicy, Required> ||
                              std::is_same_v<ExistencePolicy, Optional>,
                "JsonAccessMixin: unsupported ExistencePolicy");
        return AcquireMapWith<ExistencePolicy, Extractor<Json, T>>(path, map);
    }

private:
    template <typename ExistencePolicy, typename Ext, typename Path, typename T>
    Derived& AcquireWith(const Path& path, T& out, T default_val) {
        const auto view = self().template View<ExistencePolicy>(path);
        if (!view.valid()) {
            out = std::move(default_val);
            return self();
        }
        T value{};
        if (!Ext::Extract(view.json(), value)) {
            if constexpr (std::is_same_v<ExistencePolicy, Required>) {
                self().error_handler().OnTypeMismatch(detail::PathString(path));
            }
            out = std::move(default_val);
            return self();
        }
        out = std::move(value);
        return self();
    }

    // ExistencePolicy 只作用于数组本身；元素类型不符始终走 OnTypeMismatch（完整路径，如 a.b[2]）
    template <typename ExistencePolicy,
            typename Ext,
            template <typename...> class List,
            typename T,
            typename Path>
    Derived& AcquireArrayWith(const Path& path, List<T>& arr) {
        arr.clear();
        auto range = self().template ViewArray<ExistencePolicy>(path);
        if constexpr (requires { arr.reserve(std::size_t{}); }) {
            arr.reserve(range.size());
        }
        std::size_t i = 0;
        for (auto&& element : range) {
            T value{};
            if (!Ext::Extract(element.json(), value)) {
                std::string element_path = detail::PathString(path);
                element_path += '[';
                detail::AppendNumber(element_path, i);
                element_path += ']';
                self().error_handler().OnTypeMismatch(element_path);
                return self();  // 若 ErrorHandler 返回，则中止填充
            }
            arr.push_back(std::move(value));
            ++i;
        }
        return self();
    }

    // 元素路径组装为 <path>.<key>（如 a.b.c）
    template <typename ExistencePolicy,
            typename Ext,
            template <typename...> class Map,
            typename T,
            typename Path>
    Derived& AcquireMapWith(const Path& path, Map<std::string, T>& map) {
        map.clear();
        for (auto&& [key, value_view] : self().template ViewObject<ExistencePolicy>(path)) {
            T value{};
            if (!Ext::Extract(value_view.json(), value)) {
                std::string member_path = detail::PathString(path);
                if (!member_path.empty()) {
                    member_path += '.';
                }
                member_path += key;
                self().error_handler().OnTypeMismatch(member_path);
                return self();  // 若 ErrorHandler 返回，则中止填充
            }
            map.emplace(std::string(key), std::move(value));
        }
        return self();
    }

    Derived& self() noexcept { return static_cast<Derived&>(*this); }
};

}  // namespace sc::json

#endif  // SCCOMMON_JSON_ACCESS_MIXIN_H
