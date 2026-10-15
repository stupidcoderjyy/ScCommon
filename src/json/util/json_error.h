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

namespace sc::json {

struct DefaultJsonErrorHandler {
  static void OnPathResolveError(std::string_view path) {
    throw std::runtime_error("JsonOwner: failed to resolve path '" +
                             std::string(path) + "'");
  }
  static void OnTypeMismatch(std::string_view path) {
    throw std::runtime_error("JsonOwner: type mismatch at path '" +
                             std::string(path) + "'");
  }
  static void OnJsonParseError(std::string_view json, std::exception& e) {
    (void)json;
    throw std::runtime_error("JsonOwner: invalid json: " +
                             std::string(e.what()));
  }
};

template <typename T>
concept JsonErrorHandler =
    std::default_initializable<T> &&
    requires(T& handler, std::string_view msg, std::exception& parse_error,
             std::string_view path, std::string_view json) {
      handler.OnPathResolveError(msg);
      handler.OnTypeMismatch(path);
      handler.OnJsonParseError(json, parse_error);
    };

struct Required {};

struct Optional {};

}  // namespace sc::json

#endif  // SCCOMMON_JSON_ERROR_H
