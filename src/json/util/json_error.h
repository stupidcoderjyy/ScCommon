//
// Created by PC on 2026/9/14.
//

#ifndef SCCOMMON_JSON_ERROR_H
#define SCCOMMON_JSON_ERROR_H

#include <exception>
#include <stdexcept>
#include <string>
#include <string_view>

namespace sc::json {

struct DefaultJsonErrorHandler {
  static void OnPathResolveError(const std::exception& e) {
    throw std::runtime_error(e.what());
  }
  static void OnTypeMismatch(std::string_view path) {
    throw std::runtime_error("JsonParser: type mismatch at path '" +
                             std::string(path) + "'");
  }
  static void OnJsonParseError(std::string_view json, std::exception& e) {
    (void)json;
    throw std::runtime_error("JsonParser: invalid json: " +
                             std::string(e.what()));
  }
};

struct Required {};

struct Optional {};

}  // namespace sc::json

#endif  // SCCOMMON_JSON_ERROR_H
