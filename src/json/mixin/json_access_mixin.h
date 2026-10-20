//
// Created by jiangyuyun3 on 2026/9/14.
//

#ifndef SCCOMMON_JSON_ACCESS_MIXIN_H
#define SCCOMMON_JSON_ACCESS_MIXIN_H

#include <cstddef>
#include <string>
#include <type_traits>
#include <utility>

#include "../../reflect/pfr.hpp"
#include "../json_path.h"
#include "../util/json_detail.h"
#include "../util/json_error.h"
#include "../util/json_read_write.h"

namespace sc::json {

template <typename Derived, typename Json,
          JsonErrorHandler E = DefaultJsonErrorHandler>
class JsonAccessMixin {
 public:
  template <typename T, AccessPolicy Policy = Optional, std::size_t N>
  Derived& Put(const Parts<N>& path, T& out,
               std::type_identity_t<T> default_val = {}) {
    return FillValue<Policy, Extractor<Json, T>>(path, out,
                                                 std::move(default_val));
  }

  template <typename T, AccessPolicy Policy = Optional, std::size_t N>
  [[nodiscard]] T Get(const Parts<N>& path) {
    T out{};
    Put<T, Policy>(path, out, T{});
    return out;
  }

  template <typename T, AccessPolicy Policy = Optional, std::size_t N>
  [[nodiscard]] T Get(const Parts<N>& path, T default_val) {
    T out{};
    Put<T, Policy>(path, out, std::move(default_val));
    return out;
  }

  template <typename T, template <typename...> class List, std::size_t N>
  Derived& PutArray(const Parts<N>& path, List<T>& arr) {
    return FillArray<Optional, Extractor<Json, T>>(path, arr);
  }

  template <typename T, AccessPolicy Policy, template <typename...> class List,
            std::size_t N>
  Derived& PutArray(const Parts<N>& path, List<T>& arr) {
    return FillArray<Policy, Extractor<Json, T>>(path, arr);
  }

  template <typename R, AccessPolicy Policy = Optional, std::size_t N>
  [[nodiscard]] R GetArray(const Parts<N>& path) {
    static_assert(
        requires { typename R::value_type; },
        "JsonAccessMixin: GetArray<R> 的 R 必须是容器类型，"
        "如 std::vector<std::int64_t>");
    R out{};
    FillArray<Policy, Extractor<Json, typename R::value_type>>(path, out);
    return out;
  }

  template <typename T, template <typename...> class Map, std::size_t N>
  Derived& PutMap(const Parts<N>& path, Map<std::string, T>& map) {
    return FillMap<Optional, Extractor<Json, T>>(path, map);
  }

  template <typename T, AccessPolicy Policy, template <typename...> class Map,
            std::size_t N>
  Derived& PutMap(const Parts<N>& path, Map<std::string, T>& map) {
    return FillMap<Policy, Extractor<Json, T>>(path, map);
  }

  template <typename R, AccessPolicy Policy = Optional, std::size_t N>
  [[nodiscard]] R GetMap(const Parts<N>& path) {
    static_assert(
        requires { typename R::mapped_type; },
        "JsonAccessMixin: GetMap<R> 的 R 必须是 map 类型，"
        "如 std::map<std::string, std::int64_t>");
    R out{};
    FillMap<Policy, Extractor<Json, typename R::mapped_type>>(path, out);
    return out;
  }

  // 将目标路径上的json解析为类型
  // 支持: 1)已注册的可聚合结构体/类; 2) 线性容器; 3) 表容器; 4)
  // 所有Json支持的基本类型
  //      对于第1类，Policy会被传递进每一个字段
  // 无论字段解析成功与否，总是会解析所有字段
  template <typename T, AccessPolicy Policy = Optional, std::size_t N>
  void PutType(const Parts<N>& path, T& t) {
    const auto view = self().template View<Policy>(path);
    if (!view.valid()) {
      return;
    }
    LoadType<Policy>(*view.node(), t);
  }

  template <typename T, AccessPolicy Policy = Optional, std::size_t N>
  [[nodiscard]] T GetType(const Parts<N>& path) {
    T out{};
    PutType<T, Policy>(path, out);
    return out;
  }

 private:
  Derived& self() noexcept { return static_cast<Derived&>(*this); }

  template <AccessPolicy Policy, typename Ext, std::size_t N, typename T>
  Derived& FillValue(const Parts<N>& path, T& out, T default_val) {
    static_assert(
        !std::is_same_v<T, Required> && !std::is_same_v<T, Optional>,
        "JsonAccessMixin: 第一个模板参数是数据类型，策略在第二个，"
        "如 Put<int, Required>(path, out) / Get<int, Required>(path)");
    const Json* base = self().node();
    if (base == nullptr) {
      out = std::move(default_val);
      return self();
    }
    const Json* target = nullptr;
    if constexpr (N == 0) {
      target = base;
    } else if constexpr (N == 1) {
      target = JsonTraits<Json>::TryGetChild(*base, path[0]);
      if constexpr (std::is_same_v<Policy, Required>) {
        if (target == nullptr) {
          self().error_handler().OnPathResolveError(detail::PathString(path));
        }
      }
    } else {
      target = detail::ResolveWithPolicy<Json, N, Policy>(
          base, path, self().error_handler());
    }
    if (target == nullptr) {
      out = std::move(default_val);
      return self();
    }
    if (!Ext::Extract(*target, out)) {
      if constexpr (std::is_same_v<Policy, Required>) {
        self().error_handler().OnTypeMismatch(detail::PathString(path));
      }
      out = std::move(default_val);
    }
    return self();
  }

