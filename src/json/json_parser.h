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
    static void OnJsonParseError(std::string_view json, std::exception& e) {
        throw std::runtime_error("JsonParser: invalid json: " + std::string(e.what()));
    }
};

struct Required {};

struct Optional {};

template<typename Json, typename ErrorHandler = DefaultJsonParserErrorHandler>
class JsonParser {
public:
    explicit JsonParser(std::string_view json, ErrorHandler error_handler = {}): error_handler_(error_handler), root_(builder_.Parse(json, error_handler_)) {}

    explicit JsonParser(const Json& root, ErrorHandler handler = {})
        : error_handler_(std::move(handler)), root_(root) {}

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

    template<template<typename J> class Extractor, typename ExistencePolicy = Optional, template<typename ...> class List, typename T = Extractor<Json>::ValueType>
    JsonParser& AcquireArray(std::string_view path, List<T>& arr) {
        static_assert(std::is_same_v<ExistencePolicy, Required> ||
                          std::is_same_v<ExistencePolicy, Optional>,
                      "JsonParser: unsupported ExistencePolicy");
        static_assert(std::is_same_v<T, typename Extractor<Json>::ValueType>, "JsonParser: Acquire target type must match Extractor<Json>::ValueType");
        // 把一个json数组填入arr
        // ExistencePolicy只针对数组
        // 内部元素如果出现类型不匹配的情况, 直接调用OnTypeMismatch（需要组装完整路径，如 a.b[2]）
        const Json* node = Resolve(path);
        if (!node || !JsonTraits<Json>::IsArray(*node)) {
            if constexpr (std::is_same_v<ExistencePolicy, Optional>) {
                arr = List<T>{};
            } else {
                if (!node) {
                    error_handler_.OnMissing(path);
                } else {
                    error_handler_.OnTypeMismatch(path);
                }
            }
            return *this;
        }
        arr.clear();
        const std::size_t size = JsonTraits<Json>::GetArraySize(*node);
        for (std::size_t i = 0; i < size; ++i) {
            T value{};
            if (!Extractor<Json>::Extract(JsonTraits<Json>::GetArrayElement(*node, i), value)) {
                std::string element_path(path);
                element_path += '[';
                element_path += std::to_string(i);
                element_path += ']';
                error_handler_.OnTypeMismatch(element_path);
                return *this;  // 若 ErrorHandler 返回，则中止填充
            }
            arr.push_back(std::move(value));
        }
        return *this;
    }

    template<template<typename J> class Extractor, typename ExistencePolicy = Optional, template<typename ...> class Map, typename T = Extractor<Json>::ValueType>
    JsonParser& AcquireMap(std::string_view path, Map<std::string, T>& map) {
        static_assert(std::is_same_v<ExistencePolicy, Required> ||
                          std::is_same_v<ExistencePolicy, Optional>,
                      "JsonParser: unsupported ExistencePolicy");
        static_assert(std::is_same_v<T, typename Extractor<Json>::ValueType>, "JsonParser: Acquire target type must match Extractor<Json>::ValueType");
        // 把一个json对象填入map
        // ExistencePolicy只针对map
        // 内部元素如果出现类型不匹配的情况, 直接调用OnTypeMismatch（需要组装完整路径，如 a.b.c）
        const Json* node = Resolve(path);
        if (!node || !JsonTraits<Json>::IsObject(*node)) {
            if constexpr (std::is_same_v<ExistencePolicy, Optional>) {
                map = Map<std::string, T>{};
            } else {
                if (!node) {
                    error_handler_.OnMissing(path);
                } else {
                    error_handler_.OnTypeMismatch(path);
                }
            }
            return *this;
        }
        map.clear();
        JsonTraits<Json>::ForEachMember(*node, [&](std::string_view key, const Json& member) {
            T value{};
            if (!Extractor<Json>::Extract(member, value)) {
                std::string member_path(path);
                if (!member_path.empty()) {
                    member_path += '.';
                }
                member_path += key;
                error_handler_.OnTypeMismatch(member_path);
                return false;  // 若 ErrorHandler 返回，则中止填充
            }
            map.emplace(std::string(key), std::move(value));
            return true;
        });
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

    ErrorHandler error_handler_;
    JsonBuilder<Json, ErrorHandler> builder_;
    const Json& root_;
};

}  // namespace sc::json

#endif  // SCCOMMON_JSON_PARSER_N_H
