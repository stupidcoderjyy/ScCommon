//
// Created by jiangyuyun3 on 2026/10/15.
//

#ifndef SCCOMMON_JSON_WRITE_MIXIN_H
#define SCCOMMON_JSON_WRITE_MIXIN_H

#include <cstddef>
#include <string>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

#include "../../reflect/pfr.hpp"
#include "../json_owner.h"
#include "../json_traits.h"
#include "../util/json_detail.h"
#include "../util/json_read_write.h"

namespace sc::json {

namespace detail {

template <typename Json, typename T>
inline constexpr bool self_writable_v =
    std::is_base_of_v<JsonOwnerStorage<Json>, std::remove_cvref_t<T>> ||
    reflect::reflectable_v<std::remove_cvref_t<T>> ||
    is_map_v<std::remove_cvref_t<T>>;

}  // namespace detail

template <typename Derived, typename Json>
class JsonWriteMixin {
 public:
  using Self = Derived;
  using OwnerType = JsonOwnerStorage<Json>;

  template <AccessPolicy Policy = Optional, std::size_t N, typename T>
  void Set(const Parts<N>& path, T&& value) {
    if constexpr (N == 0) {
      if constexpr (detail::self_writable_v<Json, T>) {
        WriteSelf<Policy>(std::forward<T>(value));
      } else {
        PathError<Policy>(path);
      }
    } else if (Json* parent = Parent<Policy, true>(path); parent) {
      if constexpr (std::is_base_of_v<OwnerType, std::remove_cvref_t<T>>) {
        WriteOwner<Policy>(*self().owner(), *parent, path,
                           std::forward<T>(value));
      } else if (!WriteValue<Policy>(*parent, path[N - 1], value)) {
        TypeError<Policy>(path);
      }
    }
  }

  template <AccessPolicy Policy = Optional, typename T>
  void Set(std::string_view name, T&& value) {
    Set<Policy>(Parts{name}, std::forward<T>(value));
  }

  template <AccessPolicy Policy = Optional, std::size_t N>
  void Remove(const Parts<N>& path) {
    if constexpr (N == 0) {
      PathError<Policy>(path);
    } else if (Json* parent = Parent<Policy, false>(path); parent) {
      if (!JsonTraits<Json>::RemoveChild(*self().owner(), *parent,
                                         path[N - 1])) {
        PathError<Policy>(path);
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
    Proxy& operator=(T&& value) {
      writer_->template Set<Policy>(path_, std::forward<T>(value));
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

  template <AccessPolicy Policy, bool kCreate, std::size_t N>
  Json* Parent(const Parts<N>& path) {
    auto* owner = self().owner();
    if (!owner) return nullptr;
    if constexpr (N == 1) {
      return &owner->json();
    } else {
      using Handler = std::remove_reference_t<decltype(self().error_handler())>;
      return detail::ResolveWithPolicy<Json, N - 1, Policy, Handler, kCreate>(
          &owner->json(), path.template Sub<0, N - 1>(), self().error_handler(),
          owner);
    }
  }

  template <AccessPolicy Policy, std::size_t N>
  void PathError(const Parts<N>& path) {
    if constexpr (std::is_same_v<Policy, Required>) {
      self().error_handler().OnPathResolveError(detail::PathString(path));
    }
  }

  template <AccessPolicy Policy, std::size_t N>
  void TypeError(const Parts<N>& path) {
    if constexpr (std::is_same_v<Policy, Required>) {
      self().error_handler().OnTypeMismatch(detail::PathString(path));
    }
  }

  template <AccessPolicy Policy>
  void PathError(std::string_view path) {
    if constexpr (std::is_same_v<Policy, Required>) {
      self().error_handler().OnPathResolveError(path);
    }
  }

  template <AccessPolicy Policy>
  void TypeError(std::string_view path) {
    if constexpr (std::is_same_v<Policy, Required>) {
      self().error_handler().OnTypeMismatch(path);
    }
  }

  template <AccessPolicy Policy, typename T>
  bool WriteFields(Json& target, const T& value) {
    bool ok = true;
    std::apply(
        [&](const auto&... field) {
          ((ok =
                ok && WriteValue<Policy>(target, field.name, field.Get(value))),
           ...);
        },
        reflect::Descriptor<T>::fields());
    return ok;
  }

  template <AccessPolicy Policy, typename T>
  bool WriteValue(const Json& parent, std::string_view name, const T& value) {
    if constexpr (reflect::reflectable_v<T>) {
      Json* child = JsonTraits<Json>::AddChild(*self().owner(), parent, name);
      return child != nullptr && WriteFields<Policy>(*child, value);
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

  template <AccessPolicy Policy, typename T>
  bool WriteElement(Json& array, const T& value) {
    if constexpr (reflect::reflectable_v<T>) {
      Json* child = JsonTraits<Json>::AppendObject(*self().owner(), array);
      return child != nullptr && WriteFields<Policy>(*child, value);
    } else {
      return JsonTraits<Json>::Append(*self().owner(), array, value) != nullptr;
    }
  }

  template <typename T>
  bool Transfer(OwnerType& owner, Json& dst, T&& value) {
    if constexpr (std::is_lvalue_reference_v<T>) {
      return JsonTraits<Json>::Copy(owner, value.json(), dst);
    } else {
      return JsonTraits<Json>::Move(owner, std::move(value.json()), dst);
    }
  }

  void ClearObject(OwnerType& owner, Json& root) {
    std::vector<std::string> keys;
    for (auto it = JsonTraits<Json>::MemberBegin(root);
         it != JsonTraits<Json>::MemberEnd(root); ++it) {
      keys.emplace_back((*it).first);
    }
    for (const auto& key : keys) {
      JsonTraits<Json>::RemoveChild(owner, root, key);
    }
  }

  template <AccessPolicy Policy, std::size_t N, typename T>
  void WriteOwner(OwnerType& owner, Json& parent, const Parts<N>& path,
                  T&& value) {
    Json* child = JsonTraits<Json>::AddChild(owner, parent, path[N - 1]);
    if (!child || !Transfer(owner, *child, std::forward<T>(value))) {
      TypeError<Policy>(path);
    }
  }

  template <AccessPolicy Policy, typename T>
  void WriteSelf(T&& value) {
    OwnerType* owner = self().owner();
    if (!owner) {
      return;
    }
    Json& root = owner->json();
    using U = std::remove_cvref_t<T>;
    if constexpr (std::is_base_of_v<OwnerType, U>) {
      if (!Transfer(*owner, root, std::forward<T>(value))) {
        PathError<Policy>(std::string_view{});
      }
    } else {
      ClearObject(*owner, root);
      if constexpr (reflect::reflectable_v<U>) {
        if (!WriteFields<Policy>(root, value)) {
          TypeError<Policy>(std::string_view{});
        }
      } else if constexpr (detail::is_map_v<U>) {
        for (const auto& [key, mapped] : value) {
          if (!WriteValue<Policy>(root, key, mapped)) {
            TypeError<Policy>(key);
            return;
          }
        }
      }
    }
  }
};

}  // namespace sc::json

#endif  // SCCOMMON_JSON_WRITE_MIXIN_H
