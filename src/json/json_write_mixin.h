//
// Created by jiangyuyun3 on 2026/10/15.
//

#ifndef SCCOMMON_JSON_WRITE_MIXIN_H
#define SCCOMMON_JSON_WRITE_MIXIN_H

#include <cstddef>
#include <string_view>
#include <tuple>
#include <type_traits>

#include "json_owner.h"
#include "json_path.h"
#include "json_traits.h"
#include "reflect/get_fields.h"
#include "util/json_detail.h"
#include "util/json_read_write.h"

namespace sc::json {

template <typename Derived, typename Json, typename ErrorHandler>
class JsonWriteMixin {
 public:
  using Self = Derived;
  using OwnerType = JsonOwner<Json, ErrorHandler>;

  template <typename ExistencePolicy = Required, std::size_t N, typename T>
  Self& Set(const Parts<N>& path, const T& value) {
    if constexpr (N == 0) {
      if constexpr (std::is_same_v<ExistencePolicy, Required>) {
        self().owner().err_handler().OnPathResolveError(
            detail::PathString(path));
      }
      return self();
    } else {
      Json* parent = nullptr;
      if constexpr (N == 1) {
        parent = &self().owner().json();
      } else {
        const auto sub = path.template Sub<0, N - 1>();
        parent = detail::ResolveWithPolicy<Json, N - 1, ExistencePolicy,
                                           ErrorHandler, true>(
            &self().owner().json(), sub, self().owner().err_handler(),
            &self().owner());
      }
      if (parent == nullptr) {
        return self();
      }
      if (!WriteValue<ExistencePolicy>(*parent, path[N - 1], value)) {
        if constexpr (std::is_same_v<ExistencePolicy, Required>) {
          self().owner().err_handler().OnTypeMismatch(detail::PathString(path));
        }
      }
      return self();
    }
  }

  template <std::size_t N, typename ExistencePolicy = Required>
  class Proxy {
   public:
    Proxy(Self& writer, const Parts<N>& path) : writer_(&writer), path_(path) {}

    template <typename T>
    Proxy& operator=(const T& value) {
      writer_->template Set<ExistencePolicy>(path_, value);
      return *this;
    }

   private:
    Self* writer_;
    Parts<N> path_;
  };

  template <typename ExistencePolicy = Required, std::size_t N>
  Proxy<N, ExistencePolicy> operator[](const Parts<N>& path) {
    return Proxy<N, ExistencePolicy>(self(), path);
  }

 private:
  Self& self() noexcept { return static_cast<Self&>(*this); }

  template <typename ExistencePolicy, typename T>
  bool WriteValue(const Json& parent, std::string_view name, const T& value) {
    if constexpr (reflect::is_registered_v<T>) {
      Json* child = JsonTraits<Json>::AddChild(self().owner(), parent, name);
      if (child == nullptr) {
        return false;
      }
      bool ok = true;
      std::apply(
          [&](const auto&... field) {
            ((ok = ok && WriteValue<ExistencePolicy>(*child, field.name,
                                                    value.*field.member)),
             ...);
          },
          reflect::Descriptor<T>::fields());
      return ok;
    } else if constexpr (detail::is_sequence_v<T>) {
      Json* array = JsonTraits<Json>::AddList(self().owner(), parent, name);
      if (array == nullptr) {
        return false;
      }
      for (const auto& element : value) {
        if (!WriteElement<ExistencePolicy>(*array, element)) {
          return false;
        }
      }
      return true;
    } else if constexpr (detail::is_map_v<T>) {
      Json* child = JsonTraits<Json>::AddChild(self().owner(), parent, name);
      if (child == nullptr) {
        return false;
      }
      for (const auto& [key, mapped] : value) {
        if (!WriteValue<ExistencePolicy>(*child, key, mapped)) {
          return false;
        }
      }
      return true;
    } else if constexpr (detail::is_optional_v<T>) {
      if (!value.has_value()) {
        return JsonTraits<Json>::AddNull(self().owner(), parent, name) !=
               nullptr;
      }
      return WriteValue<ExistencePolicy>(parent, name, *value);
    } else {
      return Injector<Json, T>::Inject(self().owner(), parent, name, value);
    }
  }

  template <typename ExistencePolicy, typename T>
  bool WriteElement(Json& array, const T& value) {
    if constexpr (reflect::is_registered_v<T>) {
      Json* child = JsonTraits<Json>::AppendObject(self().owner(), array);
      if (child == nullptr) {
        return false;
      }
      bool ok = true;
      std::apply(
          [&](const auto&... field) {
            ((ok = ok && WriteValue<ExistencePolicy>(*child, field.name,
                                                    value.*field.member)),
             ...);
          },
          reflect::Descriptor<T>::fields());
      return ok;
    } else {
      return JsonTraits<Json>::Append(self().owner(), array, value) != nullptr;
    }
  }
};

}  // namespace sc::json

#endif  // SCCOMMON_JSON_WRITE_MIXIN_H
