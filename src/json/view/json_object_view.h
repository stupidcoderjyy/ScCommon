//
// Created by PC on 2026/9/14.
//

#ifndef SCCOMMON_JSON_OBJECT_VIEW_H
#define SCCOMMON_JSON_OBJECT_VIEW_H

#include <cstddef>
#include <iterator>
#include <string_view>
#include <utility>

#include "../json_view.h"
#include "../util/json_detail.h"

namespace sc::json {

template <typename Json, JsonErrorHandler E = DefaultJsonErrorHandler>
class JsonObjectView {
 public:
  JsonObjectView() noexcept = default;
  explicit JsonObjectView(const Json& obj,
                          E& handler = GetDefaultErrorHandler<E>()) noexcept
      : obj_(&obj), handler_(&handler) {}

  struct iterator {
    using difference_type = std::ptrdiff_t;
    using value_type = std::pair<std::string_view, JsonView<Json, E>>;
    using reference = value_type;
    using pointer = void;
    using iterator_concept = std::forward_iterator_tag;
    using iterator_category = std::forward_iterator_tag;

    value_type operator*() const {
      const auto member = *it;
      return {member.first, JsonView<Json, E>(*member.second, *handler_)};
    }
    [[nodiscard]] detail::ArrowProxy<value_type> operator->() const {
      return {operator*()};
    }
    iterator& operator++() {
      ++it;
      return *this;
    }
    // 后置自增
    iterator operator++(int) {
      auto copy = *this;
      ++it;
      return copy;
    }
    bool operator==(const iterator& other) const { return it == other.it; }

    JsonTraits<Json>::MemberIterator it{};
    E* handler_ = nullptr;
  };

  iterator begin() const {
    return obj_ ? iterator{JsonTraits<Json>::MemberBegin(*obj_), handler_}
                : iterator{};
  }

  iterator end() const {
    return obj_ ? iterator{JsonTraits<Json>::MemberEnd(*obj_), handler_}
                : iterator{};
  }

  [[nodiscard]] const Json* node() const noexcept { return obj_; }

 private:
  const Json* obj_ = nullptr;
  E* handler_ = nullptr;
};

template <typename Json, JsonErrorHandler E>
template <AccessPolicy Policy, std::size_t N>
JsonObjectView<Json, E> JsonView<Json, E>::ViewObject(
    const Parts<N>& path) const {
  if (!valid()) {
    return JsonObjectView<Json, E>{};
  }
  const Json* node =
      detail::ResolveWithPolicy<Json, N, Policy>(json_, path, error_handler());
  if (!node) {
    return JsonObjectView<Json, E>{};
  }
  if (!JsonTraits<Json>::IsObject(*node)) {
    if constexpr (std::is_same_v<Policy, Required>) {
      error_handler().OnTypeMismatch(detail::PathString(path));
    }
    return JsonObjectView<Json, E>{};
  }
  return JsonObjectView<Json, E>(*node, error_handler());
}

}  // namespace sc::json

#endif  // SCCOMMON_JSON_OBJECT_VIEW_H
