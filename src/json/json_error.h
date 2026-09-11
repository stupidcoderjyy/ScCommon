//
// Created by jiangyuyun3 on 2026/9/11.
//

#ifndef SCCOMMON_JSON_ERROR_H
#define SCCOMMON_JSON_ERROR_H

#include <exception>
#include <stdexcept>
#include <string>
#include <string_view>

namespace sc::json {

// 错误处理协议：
//   OnPathResolveError —— 路径解析失败（缺失 key、下标越界、路径中途类型不符等）
//   OnTypeMismatch     —— 路径解析成功，但节点无法转换为目标类型
//   OnJsonParseError   —— 字符串构造 JsonParser 时 JSON 文本本身非法
struct DefaultJsonParserErrorHandler {
    static void OnPathResolveError(const std::exception& e) { throw std::runtime_error(e.what()); }
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

}  // namespace sc::json

#endif  // SCCOMMON_JSON_ERROR_H
