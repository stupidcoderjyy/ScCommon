//
// Created by PC on 2026/9/14.
//

#ifndef SCCOMMON_JSON_ERROR_H
#define SCCOMMON_JSON_ERROR_H

#include <concepts>
#include <exception>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>

namespace sc::json {

struct DefaultJsonErrorHandler {
  static void OnPathResolveError(std::string_view path) {
    throw std::runtime_error("sc::json: failed to resolve path '" +
                             std::string(path) + "'");
  }
  static void OnTypeMismatch(std::string_view path) {
    throw std::runtime_error("sc::json: type mismatch at path '" +
                             std::string(path) + "'");
  }
  static void OnJsonParseError(std::string_view reason) {
    throw std::runtime_error("sc::json: invalid json: " + std::string(reason));
  }
};

struct EmptyJsonErrorHandler {
  static void OnPathResolveError(std::string_view) {}
  static void OnTypeMismatch(std::string_view) {}
  static void OnJsonParseError(std::string_view) {}
};

inline DefaultJsonErrorHandler kDefaultErrorHandler{};
inline EmptyJsonErrorHandler kEmptyErrorHandler{};

template <typename T>
concept JsonErrorHandler =
    std::default_initializable<T> &&
    requires(T& handler, std::string_view msg, std::string_view path,
             std::string_view reason) {
      handler.OnPathResolveError(msg);
      handler.OnTypeMismatch(path);
      handler.OnJsonParseError(reason);
    };

template <JsonErrorHandler E>
E& GetDefaultErrorHandler() {
  if constexpr (std::is_same_v<E, EmptyJsonErrorHandler>) {
    return kEmptyErrorHandler;
  } else if constexpr (std::is_same_v<E, DefaultJsonErrorHandler>) {
    return kDefaultErrorHandler;
  } else {
    static_assert(!std::is_same_v<E, E>,
                  "sc::json: custom error handler must be passed explicitly");
  }
  return kDefaultErrorHandler;
}

struct Required {};

struct Optional {};

template <typename T>
concept AccessPolicy =
    std::is_same_v<T, Required> || std::is_same_v<T, Optional>;

}  // namespace sc::json

#endif  // SCCOMMON_JSON_ERROR_H
