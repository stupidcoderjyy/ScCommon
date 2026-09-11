//
// Created by jiangyuyun3 on 2026/9/7.
//

#ifndef SCCOMMON_JSON_PARSER_N_H
#define SCCOMMON_JSON_PARSER_N_H

#include "json_extractor.h"  //NOLINT
#include "json_path.h"
#include "json_traits.h"

#include <charconv>
#include <cstddef>
#include <stack>
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
        (void)json;
        throw std::runtime_error("JsonParser: invalid json: " + std::string(e.what()));
    }
};

struct Required {};

struct Optional {};

template <typename Json, typename ErrorHandler = DefaultJsonParserErrorHandler>
class JsonParser {
public:
    JsonParser(const JsonParser&) = delete;
    JsonParser(JsonParser&&) = delete;
    JsonParser& operator=(const JsonParser&) = delete;
    JsonParser& operator=(JsonParser&&) = delete;

    explicit JsonParser(std::string_view json, ErrorHandler error_handler = {})
        : error_handler_(std::move(error_handler)),
          builder_(json, error_handler_),
          root_(builder_.json()) {
        stack_.push(&root_);
    }

    explicit JsonParser(const Json& root, ErrorHandler handler = {})
        : error_handler_(std::move(handler)), root_(root) {
        stack_.push(&root_);
    }

    const Json& json() const { return root_; }

