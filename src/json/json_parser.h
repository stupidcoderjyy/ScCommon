//
// Created by jiangyuyun3 on 2026/9/7.
//

#ifndef SCCOMMON_JSON_PARSER_N_H
#define SCCOMMON_JSON_PARSER_N_H

#include "json_extractor.h" //NOLINT
#include "json_traits.h"

#include <charconv>
#include <cstddef>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>

#if defined(NLOHMANN_JSON_HPP) || defined(INCLUDE_NLOHMANN_JSON_HPP_) || \
        __has_include(<nlohmann/json.hpp>)
#include "traits/nlohmann.h"
#endif
#if defined(RAPIDJSON_DOCUMENT_H_) || __has_include(<rapidjson/document.h>)
#include "traits/rapidjson.h"
#endif
#if defined(JSON_JSON_H_INCLUDED) || __has_include(<json/json.h>)
#include "traits/jsoncpp.h"
#endif

namespace sc::json {

struct DefaultJsonParserErrorHandler {
    static void OnMissing(std::string_view path) {
        throw std::runtime_error("JsonParser: path '" + std::string(path) + "' not found");
    }
    static void OnTypeMismatch(std::string_view path) {
        throw std::runtime_error("JsonParser: type mismatch at path '" + std::string(path) + "'");
    }
};

struct Required {};

struct Optional {};

template<typename Json, typename ErrorHandler = DefaultJsonParserErrorHandler>
class JsonParser {
public:
    explicit JsonParser(const Json& root, ErrorHandler handler = {})
        : root_(root), error_handler_(std::move(handler)) {}

    template<template<typename> class Extractor, typename ExistencePolicy = Optional, typename Func,
             typename T = Extractor<Json>::ValueType>
    JsonParser& Parse(std::string_view path, Func&& callback, T default_val = {}) {
        static_assert(std::is_same_v<ExistencePolicy, Required> ||
                          std::is_same_v<ExistencePolicy, Optional>,
                      "JsonParser: unsupported ExistencePolicy");
        using Val = Extractor<Json>::ValueType;
        static_assert(std::is_same_v<T, Val>,
                      "JsonParser: default value type must match Extractor<Json>::ValueType");
        Val value = std::move(default_val);
        const Json* node = Resolve(path);
        if (!node) {
            if constexpr (std::is_same_v<ExistencePolicy, Optional>) {
                std::forward<Func>(callback)(std::move(value));
            } else {
                error_handler_.OnMissing(path);
            }
            return *this;
        }
        if (!Extractor<Json>::Extract(*node, value)) {
            if constexpr (std::is_same_v<ExistencePolicy, Optional>) {
                std::forward<Func>(callback)(std::move(value));
            } else {
                error_handler_.OnTypeMismatch(path);
            }
            return *this;
        }
        std::forward<Func>(callback)(std::move(value));
        return *this;
    }

    template<template<typename J> class Extractor, typename ExistencePolicy = Optional,
             typename T = Extractor<Json>::ValueType>
    JsonParser& Acquire(std::string_view path, T& out, T default_val = {}) {
        static_assert(std::is_same_v<ExistencePolicy, Required> ||
                          std::is_same_v<ExistencePolicy, Optional>,
                      "JsonParser: unsupported ExistencePolicy");
        using Val = Extractor<Json>::ValueType;
        static_assert(std::is_same_v<T, Val>,
                      "JsonParser: Acquire target type must match Extractor<Json>::ValueType");
        Val value = std::move(default_val);
        const Json* node = Resolve(path);
        if (!node) {
            if constexpr (std::is_same_v<ExistencePolicy, Optional>) {
                out = std::move(value);
            } else {
                error_handler_.OnMissing(path);
            }
            return *this;
        }
        if (!Extractor<Json>::Extract(*node, value)) {
            if constexpr (std::is_same_v<ExistencePolicy, Optional>) {
                out = std::move(value);
            } else {
                error_handler_.OnTypeMismatch(path);
            }
            return *this;
        }
        out = std::move(value);
        return *this;
    }

private:
    const Json* Resolve(std::string_view path) const {
        const Json* result = &root_;
        std::size_t pos = 0;
        while (true) {
            const std::size_t dot = path.find('.', pos);
            const std::size_t end = dot == std::string_view::npos ? path.size() : dot;
            if (!ApplySegment(result, path.substr(pos, end - pos))) {
                return nullptr;
            }
            if (dot == std::string_view::npos) {
                break;
            }
            pos = dot + 1;
        }
        return result;
    }

    bool ApplySegment(const Json*& json, std::string_view segment) const {
        auto bracket = segment.find('[');
        if (auto key = segment.substr(0, bracket); !key.empty()) {
            if (!JsonTraits<Json>::IsObject(*json) || !JsonTraits<Json>::HasKey(*json, key)) {
                return false;
            }
            json = &JsonTraits<Json>::GetChild(*json, key);
        }

        auto l = bracket;
        while (l != std::string_view::npos) {
            auto r = segment.find(']', l + 1);
            if (r == std::string_view::npos) {
                return false;
            }
            const std::string_view digits = segment.substr(l + 1, r - l - 1);
            if (digits.empty()) {
                return false;
            }
            std::size_t index = 0;
            if (auto [ptr, ec] =
                        std::from_chars(digits.data(), digits.data() + digits.size(), index);
                ec != std::errc() || ptr != digits.data() + digits.size()) {
                return false;
            }
            if (!JsonTraits<Json>::IsArray(*json) ||
                index >= JsonTraits<Json>::GetArraySize(*json)) {
                return false;
            }
            json = &JsonTraits<Json>::GetArrayElement(*json, index);
            l = segment.find('[', r + 1);
        }
        return true;
    }

    const Json& root_;
    ErrorHandler error_handler_;
};

}  // namespace sc::json

#endif  // SCCOMMON_JSON_PARSER_N_H
