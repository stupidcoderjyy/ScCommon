//
// Created by PC on 2026/8/30.
//

#ifndef SCCOMMON_JSON_TRAITS_JSONCPP_H
#define SCCOMMON_JSON_TRAITS_JSONCPP_H

#include <json/json.h>

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>

#include "../json_traits.h"

// JsonCpp 适配：JsonType = Json::Value。
template <>
struct sc::json::JsonTraits<Json::Value> {
    using Json = Json::Value;

    static bool IsObject(const Json& json) { return json.isObject(); }
    static bool IsArray(const Json& json) { return json.isArray(); }
    static bool IsInt(const Json& json) { return json.isInt(); }
    static bool IsUint(const Json& json) { return json.isUInt(); }
    static bool IsInt64(const Json& json) { return json.isInt64(); }
    static bool IsUint64(const Json& json) { return json.isUInt64(); }
    static bool IsDouble(const Json& json) { return json.isDouble(); }
    static bool IsString(const Json& json) { return json.isString(); }
    static bool IsBool(const Json& json) { return json.isBool(); }

    static bool HasKey(const Json& json, std::string_view key) { return json.isMember(std::string(key)); }
    static const Json& GetChild(const Json& json, std::string_view key) {
        return json[std::string(key)];
    }
    static std::size_t GetArraySize(const Json& json) { return json.size(); }
    static const Json& GetArrayElement(const Json& json, std::size_t index) { return json[static_cast<Json::ArrayIndex>(index)]; }

    static int GetInt(const Json& json) { return json.asInt(); }
    static unsigned int GetUint(const Json& json) { return json.asUInt(); }
    static std::int64_t GetInt64(const Json& json) { return json.asInt64(); }
    static std::uint64_t GetUint64(const Json& json) { return json.asUInt64(); }
    static double GetDouble(const Json& json) { return json.asDouble(); }
    static std::string GetString(const Json& json) { return json.asString(); }
    static bool GetBool(const Json& json) { return json.asBool(); }
};  // namespace sc::json

#endif  // SCCOMMON_JSON_TRAITS_JSONCPP_H
