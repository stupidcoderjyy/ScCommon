//
// Created by jiangyuyun3 on 2026/9/11.
//

#ifndef SCCOMMON_JSON_DETAIL_H
#define SCCOMMON_JSON_DETAIL_H

#include "json_path.h"
#include "json_traits.h"

#include <charconv>
#include <cstddef>
#include <string>
#include <string_view>

namespace sc::json::detail {

// 单步解析：token 以 '[' 开头按数组下标处理，否则按对象 key 处理。
// 供 JsonParser 与各 View 共用。
template <typename Json>
const Json* Step(const Json* node, std::string_view token) {
    if (!node || token.empty()) {
        return nullptr;
    }
    if (token.front() == '[') {
        if (token.size() < 2 || token.back() != ']') {
            return nullptr;
        }
        const std::string_view digits = token.substr(1, token.size() - 2);
        std::size_t index = 0;
        if (auto [ptr, ec] = std::from_chars(digits.data(), digits.data() + digits.size(), index);
                ec != std::errc() || ptr != digits.data() + digits.size()) {
            return nullptr;
        }
        if (!JsonTraits<Json>::IsArray(*node) || index >= JsonTraits<Json>::GetArraySize(*node)) {
            return nullptr;
        }
        return &JsonTraits<Json>::GetArrayElement(*node, index);
    }
    return JsonTraits<Json>::TryGetChild(*node, token);
}

inline void AppendNumber(std::string& out, std::size_t value) {
    char buf[24];
    if (const auto [end, ec] = std::to_chars(buf, buf + sizeof(buf), value); ec == std::errc()) {
        out.append(buf, end);
    }
}

// 把编译期路径还原成 "a.b[2].c" 形式并追加到 out。
template <std::size_t n>
void AppendPath(std::string& out, const Parts<n>& parts) {
    for (std::size_t i = 0; i < parts.Size(); ++i) {
        const std::string_view token = parts[i];
        if (const bool is_index = !token.empty() && token.front() == '[';
                !is_index && !out.empty()) {
            out += '.';
        }
        out += token;
    }
}

template <std::size_t n>
std::string PathString(const Parts<n>& parts) {
    std::string out;
    AppendPath(out, parts);
    return out;
}

// 失败信息一次成串（预留容量 + 追加），避免多次临时字符串分配。
template <std::size_t n>
std::string ResolveErrorMessage(const Parts<n>& parts, std::size_t actual, std::string_view token) {
    std::string out;
    out.reserve(64 + parts.Size() * 8 + token.size());
    out += "JsonParser: cannot resolve path '";
    AppendPath(out, parts);
    out += "' at segment '";
    out.append(token);
    out += "' (resolved ";
    AppendNumber(out, actual);
    out += " of ";
    AppendNumber(out, parts.Size());
    out += " segments)";
    return out;
}

}  // namespace sc::json::detail

#endif  // SCCOMMON_JSON_DETAIL_H
