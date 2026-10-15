//
// Created by jiangyuyun3 on 2026/9/14.
//

#ifndef SCCOMMON_JSON_ARRAY_VIEW_H
#define SCCOMMON_JSON_ARRAY_VIEW_H

#include "../json_view.h"
#include "../util/json_detail.h"
#include "../util/json_error.h"

namespace sc::json {

template <typename Json, JsonErrorHandler E = DefaultJsonErrorHandler>
class JsonArrayView {
 public:
  JsonArrayView() noexcept = default;
  explicit JsonArrayView(const Json& array) noexcept
      : array_(&array), size_(JsonTraits<Json>::GetArraySize(array)) {}

  struct iterator {
    using difference_type = std::ptrdiff_t;
    using value_type = JsonView<Json, E>;
    using reference = value_type;
    using pointer = void;
    using iterator_concept = std::random_access_iterator_tag;
    using iterator_category = std::random_access_iterator_tag;

    value_type operator*() const {
      return value_type{JsonTraits<Json>::GetArrayElement(*array_, index_)};
    }
    [[nodiscard]] detail::ArrowProxy<value_type> operator->() const {
      return {operator*()};
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
  };

  [[nodiscard]] iterator begin() const { return {array_, 0}; }
  [[nodiscard]] iterator end() const { return {array_, size_}; }
  [[nodiscard]] std::size_t size() const noexcept { return size_; }
  [[nodiscard]] bool empty() const noexcept { return size_ == 0; }
  [[nodiscard]] const Json* node() const noexcept { return array_; }

 private:
  const Json* array_ = nullptr;
  std::size_t size_ = 0;  // 缓存，使 end()/size() 均 O(1)
};

template <typename Json, JsonErrorHandler E>
template <AccessPolicy Policy, std::size_t N>
JsonArrayView<Json, E> JsonView<Json, E>::ViewArray(const Parts<N>& path,
                                                    E& handler) const {
  if (!valid()) {
    return {};
  }
  const Json* node =
      detail::ResolveWithPolicy<Json, N, Policy>(json_, path, handler);
  if (!node) {
    return {};
  }
  if (!JsonTraits<Json>::IsArray(*node)) {
    if constexpr (std::is_same_v<Policy, Required>) {
      handler.OnTypeMismatch(detail::PathString(path));
    }
    return {};
  }
  return JsonArrayView<Json, E>(*node);
}

}  // namespace sc::json

#endif  // SCCOMMON_JSON_ARRAY_VIEW_H
