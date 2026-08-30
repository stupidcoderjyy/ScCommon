//
// Created by PC on 2026/8/30.
//

#ifndef SCCOMMON_JSON_TRAITS_H
#define SCCOMMON_JSON_TRAITS_H
#include <cstddef>
#include <string>
#include <string_view>

namespace sc::common::json {

// JsonTraits 是对不同 JSON 库的最小静态访问接口（命名遵循 CamelCase）。
// 适配一个新 JSON 库时，只需为 JsonTraits<JsonType> 提供显式特化。
template<typename Json>
struct JsonTraits {
    static bool IsObject(const Json&);
    static bool IsArray(const Json&);
    static bool IsNumberInteger(const Json&);
    static bool IsDouble(const Json&);
    static bool IsString(const Json&);
    static bool IsBool(const Json&);

    static bool HasKey(const Json&, std::string_view key);
    static const Json& GetChild(const Json&, std::string_view key);
    static std::size_t GetArraySize(const Json&);
    static const Json& GetArrayElement(const Json&, std::size_t index);

    static int GetInt(const Json&);
    static double GetDouble(const Json&);
    static std::string GetString(const Json&);
    static bool GetBool(const Json&);
};

}  // namespace sc::common::json

#endif  // SCCOMMON_JSON_TRAITS_H
