//
// Created by jiangyuyun3 on 2026/10/15.
//

#ifndef SCCOMMON_JSON_WRITE_MIXIN_H
#define SCCOMMON_JSON_WRITE_MIXIN_H

#include <cstddef>
#include <string_view>
#include <tuple>
#include <type_traits>

#include "../../reflect/pfr.hpp"
#include "../json_owner.h"
#include "../json_traits.h"
#include "../util/json_detail.h"
#include "../util/json_read_write.h"

namespace sc::json {

template <typename Derived, typename Json>
class JsonWriteMixin {
 public:
  using Self = Derived;
  using OwnerType = JsonOwnerStorage<Json>;

  template <typename T, AccessPolicy Policy = Optional, std::size_t N>
  void Set(const Parts<N>& path, const T& value) {
    OwnerType* owner = self().owner();
    if (owner == nullptr) {
      return;
    }
    if constexpr (N == 0) {
      if constexpr (std::is_same_v<Policy, Required>) {
        self().error_handler().OnPathResolveError(detail::PathString(path));
      }
    } else {
      Json* parent = nullptr;
      if constexpr (N == 1) {
        parent = &owner->json();
      } else {
        const auto sub = path.template Sub<0, N - 1>();
        using Handler =
            std::remove_reference_t<decltype(self().error_handler())>;
        parent = detail::ResolveWithPolicy<Json, N - 1, Policy, Handler, true>(
            &owner->json(), sub, self().error_handler(), owner);
      }
      if (parent == nullptr) {
        return;
      }
      if (!WriteValue<Policy>(*parent, path[N - 1], value)) {
        if constexpr (std::is_same_v<Policy, Required>) {
          self().error_handler().OnTypeMismatch(detail::PathString(path));
        }
      }
    }
  }

  template <typename T, AccessPolicy Policy = Optional>
  void Set(std::string_view name, const T& value) {
    Set<T, Policy>(Parts{name}, value);
  }

  template <AccessPolicy Policy = Optional, std::size_t N>
  void Remove(const Parts<N>& path) {
    OwnerType* owner = self().owner();
    if (owner == nullptr) {
      return;
    }
    if constexpr (N == 0) {
      // 空路径即根节点，无法删除
      if constexpr (std::is_same_v<Policy, Required>) {
        self().error_handler().OnPathResolveError(detail::PathString(path));
      }
    } else {
      Json* parent = nullptr;
      if constexpr (N == 1) {
        parent = &owner->json();
      } else {
        const auto sub = path.template Sub<0, N - 1>();
        parent = detail::ResolveWithPolicy<Json, N - 1, Policy>(
            &owner->json(), sub, self().error_handler());
      }
      if (parent == nullptr) {
        return;
      }
      if (!JsonTraits<Json>::RemoveChild(*owner, *parent, path[N - 1])) {
        if constexpr (std::is_same_v<Policy, Required>) {
          self().error_handler().OnPathResolveError(detail::PathString(path));
        }
      }
    }
  }

  template <AccessPolicy Policy = Optional>
  void Remove(std::string_view path) {
    Remove<Policy>(Parts{path});
  }

  template <std::size_t N, AccessPolicy Policy = Optional>
  class Proxy {
   public:
    Proxy(Self& writer, const Parts<N>& path) : writer_(&writer), path_(path) {}

    template <typename T>
    Proxy& operator=(const T& value) {
      writer_->template Set<T, Policy>(path_, value);
      return *this;
    }

   private:
    Self* writer_;
    Parts<N> path_;
  };

  template <AccessPolicy Policy = Optional, std::size_t N>
  Proxy<N, Policy> operator[](const Parts<N>& path) {
    return Proxy<N, Policy>(self(), path);
  }

  template <AccessPolicy Policy = Optional>
  Proxy<1, Policy> operator[](std::string_view name) {
    return Proxy<1, Policy>(self(), Parts{name});
  }

 private:
  Self& self() noexcept { return static_cast<Self&>(*this); }

  template <typename Policy, typename T>
  bool WriteValue(const Json& parent, std::string_view name, const T& value) {
    if constexpr (reflect::reflectable_v<T>) {
      Json* child = JsonTraits<Json>::AddChild(*self().owner(), parent, name);
      if (child == nullptr) {
        return false;
      }
      bool ok = true;
      std::apply(
          [&](const auto&... field) {
            ((ok = ok &&
                   WriteValue<Policy>(*child, field.name, field.Get(value))),
             ...);
          },
          reflect::Descriptor<T>::fields());
      return ok;
    } else if constexpr (detail::is_sequence_v<T>) {
      Json* array = JsonTraits<Json>::AddList(*self().owner(), parent, name);
      if (array == nullptr) {
        return false;
      }
      for (const auto& element : value) {
        if (!WriteElement<Policy>(*array, element)) {
          return false;
        }
      }
      return true;
    } else if constexpr (detail::is_map_v<T>) {
      Json* child = JsonTraits<Json>::AddChild(*self().owner(), parent, name);
      if (child == nullptr) {
        return false;
      }
      for (const auto& [key, mapped] : value) {
        if (!WriteValue<Policy>(*child, key, mapped)) {
          return false;
        }
      }
      return true;
    } else if constexpr (detail::is_optional_v<T>) {
      if (!value.has_value()) {
        return JsonTraits<Json>::AddNull(*self().owner(), parent, name) !=
               nullptr;
      }
      return WriteValue<Policy>(parent, name, *value);
    } else {
      return Injector<Json, T>::Inject(*self().owner(), parent, name, value);
    }
  }

  template <typename Policy, typename T>
  bool WriteElement(Json& array, const T& value) {
    if constexpr (reflect::reflectable_v<T>) {
      Json* child = JsonTraits<Json>::AppendObject(*self().owner(), array);
      if (child == nullptr) {
        return false;
      }
      bool ok = true;
      std::apply(
          [&](const auto&... field) {
            ((ok = ok &&
                   WriteValue<Policy>(*child, field.name, field.Get(value))),
             ...);
          },
          reflect::Descriptor<T>::fields());
      return ok;
    } else {
      return JsonTraits<Json>::Append(*self().owner(), array, value) != nullptr;
    }
  }
};

}  // namespace sc::json

#endif  // SCCOMMON_JSON_WRITE_MIXIN_H
