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

// 错误处理协议：
//   OnPathResolveError —— 路径解析失败（缺失 key、下标越界、路径中途类型不符等）
//   OnTypeMismatch     —— 路径解析成功，但节点无法转换为目标类型
//   OnJsonParseError   —— 字符串构造 JsonParser 时 JSON 文本本身非法
struct DefaultJsonParserErrorHandler {
    static void OnPathResolveError(const std::exception& e) {
        throw std::runtime_error(e.what());
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

template <typename Json, typename ErrorHandler, std::size_t n>
class JsonScope;

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

    const Json& json() const { return *stack_.top(); }

    template <template <typename> class Extractor,
            typename ExistencePolicy = Optional,
            std::size_t n,
            typename Func,
            typename T = Extractor<Json>::ValueType>
    JsonParser& Parse(const Parts<n>& path, Func&& callback, T default_val = {}) {
        static_assert(std::is_same_v<ExistencePolicy, Required> ||
                              std::is_same_v<ExistencePolicy, Optional>,
                "JsonParser: unsupported ExistencePolicy");
        using Val = Extractor<Json>::ValueType;
        static_assert(std::is_same_v<T, Val>,
                "JsonParser: default value type must match Extractor<Json>::ValueType");
        Val value = std::move(default_val);
        std::size_t actual = 0;
        const Json* node = Resolve<ExistencePolicy>(path, actual);
        if (!node) {
            // Required：Resolve 内已交给 OnPathResolveError（handler 返回时才走到这里）
            if constexpr (std::is_same_v<ExistencePolicy, Optional>) {
                std::forward<Func>(callback)(std::move(value));
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
            std::size_t n,
            typename T = Extractor<Json>::ValueType>
    JsonParser& Acquire(const Parts<n>& path, T& out, T default_val = {}) {
        static_assert(std::is_same_v<ExistencePolicy, Required> ||
                              std::is_same_v<ExistencePolicy, Optional>,
                "JsonParser: unsupported ExistencePolicy");
        using Val = Extractor<Json>::ValueType;
        static_assert(std::is_same_v<T, Val>,
                "JsonParser: Acquire target type must match Extractor<Json>::ValueType");
        Val value = std::move(default_val);
        std::size_t actual = 0;
        const Json* node = Resolve<ExistencePolicy>(path, actual);
        if (!node) {
            if constexpr (std::is_same_v<ExistencePolicy, Optional>) {
                out = std::move(value);
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
            std::size_t n,
            template <typename...> class List,
            typename T = Extractor<Json>::ValueType>
    JsonParser& AcquireArray(const Parts<n>& path, List<T>& arr) {
        static_assert(std::is_same_v<ExistencePolicy, Required> ||
                              std::is_same_v<ExistencePolicy, Optional>,
                "JsonParser: unsupported ExistencePolicy");
        static_assert(std::is_same_v<T, typename Extractor<Json>::ValueType>,
                "JsonParser: Acquire target type must match Extractor<Json>::ValueType");
        // 把一个json数组填入arr
        // ExistencePolicy只针对数组
        // 内部元素如果出现类型不匹配的情况, 直接调用OnTypeMismatch（需要组装完整路径，如 a.b[2]）
        std::size_t actual = 0;
        const Json* node = Resolve<ExistencePolicy>(path, actual);
        if (!node) {
            if constexpr (std::is_same_v<ExistencePolicy, Optional>) {
                arr = List<T>{};
            }
            return *this;
        }
        if (!JsonTraits<Json>::IsArray(*node)) {
            if constexpr (std::is_same_v<ExistencePolicy, Optional>) {
                arr = List<T>{};
            } else {
                error_handler_.OnTypeMismatch(PathString(path));
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
            std::size_t n,
            template <typename...> class Map,
            typename T = Extractor<Json>::ValueType>
    JsonParser& AcquireMap(const Parts<n>& path, Map<std::string, T>& map) {
        static_assert(std::is_same_v<ExistencePolicy, Required> ||
                              std::is_same_v<ExistencePolicy, Optional>,
                "JsonParser: unsupported ExistencePolicy");
        static_assert(std::is_same_v<T, typename Extractor<Json>::ValueType>,
                "JsonParser: Acquire target type must match Extractor<Json>::ValueType");
        // 把一个json对象填入map
        // ExistencePolicy只针对map
        // 内部元素如果出现类型不匹配的情况, 直接调用OnTypeMismatch（需要组装完整路径，如 a.b.c）
        std::size_t actual = 0;
        const Json* node = Resolve<ExistencePolicy>(path, actual);
        if (!node) {
            if constexpr (std::is_same_v<ExistencePolicy, Optional>) {
                map = Map<std::string, T>{};
            }
            return *this;
        }
        if (!JsonTraits<Json>::IsObject(*node)) {
            if constexpr (std::is_same_v<ExistencePolicy, Optional>) {
                map = Map<std::string, T>{};
            } else {
                error_handler_.OnTypeMismatch(PathString(path));
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

private:
    template <typename J, typename E, std::size_t m>
    friend class JsonScope;

    // 从 stack_ 顶部节点开始依次取路径段。
    // actual 返回实际解析成功的层数（失败时为已解析的前缀长度）。
    // ExistencePolicy == Required：解析失败时构造异常交给 OnPathResolveError，并返回 nullptr；
    // ExistencePolicy == Optional：解析失败直接返回 nullptr。
    template <typename ExistencePolicy, std::size_t n>
    const Json* Resolve(const Parts<n>& parts, std::size_t& actual) {
        actual = 0;
        const Json* node = stack_.top();
        for (std::size_t i = 0; i < parts.Size(); ++i) {
            const std::string_view token = parts[i];
            const Json* next = Step(node, token);
            if (!next) {
                if constexpr (std::is_same_v<ExistencePolicy, Required>) {
                    std::runtime_error e(ResolveErrorMessage(parts, actual, token));
                    error_handler_.OnPathResolveError(e);
                }
                return nullptr;
            }
            node = next;
            ++actual;
        }
        return node;
    }

    const Json* Step(const Json* node, std::string_view token) const {
        if (token.empty()) {
            return nullptr;
        }
        if (token.front() == '[') {
            if (token.size() < 2 || token.back() != ']') {
                return nullptr;
            }
            const std::string_view digits = token.substr(1, token.size() - 2);
            std::size_t index = 0;
            if (auto [ptr, ec] =
                        std::from_chars(digits.data(), digits.data() + digits.size(), index);
                    ec != std::errc() || ptr != digits.data() + digits.size()) {
                return nullptr;
            }
            if (!JsonTraits<Json>::IsArray(*node) ||
                    index >= JsonTraits<Json>::GetArraySize(*node)) {
                return nullptr;
            }
            return &JsonTraits<Json>::GetArrayElement(*node, index);
        }
        if (!JsonTraits<Json>::IsObject(*node) || !JsonTraits<Json>::HasKey(*node, token)) {
            return nullptr;
        }
        return &JsonTraits<Json>::GetChild(*node, token);
    }

    void Push(const Json* node) { stack_.push(node); }

    void PopOne() noexcept {
        if (stack_.size() > 1) {
            stack_.pop();
        }
    }

    // 把编译期路径还原成 "a.b[2].c" 形式的字符串，用于错误回调。
    template <std::size_t n>
    static std::string PathString(const Parts<n>& parts) {
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

    template <std::size_t n>
    static std::string ResolveErrorMessage(const Parts<n>& parts, std::size_t actual,
                                           std::string_view token) {
        return "JsonParser: cannot resolve path '" + PathString(parts) + "' at segment '" +
               std::string(token) + "' (resolved " + std::to_string(actual) + " of " +
               std::to_string(parts.Size()) + " segments)";
    }

    ErrorHandler error_handler_;
    JsonBuilder<Json, ErrorHandler> builder_;
    const Json& root_;
    std::stack<const Json*> stack_;
};

// RAII 作用域：作用域内所有路径都相对该节点解析，析构时自动回退。
// 解析协议固定为 Required —— 路径不合法时交给 OnPathResolveError；
// 若自定义 handler 吞掉异常，则作用域退化为空操作（不压栈、析构不回退）。
template <typename Json, typename ErrorHandler, std::size_t n>
class JsonScope {
public:
    JsonScope(JsonParser<Json, ErrorHandler>& parser, const Parts<n>& path)
        : parser_(parser) {
        if (auto node = parser_.template Resolve<Required>(path, actual_)) {
            parser_.Push(node);
            pushed_ = true;
        }
    }

    JsonScope(const JsonScope&) = delete;
    JsonScope& operator=(const JsonScope&) = delete;

    JsonScope(JsonScope&& other) noexcept
        : parser_(other.parser_), actual_(other.actual_), pushed_(other.pushed_) {
        other.pushed_ = false;
    }
    JsonScope& operator=(JsonScope&&) = delete;

    ~JsonScope() { Close(); }

    // 实际解析成功的层数（等于路径段数；handler 吞掉异常时为部分层数）
    [[nodiscard]] std::size_t Resolved() const noexcept { return actual_; }

private:
    void Close() noexcept {
        if (pushed_) {
            parser_.PopOne();
            pushed_ = false;
        }
    }

    JsonParser<Json, ErrorHandler>& parser_;
    std::size_t actual_ = 0;
    bool pushed_ = false;
};

// 让 JsonScope scope(parser, "a.b"_path); 直接成立（Json、ErrorHandler、n 全部可推导）。
template <typename Json, typename ErrorHandler, std::size_t n>
JsonScope(JsonParser<Json, ErrorHandler>&, const Parts<n>&) -> JsonScope<Json, ErrorHandler, n>;

}  // namespace sc::json

#endif  // SCCOMMON_JSON_PARSER_N_H
