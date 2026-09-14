//
// Created by jiangyuyun3 on 2026/9/14.
//

#ifndef SCCOMMON_JSON_ACCESS_MIXIN_H
#define SCCOMMON_JSON_ACCESS_MIXIN_H

#include <cstddef>
#include <string>
#include <type_traits>
#include <utility>

#include "json_extractor.h"
#include "json_path.h"
#include "util/json_detail.h"
#include "util/json_error.h"

namespace sc::json {

template <typename Derived, typename Json>
class JsonAccessMixin {
 public:
  // 变量提取：必须显式指定提取器，如 Acquire<JsonInt, Required>(path, out)
  template <template <typename> class Extractor,
            typename ExistencePolicy = Optional, typename T, typename Path>
    requires PathLike<Path>
  Derived& Acquire(const Path& path, T& out, T default_val = {}) {
    static_assert(std::is_same_v<ExistencePolicy, Required> ||
                      std::is_same_v<ExistencePolicy, Optional>,
                  "JsonAccessMixin: unsupported ExistencePolicy");
    static_assert(
        std::is_same_v<T, typename Extractor<Json>::ValueType>,
        "JsonAccessMixin: out type must match Extractor<Json>::ValueType");
    return AcquireWithExtractor<ExistencePolicy, Extractor<Json>>(
        path, out, std::move(default_val));
  }

  // 变量提取：由 out 类型推导提取器
  template <typename ExistencePolicy = Optional, typename T, typename Path>
    requires PathLike<Path>
  Derived& Acquire(const Path& path, T& out, T default_val = {}) {
    static_assert(std::is_same_v<ExistencePolicy, Required> ||
                      std::is_same_v<ExistencePolicy, Optional>,
                  "JsonAccessMixin: unsupported ExistencePolicy");
    return AcquireWithExtractor<ExistencePolicy, Extractor<Json, T>>(
        path, out, std::move(default_val));
  }

  // 将json数组提取到列表容器，需要显式指定提取器
  template <template <typename> class Extractor,
            typename ExistencePolicy = Optional,
            template <typename...> class List, typename T, typename Path>
    requires PathLike<Path>
  Derived& AcquireArray(const Path& path, List<T>& arr) {
    static_assert(std::is_same_v<ExistencePolicy, Required> ||
                      std::is_same_v<ExistencePolicy, Optional>,
                  "JsonAccessMixin: unsupported ExistencePolicy");
    static_assert(std::is_same_v<T, typename Extractor<Json>::ValueType>,
                  "JsonAccessMixin: AcquireArray element type must match "
                  "Extractor<Json>::ValueType");
    return AcquireArrayWithExtractor<ExistencePolicy, Extractor<Json>>(path, arr);
  }

  // 将json数组提取到列表容器，由列表推导提取器
  template <typename ExistencePolicy = Optional,
            template <typename...> class List, typename T, typename Path>
    requires PathLike<Path>
  Derived& AcquireArray(const Path& path, List<T>& arr) {
    static_assert(std::is_same_v<ExistencePolicy, Required> ||
                      std::is_same_v<ExistencePolicy, Optional>,
                  "JsonAccessMixin: unsupported ExistencePolicy");
    return AcquireArrayWithExtractor<ExistencePolicy, Extractor<Json, T>>(path, arr);
  }

  // 将json提取到Map容器，需要显式指定提取器
  template <template <typename> class Extractor,
            typename ExistencePolicy = Optional,
            template <typename...> class Map, typename T, typename Path>
    requires PathLike<Path>
  Derived& AcquireMap(const Path& path, Map<std::string, T>& map) {
    static_assert(std::is_same_v<ExistencePolicy, Required> ||
                      std::is_same_v<ExistencePolicy, Optional>,
                  "JsonAccessMixin: unsupported ExistencePolicy");
    static_assert(std::is_same_v<T, typename Extractor<Json>::ValueType>,
                  "JsonAccessMixin: AcquireMap value type must match "
                  "Extractor<Json>::ValueType");
    return AcquireMapWithExtractor<ExistencePolicy, Extractor<Json>>(path, map);
  }

  // 将json提取到Map容器，由 map 值类型推导提取器
  template <typename ExistencePolicy = Optional,
            template <typename...> class Map, typename T, typename Path>
    requires PathLike<Path>
  Derived& AcquireMap(const Path& path, Map<std::string, T>& map) {
    static_assert(std::is_same_v<ExistencePolicy, Required> ||
                      std::is_same_v<ExistencePolicy, Optional>,
                  "JsonAccessMixin: unsupported ExistencePolicy");
    return AcquireMapWithExtractor<ExistencePolicy, Extractor<Json, T>>(path, map);
  }

 private:
  // 获取子类引用
  Derived& self() noexcept { return static_cast<Derived&>(*this); }

  template <typename ExistencePolicy, typename Ext, typename Path, typename T>
  Derived& AcquireWithExtractor(const Path& path, T& out, T default_val) {
    const auto view = self().template View<ExistencePolicy>(path);
    if (!view.valid()) {
      out = std::move(default_val);
      return self();
    }
    if (T value{}; Ext::Extract(view.json(), value)) {
      out = std::move(value);
    } else {
      if constexpr (std::is_same_v<ExistencePolicy, Required>) {
        self().error_handler().OnTypeMismatch(detail::PathString(path));
      }
      out = std::move(default_val);
    }
    return self();
  }

  template <typename ExistencePolicy, typename Ext,
            template <typename...> class List, typename T, typename Path>
  Derived& AcquireArrayWithExtractor(const Path& path, List<T>& arr) {
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

  template <typename ExistencePolicy, typename Ext,
            template <typename...> class Map, typename T, typename Path>
  Derived& AcquireMapWithExtractor(const Path& path, Map<std::string, T>& map) {
    map.clear();
    for (auto&& [key, value_view] :
         self().template ViewObject<ExistencePolicy>(path)) {
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
};

}  // namespace sc::json

#endif  // SCCOMMON_JSON_ACCESS_MIXIN_H