  // 容器以「完整类型」作参数，内部元素类型取自 Container::value_type，
  // 不再需要推导模板模板参数（MSVC 更稳）。
  template <AccessPolicy Policy, typename Ext, typename Container,
            std::size_t N>
  Derived& FillArray(const Parts<N>& path, Container& arr) {
    using T = Container::value_type;
    arr.clear();
    const auto range = self().template ViewArray<Policy>(path);
    const Json* array = range.node();
    if (array == nullptr) {
      return self();
    }
    if constexpr (requires { arr.reserve(std::size_t{}); }) {
      arr.reserve(range.size());
    }
    const std::size_t size = range.size();
    for (std::size_t i = 0; i < size; ++i) {
      const Json& element = JsonTraits<Json>::GetArrayElement(*array, i);
      T value{};
      if (!Ext::Extract(element, value)) {
        std::string element_path = detail::PathString(path);
        element_path += '[';
        detail::AppendNumber(element_path, i);
        element_path += ']';
        self().error_handler().OnTypeMismatch(element_path);
        return self();  // 若 E 返回，则中止填充
      }
      arr.push_back(std::move(value));
    }
    return self();
  }

  template <AccessPolicy Policy, typename Ext, typename Container,
            std::size_t N>
  Derived& FillMap(const Parts<N>& path, Container& map) {
    using T = Container::mapped_type;
    map.clear();
    const auto range = self().template ViewObject<Policy>(path);
    const Json* object = range.node();
    if (object == nullptr) {
      return self();
    }
    for (auto it = JsonTraits<Json>::MemberBegin(*object);
         it != JsonTraits<Json>::MemberEnd(*object); ++it) {
      const auto member = *it;
      T value{};
      if (!Ext::Extract(*member.second, value)) {
        std::string member_path = detail::PathString(path);
        if (!member_path.empty()) {
          member_path += '.';
        }
        member_path += member.first;
        self().error_handler().OnTypeMismatch(member_path);
        return self();  // 若 E 返回，则中止填充
      }
      map.emplace(std::string(member.first), std::move(value));
    }
    return self();
  }

  template <AccessPolicy Policy, typename T>
  bool LoadType(const Json& node, T& out) {
    if constexpr (reflect::reflectable_v<T>) {
      std::apply(
          [&](const auto&... field) {
            (LoadField<Policy>(node, field, out), ...);
          },
          reflect::Descriptor<T>::fields());
      return true;
    } else if constexpr (detail::is_sequence_v<T>) {
      if (!JsonTraits<Json>::IsArray(node)) {
        return false;
      }
      const std::size_t size = JsonTraits<Json>::GetArraySize(node);
      out.clear();
      if constexpr (requires { out.reserve(std::size_t{}); }) {
        out.reserve(size);
      }
      for (std::size_t i = 0; i < size; ++i) {
        using Element = T::value_type;
        Element element{};
        if (!LoadType<Policy>(JsonTraits<Json>::GetArrayElement(node, i),
                              element)) {
          return false;
        }
        out.push_back(std::move(element));
      }
      return true;
    } else if constexpr (detail::is_map_v<T>) {
      if (!JsonTraits<Json>::IsObject(node)) {
        return false;
      }
      out.clear();
      for (auto it = JsonTraits<Json>::MemberBegin(node);
           it != JsonTraits<Json>::MemberEnd(node); ++it) {
        const auto member = *it;
        using Mapped = T::mapped_type;
        Mapped value{};
        if (!LoadType<Policy>(*member.second, value)) {
          return false;
        }
        out.emplace(std::string(member.first), std::move(value));
      }
      return true;
    } else {
      return Extractor<Json, T>::Extract(node, out);
    }
  }

  template <AccessPolicy Policy, typename FieldT, typename T>
  void LoadField(const Json& node, const FieldT& field, T& out) {
    const Json* child = JsonTraits<Json>::TryGetChild(node, field.name);
    if (!child) {
      if constexpr (std::is_same_v<Policy, Required>) {
        self().error_handler().OnPathResolveError(field.name);
      }
      return;
    }
    if (!LoadType<Policy>(*child, field.Get(out))) {
      self().error_handler().OnTypeMismatch(field.name);
    }
  }
};

}  // namespace sc::json

#endif  // SCCOMMON_JSON_ACCESS_MIXIN_H
