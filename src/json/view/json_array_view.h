//
// Created by jiangyuyun3 on 2026/9/11.
//

#ifndef SCCOMMON_JSON_VIEW_JSON_ARRAY_VIEW_H
#define SCCOMMON_JSON_VIEW_JSON_ARRAY_VIEW_H

#include <cstddef>
#include <iterator>
#include <type_traits>

#include "json_view.h"

namespace sc::json {

// 对 JSON 数组的惰性 range，元素按值产出 JsonView，满足 random_access_range。
template <typename Json, typename ErrorHandler = DefaultJsonParserErrorHandler>
class JsonArrayView {
 public:
  struct iterator {
    using difference_type = std::ptrdiff_t;
    using value_type = JsonView<Json, ErrorHandler>;
    using reference = value_type;  // 按值返回
    using pointer = void;
    using iterator_concept = std::random_access_iterator_tag;
    using iterator_category = std::random_access_iterator_tag;

    value_type operator*() const {
      return value_type{JsonTraits<Json>::GetArrayElement(*array_, index_),
                        *handler_};
    }
    iterator& operator++() {
      ++index_;
      return *this;
    }
    iterator operator++(int) {
      iterator copy = *this;
      ++index_;
      return copy;
    }
    iterator& operator--() {
      --index_;
      return *this;
    }
    iterator operator--(int) {
      iterator copy = *this;
      --index_;
      return copy;
    }
    iterator& operator+=(difference_type offset) {
      index_ = static_cast<std::size_t>(static_cast<difference_type>(index_) +
                                        offset);
      return *this;
    }
    iterator& operator-=(difference_type offset) { return *this += -offset; }
    iterator operator+(difference_type offset) const {
      iterator copy = *this;
      copy += offset;
      return copy;
    }
    iterator operator-(difference_type offset) const {
      return *this + (-offset);
    }
    friend iterator operator+(difference_type offset, const iterator& it) {
      return it + offset;
    }
    difference_type operator-(const iterator& other) const {
      return static_cast<difference_type>(index_) -
             static_cast<difference_type>(other.index_);
    }
    value_type operator[](difference_type offset) const {
      return *(*this + offset);
    }
    bool operator==(const iterator& other) const {
      return index_ == other.index_;
    }
    bool operator<(const iterator& other) const {
      return index_ < other.index_;
    }
    bool operator!=(const iterator& other) const { return !(*this == other); }
    bool operator>(const iterator& other) const { return other < *this; }
    bool operator<=(const iterator& other) const { return !(other < *this); }
    bool operator>=(const iterator& other) const { return !(*this < other); }

    const Json* array_ = nullptr;
    std::size_t index_ = 0;
    ErrorHandler* handler_ = nullptr;
  };

  JsonArrayView() noexcept = default;
  JsonArrayView(const Json& array, ErrorHandler& handler) noexcept
      : array_(&array),
        size_(JsonTraits<Json>::GetArraySize(array)),
        handler_(&handler) {}

  iterator begin() const { return {array_, 0, handler_}; }
  iterator end() const { return {array_, size_, handler_}; }
  [[nodiscard]] std::size_t size() const noexcept { return size_; }
  [[nodiscard]] bool empty() const noexcept { return size_ == 0; }

  static JsonArrayView Empty() noexcept { return {}; }

 private:
  const Json* array_ = nullptr;
  std::size_t size_ = 0;  // 缓存，使 end()/size() 均 O(1)
  ErrorHandler* handler_ = nullptr;
};

template <typename Json, typename ErrorHandler>
template <typename ExistencePolicy, typename Path>
  requires PathLike<Path>
JsonArrayView<Json, ErrorHandler> JsonView<Json, ErrorHandler>::ViewArray(
    const Path& path) const {
  static_assert(std::is_same_v<ExistencePolicy, Required> ||
                    std::is_same_v<ExistencePolicy, Optional>,
                "JsonView: unsupported ExistencePolicy");
  if (!valid()) {
    return JsonArrayView<Json, ErrorHandler>::Empty();
  }
  const Json* node = this->template Find<ExistencePolicy>(path);
  if (!node) {
    return JsonArrayView<Json, ErrorHandler>::Empty();
  }
  if (!JsonTraits<Json>::IsArray(*node)) {
    if constexpr (std::is_same_v<ExistencePolicy, Required>) {
      handler_->OnTypeMismatch(detail::PathString(path));
    }
    return JsonArrayView<Json, ErrorHandler>::Empty();
  }
  return JsonArrayView<Json, ErrorHandler>(*node, *handler_);
}

}  // namespace sc::json

#endif  // SCCOMMON_JSON_VIEW_JSON_ARRAY_VIEW_H
