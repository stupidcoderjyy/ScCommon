//
// Created by PC on 2026/8/30.
//

#ifndef SCCOMMON_NLOHMANN_H
#define SCCOMMON_NLOHMANN_H

#include <nlohmann/json.hpp>

#include "../json_traits.h"

template <>
struct sc::common::json::JsonTraits<nlohmann::json> {
    using Json = nlohmann::json;

    static bool IsObject(const Json& node) { return node.is_object(); }

    static bool IsArray(const Json& node) { return node.is_array(); }

    static bool IsNumberInteger(const Json& node) {
        return node.is_number_integer() || node.is_number_unsigned();
    }

    static bool IsDouble(const Json& node) { return node.is_number_float(); }

    static bool IsString(const Json& node) { return node.is_string(); }

    static bool IsBool(const Json& node) { return node.is_boolean(); }

    static bool HasKey(const Json& node, std::string_view key) {
        return node.contains(std::string(key));
    }

    static const Json& GetChild(const Json& node, std::string_view key) {
        return node.at(std::string(key));
    }

    static std::size_t GetArraySize(const Json& node) { return node.size(); }

    static const Json& GetArrayElement(const Json& node, std::size_t index) {
        return node.at(index);
    }

    static int GetInt(const Json& node) { return node.get<int>(); }

    static double GetDouble(const Json& node) { return node.get<double>(); }

    static std::string GetString(const Json& node) {
        return node.get<std::string>();
    }

    static bool GetBool(const Json& node) { return node.get<bool>(); }
};

#endif  // SCCOMMON_NLOHMANN_H
