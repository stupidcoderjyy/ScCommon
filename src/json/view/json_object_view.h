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

namespace sc::json {

template <typename Json, typename ErrorHandler = DefaultJsonErrorHandler>
class JsonObjectView {
 public:
  JsonObjectView() noexcept = default;
  JsonObjectView(const Json& obj, ErrorHandler& handler) noexcept
      : obj_(&obj), handler_(&handler) {}

  struct iterator {
    using difference_type = std::ptrdiff_t;
    using value_type =
        std::pair<std::string_view, JsonView<Json, ErrorHandler>>;
    using reference = value_type;
    using pointer = void;
    using iterator_concept = std::forward_iterator_tag;
    using iterator_category = std::forward_iterator_tag;

    value_type operator*() const {
      const auto member = *it;
      return {member.first,
              JsonView<Json, ErrorHandler>(*member.second, *handler)};
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
    ErrorHandler* handler = nullptr;
  };

  iterator begin() const {
    return obj_ ? iterator{JsonTraits<Json>::MemberBegin(*obj_), handler_}
                : iterator{};
  }

  iterator end() const {
    return obj_ ? iterator{JsonTraits<Json>::MemberEnd(*obj_), handler_}
                : iterator{};
  }

 private:
  const Json* obj_ = nullptr;
  ErrorHandler* handler_ = nullptr;
};

template <typename Json, typename ErrorHandler>
template <typename ExistencePolicy, typename Path>
  requires PathLike<Path>
JsonObjectView<Json, ErrorHandler> JsonView<Json, ErrorHandler>::ViewObject(
    const Path& path) const {
  static_assert(std::is_same_v<ExistencePolicy, Required> ||
                    std::is_same_v<ExistencePolicy, Optional>,
                "JsonView: unsupported ExistencePolicy");
  if (!valid()) {
    return JsonObjectView<Json, ErrorHandler>{};
  }
  const Json* node = this->template Find<ExistencePolicy>(path);
  if (!node) {
    return JsonObjectView<Json, ErrorHandler>{};
  }
  if (!JsonTraits<Json>::IsObject(*node)) {
    if constexpr (std::is_same_v<ExistencePolicy, Required>) {
      handler_.OnTypeMismatch(detail::PathString(path));
    }
    return JsonObjectView<Json, ErrorHandler>{};
  }
  return JsonObjectView<Json, ErrorHandler>(*node, handler_);
}

}  // namespace sc::json

#endif  // SCCOMMON_JSON_OBJECT_VIEW_H
