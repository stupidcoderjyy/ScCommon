//
// Created by PC on 2026/8/30.
//

#ifndef SCCOMMON_JSON_TRAITS_JSONCPP_H
#define SCCOMMON_JSON_TRAITS_JSONCPP_H

#include <json/json.h>

#include <cstddef>
#include <string>
#include <string_view>

#include "../json_traits.h"

// JsonCpp 适配：JsonType = Json::Value。
template <>
struct sc::common::json::JsonTraits<Json::Value> {
    using Json = Json::Value;

    static bool IsObject(const Json& node) { return node.isObject(); }

    static bool IsArray(const Json& node) { return node.isArray(); }

    static bool IsNumberInteger(const Json& node) {
        return node.isInt() || node.isUInt() || node.isInt64() || node.isUInt64();
    }

    static bool IsDouble(const Json& node) { return node.isDouble(); }

    static bool IsString(const Json& node) { return node.isString(); }

    static bool IsBool(const Json& node) { return node.isBool(); }

    static bool HasKey(const Json& node, std::string_view key) {
        return node.isMember(std::string(key));
    }

    static const Json& GetChild(const Json& node, std::string_view key) {
        return node[std::string(key)];
    }

    static std::size_t GetArraySize(const Json& node) { return node.size(); }

    static const Json& GetArrayElement(const Json& node, std::size_t index) {
        return node[static_cast<Json::ArrayIndex>(index)];
    }

    static int GetInt(const Json& node) { return node.asInt(); }

    static double GetDouble(const Json& node) { return node.asDouble(); }

    static std::string GetString(const Json& node) { return node.asString(); }

    static bool GetBool(const Json& node) { return node.asBool(); }
};  // namespace sc::common::json

#endif  // SCCOMMON_JSON_TRAITS_JSONCPP_H