    template <template <typename> class Extractor,
            typename ExistencePolicy = Optional,
            std::size_t N,
            typename Func,
            typename T = Extractor<Json>::ValueType>
    JsonParser& Parse(const Parts<N>& path, Func&& callback, T default_val = {}) {
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
                error_handler_.OnMissing(PathString(path));
            }
            return *this;
        }
        if (!Extractor<Json>::Extract(*node, value)) {
            if constexpr (std::is_same_v<ExistencePolicy, Optional>) {
                std::forward<Func>(callback)(std::move(value));
            } else {
                error_handler_.OnTypeMismatch(PathString(path));
            }
            return *this;
        }
        std::forward<Func>(callback)(std::move(value));
        return *this;
    }

    template <template <typename J> class Extractor,
            typename ExistencePolicy = Optional,
            std::size_t N,
            typename T = Extractor<Json>::ValueType>
    JsonParser& Acquire(const Parts<N>& path, T& out, T default_val = {}) {
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
                error_handler_.OnMissing(PathString(path));
            }
            return *this;
        }
        if (!Extractor<Json>::Extract(*node, value)) {
            if constexpr (std::is_same_v<ExistencePolicy, Optional>) {
                out = std::move(value);
            } else {
                error_handler_.OnTypeMismatch(PathString(path));
            }
            return *this;
        }
        out = std::move(value);
        return *this;
    }

    template <template <typename J> class Extractor,
            typename ExistencePolicy = Optional,
            std::size_t N,
            template <typename...> class List,
            typename T = Extractor<Json>::ValueType>
    JsonParser& AcquireArray(const Parts<N>& path, List<T>& arr) {
        static_assert(std::is_same_v<ExistencePolicy, Required> ||
                              std::is_same_v<ExistencePolicy, Optional>,
                "JsonParser: unsupported ExistencePolicy");
        static_assert(std::is_same_v<T, typename Extractor<Json>::ValueType>,
                "JsonParser: Acquire target type must match Extractor<Json>::ValueType");
        // 把一个json数组填入arr
        // ExistencePolicy只针对数组
        // 内部元素如果出现类型不匹配的情况, 直接调用OnTypeMismatch（需要组装完整路径，如 a.b[2]）
        const Json* node = Resolve(path);
        if (!node || !JsonTraits<Json>::IsArray(*node)) {
            if constexpr (std::is_same_v<ExistencePolicy, Optional>) {
                arr = List<T>{};
            } else {
                if (!node) {
                    error_handler_.OnMissing(PathString(path));
                } else {
                    error_handler_.OnTypeMismatch(PathString(path));
                }
            }
            return *this;
        }
        const std::size_t size = JsonTraits<Json>::GetArraySize(*node);
        arr.resize(size);
        for (std::size_t i = 0; i < size; ++i) {
            T value{};
            if (!Extractor<Json>::Extract(JsonTraits<Json>::GetArrayElement(*node, i), value)) {
                std::string element_path = PathString(path);
                element_path += '[';
                element_path += std::to_string(i);
                element_path += ']';
                error_handler_.OnTypeMismatch(element_path);
                return *this;  // 若 ErrorHandler 返回，则中止填充
            }
            arr[i] = std::move(value);
        }
        return *this;
    }

    template <template <typename J> class Extractor,
            typename ExistencePolicy = Optional,
            std::size_t N,
            template <typename...> class Map,
            typename T = Extractor<Json>::ValueType>
    JsonParser& AcquireMap(const Parts<N>& path, Map<std::string, T>& map) {
        static_assert(std::is_same_v<ExistencePolicy, Required> ||
                              std::is_same_v<ExistencePolicy, Optional>,
                "JsonParser: unsupported ExistencePolicy");
        static_assert(std::is_same_v<T, typename Extractor<Json>::ValueType>,
                "JsonParser: Acquire target type must match Extractor<Json>::ValueType");
        // 把一个json对象填入map
        // ExistencePolicy只针对map
        // 内部元素如果出现类型不匹配的情况, 直接调用OnTypeMismatch（需要组装完整路径，如 a.b.c）
        const Json* node = Resolve(path);
        if (!node || !JsonTraits<Json>::IsObject(*node)) {
            if constexpr (std::is_same_v<ExistencePolicy, Optional>) {
                map = Map<std::string, T>{};
            } else {
                if (!node) {
                    error_handler_.OnMissing(PathString(path));
                } else {
                    error_handler_.OnTypeMismatch(PathString(path));
                }
            }
            return *this;
        }
        map.clear();
        JsonTraits<Json>::ForEachMember(*node, [&](std::string_view key, const Json& member) {
            T value{};
            if (!Extractor<Json>::Extract(member, value)) {
                std::string member_path = PathString(path);
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

    template <std::size_t N>
    const Json* Resolve(const Parts<N>& parts) {
        const Json* node = stack_.top();
        for (std::size_t i = 0; i < parts.Size(); ++i) {
            const std::string_view token = parts[i];
            if (token.empty()) {
                return nullptr;
            }
            if (token.front() == '[') {
                if (token.size() < 2 || token.back() != ']') {
                    return nullptr;
                }
                const std::string_view digits = token.substr(1, token.size() - 2);
                std::size_t index = 0;
                if (auto [ptr, ec] = std::from_chars(
                            digits.data(), digits.data() + digits.size(), index);
                        ec != std::errc() || ptr != digits.data() + digits.size()) {
                    return nullptr;
                }
                if (!JsonTraits<Json>::IsArray(*node) ||
                        index >= JsonTraits<Json>::GetArraySize(*node)) {
                    return nullptr;
                }
                node = &JsonTraits<Json>::GetArrayElement(*node, index);
            } else {
                if (!JsonTraits<Json>::IsObject(*node) || !JsonTraits<Json>::HasKey(*node, token)) {
                    return nullptr;
                }
                node = &JsonTraits<Json>::GetChild(*node, token);
            }
        }
        return node;
    }

private:
    // 把编译期路径还原成 "a.b[2].c" 形式的字符串，用于错误回调。
    template <std::size_t N>
    static std::string PathString(const Parts<N>& parts) {
        std::string out;
        for (std::size_t i = 0; i < parts.Size(); ++i) {
            const std::string_view token = parts[i];
            if (const bool is_index = !token.empty() && token.front() == '[';
                    !is_index && !out.empty()) {
                out += '.';
            }
            out += token;
        }
        return out;
    }

    ErrorHandler error_handler_;
    JsonBuilder<Json, ErrorHandler> builder_;
    const Json& root_;
    std::stack<const Json*> stack_;
};

}  // namespace sc::json

#endif  // SCCOMMON_JSON_PARSER_N_H
