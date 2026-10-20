//
// Created by PC on 2026/9/14.
//

#ifndef SCCOMMON_JSON_ERROR_H
#define SCCOMMON_JSON_ERROR_H

#include <concepts>
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

namespace ec {

inline constexpr int kPathResolveError = -2000;
inline constexpr int kTypeMismatch = -2001;
inline constexpr int kJsonParseError = -2002;

}

struct ErrorCodeJsonErrorHandler {
  void OnPathResolveError(std::string_view) {
    code_ = next_ec_ == 0 ? ec::kPathResolveError : next_ec_;
    next_ec_ = 0;
  }
  void OnTypeMismatch(std::string_view) {
    code_ = next_ec_ == 0 ? ec::kTypeMismatch : next_ec_;
    next_ec_ = 0;
  }
  void OnJsonParseError(std::string_view) {
    code_ = next_ec_ == 0 ? ec::kJsonParseError : next_ec_;
    next_ec_ = 0;
  }

  [[nodiscard]] bool error() const { return code_ != 0; }
  [[nodiscard]] bool ok() const { return code_ == 0; }
  [[nodiscard]] int code() const { return code_; }

  void set_next_ec(int ec) { next_ec_ = ec; }

  bool take_error() {
    auto e = error();
    code_ = 0;
    return e;
  }

  int take_code() {
    auto c = code();
    code_ = 0;
    return c;
  }

 private:
  int code_{0};
  int next_ec_{0};
};

inline DefaultJsonErrorHandler kDefaultErrorHandler{};
inline EmptyJsonErrorHandler kEmptyErrorHandler{};
inline ErrorCodeJsonErrorHandler kErrorCodeErrorHandler{};

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
  } else if constexpr (std::is_same_v<E, ErrorCodeJsonErrorHandler>) {
    return kErrorCodeErrorHandler;
  } else {
    static_assert(!std::is_same_v<E, E>,
                  "sc::json: custom error handler must be passed explicitly");
    throw;
  }
}

struct Required {};

struct Optional {};

template <typename T>
concept AccessPolicy =
    std::is_same_v<T, Required> || std::is_same_v<T, Optional>;

}  // namespace sc::json

#endif  // SCCOMMON_JSON_ERROR_H
