//
// Created by jiangyuyun3 on 2026/9/14.
//

#ifndef SCCOMMON_JSON_ACCESS_MIXIN_H
#define SCCOMMON_JSON_ACCESS_MIXIN_H

#include <cstddef>
#include <string>
#include <type_traits>
#include <utility>

#include "json_path.h"
#include "util/json_detail.h"
#include "util/json_error.h"
#include "util/json_read_write.h"

namespace sc::json {

template <typename Derived, typename Json>
class JsonAccessMixin {
 public:
  // 所有接口的模板参数都是 <类型, 策略 = Optional>：
  // 第一个是「被提取/返回的类型」，第二个才是策略，因此 Get<bool>(path) 可用。

  // ---------------- 写入形式：Put(path, out[, default]) ----------------
  // T 由 out 推导，也可显式写 <类型, 策略>；default_val 是非推导语境，
  // 因此默认值类型可与 out 不同（如 out=int64_t、默认值写 0）。
  template <typename T, typename ExistencePolicy = Optional, std::size_t N>
  Derived& Put(const Parts<N>& path, T& out,
               std::type_identity_t<T> default_val = {}) {
    static_assert(std::is_same_v<ExistencePolicy, Required> ||
                      std::is_same_v<ExistencePolicy, Optional>,
                  "JsonAccessMixin: unsupported ExistencePolicy");
    return FillValue<ExistencePolicy, Extractor<Json, T>>(
        path, out, std::move(default_val));
  }

  // ---------------- 返回值形式：Get(path[, default]) ----------------
  // 返回类型无法推导，T 必须显式；失败时回落 T{}（Optional）或交给 handler
  template <typename T, typename ExistencePolicy = Optional, std::size_t N>
  [[nodiscard]] T Get(const Parts<N>& path) {
    T out{};
    Put<T, ExistencePolicy>(path, out);
    return out;
  }

  // 带默认值：T 可显式，也可由默认值推导（Get(path, kDefault)）
  template <typename T, typename ExistencePolicy = Optional, std::size_t N>
  [[nodiscard]] T Get(const Parts<N>& path, T default_val) {
    T out{};
    Put<T, ExistencePolicy>(path, out, std::move(default_val));
    return out;
  }

  // ---------------- 数组：写入 PutArray / 返回 GetArray ----------------
  // 写入到列表容器；元素类型 T 由容器推导。
  // 策略默认与显式写成两个重载：策略没有默认值就不存在「默认实参夹在中间、
  // 后面还要推导模板模板参数」的组合（MSVC 会在这种形态上报 C2672）。
  template <typename T, template <typename...> class List, std::size_t N>
  Derived& PutArray(const Parts<N>& path, List<T>& arr) {
    return FillArray<Optional, Extractor<Json, T>>(path, arr);
  }

  template <typename T, typename ExistencePolicy,
            template <typename...> class List, std::size_t N>
  Derived& PutArray(const Parts<N>& path, List<T>& arr) {
    static_assert(std::is_same_v<ExistencePolicy, Required> ||
                      std::is_same_v<ExistencePolicy, Optional>,
                  "JsonAccessMixin: unsupported ExistencePolicy");
    return FillArray<ExistencePolicy, Extractor<Json, T>>(path, arr);
  }

  // 返回容器：R 是完整容器类型，如 std::vector<std::int64_t>
  template <typename R, typename ExistencePolicy = Optional, std::size_t N>
  [[nodiscard]] R GetArray(const Parts<N>& path) {
    static_assert(
        requires { typename R::value_type; },
        "JsonAccessMixin: GetArray<R> 的 R 必须是容器类型，"
        "如 std::vector<std::int64_t>");
    R out{};
    FillArray<ExistencePolicy, Extractor<Json, typename R::value_type>>(path,
                                                                        out);
    return out;
  }

  // ---------------- 对象：写入 PutMap / 返回 GetMap ----------------
  // 写入到 Map<std::string, T>；值类型 T 由 map 推导（同 PutArray 拆两个重载）
  template <typename T, template <typename...> class Map, std::size_t N>
  Derived& PutMap(const Parts<N>& path, Map<std::string, T>& map) {
    return FillMap<Optional, Extractor<Json, T>>(path, map);
  }

  template <typename T, typename ExistencePolicy,
            template <typename...> class Map, std::size_t N>
  Derived& PutMap(const Parts<N>& path, Map<std::string, T>& map) {
    static_assert(std::is_same_v<ExistencePolicy, Required> ||
                      std::is_same_v<ExistencePolicy, Optional>,
                  "JsonAccessMixin: unsupported ExistencePolicy");
    return FillMap<ExistencePolicy, Extractor<Json, T>>(path, map);
  }

  // 返回 Map：R 是完整容器类型，如 std::map<std::string, std::int64_t>
  template <typename R, typename ExistencePolicy = Optional, std::size_t N>
  [[nodiscard]] R GetMap(const Parts<N>& path) {
    static_assert(
        requires { typename R::mapped_type; },
        "JsonAccessMixin: GetMap<R> 的 R 必须是 map 类型，"
        "如 std::map<std::string, std::int64_t>");
    R out{};
    FillMap<ExistencePolicy, Extractor<Json, typename R::mapped_type>>(path,
                                                                       out);
    return out;
  }

 private:
  // 获取子类引用
  Derived& self() noexcept { return static_cast<Derived&>(*this); }

  template <typename ExistencePolicy, typename Ext, std::size_t N, typename T>
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
      if constexpr (std::is_same_v<ExistencePolicy, Required>) {
        if (target == nullptr) {
          self().error_handler().OnPathResolveError(detail::PathString(path));
        }
      }
    } else {
      target = detail::ResolveWithPolicy<Json, N, ExistencePolicy>(
          base, path, self().error_handler());
    }
    if (target == nullptr) {
      out = std::move(default_val);
      return self();
    }
    if (!Ext::Extract(*target, out)) {
      if constexpr (std::is_same_v<ExistencePolicy, Required>) {
        self().error_handler().OnTypeMismatch(detail::PathString(path));
      }
      out = std::move(default_val);
    }
    return self();
  }

  // 容器以「完整类型」作参数，内部元素类型取自 Container::value_type，
  // 不再需要推导模板模板参数（MSVC 更稳）。
  template <typename ExistencePolicy, typename Ext, typename Container,
            std::size_t N>
  Derived& FillArray(const Parts<N>& path, Container& arr) {
    using T = Container::value_type;
    arr.clear();
    const auto range = self().template ViewArray<ExistencePolicy>(path);
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
        return self();  // 若 ErrorHandler 返回，则中止填充
      }
      arr.push_back(std::move(value));
    }
    return self();
  }

  template <typename ExistencePolicy, typename Ext, typename Container,
            std::size_t N>
  Derived& FillMap(const Parts<N>& path, Container& map) {
    using T = Container::mapped_type;
    map.clear();
    const auto range = self().template ViewObject<ExistencePolicy>(path);
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
        return self();  // 若 ErrorHandler 返回，则中止填充
      }
      map.emplace(std::string(member.first), std::move(value));
    }
    return self();
  }
};

}  // namespace sc::json

#endif  // SCCOMMON_JSON_ACCESS_MIXIN_H
