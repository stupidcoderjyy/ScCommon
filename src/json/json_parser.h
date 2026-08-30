//
// Created by PC on 2026/8/30.
//

#ifndef SCCOMMON_JSON_PARSER_H
#define SCCOMMON_JSON_PARSER_H
#include <cstddef>
#include <limits>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>

#include "json_extractor.h"
#include "json_traits.h"

// 自动适配：先包含目标 JSON 库头文件，再包含本文件。
// 不同版本的 nlohmann 头文件保护宏不同（NLOHMANN_JSON_HPP / INCLUDE_NLOHMANN_JSON_HPP_），
// 这里同时检测，并用 __has_include 兜底。
#if defined(NLOHMANN_JSON_HPP) || defined(INCLUDE_NLOHMANN_JSON_HPP_) || \
    __has_include(<nlohmann/json.hpp>)
#include "traits/nlohmann.h"
#endif
#if defined(RAPIDJSON_DOCUMENT_H_) || __has_include(<rapidjson/document.h>)
#include "traits/rapidjson.h"
#include "extractors/rapidjson.h"
#endif
#if defined(JSON_JSON_H_INCLUDED) || __has_include(<json/json.h>)
#include "traits/jsoncpp.h"
#endif

namespace sc::common::json {

// 存在性策略。
struct Required {};

struct Optional {};

// 默认错误处理器：路径缺失或类型不匹配时抛出 std::runtime_error。
// 可通过 JsonParser 的第二个模板参数替换为自定义错误处理器。
struct ThrowErrorHandler {
    static void OnMissing(std::string_view path) {
        throw std::runtime_error("JsonParser: path '" + std::string(path) + "' not found");
    }

    static void OnTypeMismatch(std::string_view path) {
        throw std::runtime_error("JsonParser: type mismatch at path '" + std::string(path) + "'");
    }
};

// 轻量级、模板化的 JSON 路径解析器。只持有 const 引用，解析过程不修改数据。
template <typename Json, typename ErrorHandler = ThrowErrorHandler>
class JsonParser {
public:
    explicit JsonParser(const Json& root) : root_(root) {}

    // 解析路径指向的节点：Required 缺失/类型不匹配时抛出异常，
    // Optional 则静默跳过；成功时以提取结果调用 callback，支持链式调用。
    template <template <typename> class ExtractorT, typename ExistencePolicy, typename Func>
    JsonParser& Parse(std::string_view path, Func&& callback) {
        const Json* node = Resolve(path);
        if (node == nullptr) {
            if constexpr (std::is_same_v<ExistencePolicy, Optional>) {
                return *this;
            } else {
                ErrorHandler::OnMissing(path);
                return *this;
            }
        }

        using Value = ExtractorT<Json>::value_type;
        Value value{};
        if (!ExtractorT<Json>::Extract(*node, value)) {
            if constexpr (std::is_same_v<ExistencePolicy, Optional>) {
                return *this;
            } else {
                ErrorHandler::OnTypeMismatch(path);
                return *this;
            }
        }

        std::forward<Func>(callback)(std::move(value));
        return *this;
    }

private:
    const Json& root_;

    // 将点号分隔 + 数组下标路径解析为节点指针；解析失败返回 nullptr。
    const Json* Resolve(std::string_view path) const {
        const Json* current = &root_;
        std::size_t pos = 0;
        while (true) {
            const std::size_t dot = path.find('.', pos);
            if (const std::size_t end = dot == std::string_view::npos ? path.size() : dot; !ApplySegment(current, path.substr(pos, end - pos))) {
                return nullptr;
            }
            if (dot == std::string_view::npos) {
                break;
            }
            pos = dot + 1;
        }
        return current;
    }

    // 处理单个路径段：可选的对象 key 与任意数量的 [index] 下标。
    bool ApplySegment(const Json*& current, std::string_view segment) const {
        std::size_t bracket = segment.find('[');
        if (const std::string_view key = segment.substr(0, bracket); !key.empty()) {
            if (!JsonTraits<Json>::IsObject(*current) || !JsonTraits<Json>::HasKey(*current, key)) {
                return false;
            }
            current = &JsonTraits<Json>::GetChild(*current, key);
        }

        while (bracket != std::string_view::npos) {
            const std::size_t close = segment.find(']', bracket + 1);
            if (close == std::string_view::npos) {
                return false;
            }
            const std::string_view digits = segment.substr(bracket + 1, close - bracket - 1);
            if (digits.empty()) {
                return false;
            }
            std::size_t index = 0;
            for (const char c : digits) {
                if (c < '0' || c > '9') {
                    return false;
                }
                const auto digit = static_cast<std::size_t>(c - '0');
                if (index > (std::numeric_limits<std::size_t>::max() - digit) / 10) {
                    return false;
                }
                index = index * 10 + digit;
            }
            if (!JsonTraits<Json>::IsArray(*current) || index >= JsonTraits<Json>::GetArraySize(*current)) {
                return false;
            }
            current = &JsonTraits<Json>::GetArrayElement(*current, index);
            bracket = segment.find('[', close + 1);
        }
        return true;
    }
};

}  // namespace sc::common::json

#endif  // SCCOMMON_JSON_PARSER_H
