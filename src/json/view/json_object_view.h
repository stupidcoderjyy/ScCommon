//
// Created by jiangyuyun3 on 2026/9/11.
//

#ifndef SCCOMMON_JSON_VIEW_JSON_OBJECT_VIEW_H
#define SCCOMMON_JSON_VIEW_JSON_OBJECT_VIEW_H

#include <cstddef>
#include <iterator>
#include <string_view>
#include <type_traits>
#include <utility>

#include "json_view.h"

namespace sc::json {

// 对 JSON 对象的惰性 range，元素为 (key, JsonView)，满足 forward_range。
// key 为 string_view，指向底层 Json 内部存储（jsoncpp
// 例外：键由迭代器自身持有， 在下一次解引用之前有效）。
template <typename Json, typename ErrorHandler = DefaultJsonParserErrorHandler>
class JsonObjectView {
 public:
  using value_type = std::pair<std::string_view, JsonView<Json, ErrorHandler>>;

  struct iterator {
    using difference_type = std::ptrdiff_t;
    using value_type = value_type;
    using reference = value_type;  // 按值返回，支持结构化绑定
    using pointer = void;
    using iterator_concept = std::forward_iterator_tag;
    using iterator_category = std::forward_iterator_tag;

    value_type operator*() const {
      const auto member = *it_;
      return {member.first,
              JsonView<Json, ErrorHandler>(*member.second, *handler_)};
    }
    iterator& operator++() {
      ++it_;
      return *this;
    }
    iterator operator++(int) {
      iterator copy = *this;
      ++it_;
      return copy;
    }
    bool operator==(const iterator& other) const { return it_ == other.it_; }

    JsonTraits<Json>::MemberIterator it_{};
    ErrorHandler* handler_ = nullptr;
  };

  JsonObjectView() noexcept = default;
  JsonObjectView(const Json& obj, ErrorHandler& handler) noexcept
      : obj_(&obj), handler_(&handler) {}

  // 空 view（默认构造 / Empty()）不触碰底层 Json，直接给出空区间
  iterator begin() const {
    return obj_ ? iterator{JsonTraits<Json>::MemberBegin(*obj_), handler_}
                : iterator{};
  }
  iterator end() const {
    return obj_ ? iterator{JsonTraits<Json>::MemberEnd(*obj_), handler_}
                : iterator{};
  }
  [[nodiscard]] bool empty() const { return begin() == end(); }

  static JsonObjectView Empty() noexcept { return {}; }

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
    return JsonObjectView<Json, ErrorHandler>::Empty();
  }
  const Json* node = this->template Find<ExistencePolicy>(path);
  if (!node) {
    return JsonObjectView<Json, ErrorHandler>::Empty();
  }
  if (!JsonTraits<Json>::IsObject(*node)) {
    if constexpr (std::is_same_v<ExistencePolicy, Required>) {
      handler_->OnTypeMismatch(detail::PathString(path));
    }
    return JsonObjectView<Json, ErrorHandler>::Empty();
  }
  return JsonObjectView<Json, ErrorHandler>(*node, *handler_);
}

}  // namespace sc::json

#endif  // SCCOMMON_JSON_VIEW_JSON_OBJECT_VIEW_H
