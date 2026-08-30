//
// Created by PC on 2026/8/30.
//

#ifndef SCCOMMON_JSON_TRAITS_RAPIDJSON_H
#define SCCOMMON_JSON_TRAITS_RAPIDJSON_H

#include <rapidjson/document.h>

#include <cstddef>
#include <string>
#include <string_view>

#include "../json_traits.h"

// RapidJSON 适配：JsonType = rapidjson::Value。
// rapidjson::Document 是 Value 的子类，根节点可直接绑定到 JsonParser<rapidjson::Value>。
template <>
struct sc::common::json::JsonTraits<rapidjson::Value> {
    using Json = rapidjson::Value;

    static bool IsObject(const Json& node) { return node.IsObject(); }

    static bool IsArray(const Json& node) { return node.IsArray(); }

    static bool IsNumberInteger(const Json& node) {
        return node.IsInt() || node.IsUint() || node.IsInt64() || node.IsUint64();
    }

    static bool IsDouble(const Json& node) { return node.IsDouble(); }

    static bool IsString(const Json& node) { return node.IsString(); }

    static bool IsBool(const Json& node) { return node.IsBool(); }

    static bool HasKey(const Json& node, std::string_view key) {
        return node.HasMember(std::string(key).c_str());
    }

    static const Json& GetChild(const Json& node, std::string_view key) {
        return node[std::string(key).c_str()];
    }

    static std::size_t GetArraySize(const Json& node) { return node.Size(); }

    static const Json& GetArrayElement(const Json& node, std::size_t index) {
        return node[static_cast<rapidjson::SizeType>(index)];
    }

    static int GetInt(const Json& node) { return node.GetInt(); }

    static double GetDouble(const Json& node) { return node.GetDouble(); }

    static std::string GetString(const Json& node) { return {node.GetString()}; }

    static bool GetBool(const Json& node) { return node.GetBool(); }
};  // namespace sc::common::json

#endif  // SCCOMMON_JSON_TRAITS_RAPIDJSON_H
