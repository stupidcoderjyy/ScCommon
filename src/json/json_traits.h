//
// Created by PC on 2026/8/30.
//

#ifndef SCCOMMON_JSON_TRAITS_H
#define SCCOMMON_JSON_TRAITS_H
#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>

namespace sc::json {

// JsonTraits 是对不同 JSON 库的最小静态访问接口（命名遵循 CamelCase）。
// 适配一个新 JSON 库时，只需为 JsonTraits<JsonType> 提供显式特化。

//NOLINTBEGIN
template <typename Json>
struct JsonTraits {
    static bool IsObject(const Json&);
    static bool IsArray(const Json&);
    static bool IsInt(const Json&);
    static bool IsUint(const Json&);
    static bool IsInt64(const Json&);
    static bool IsUint64(const Json&);
    static bool IsDouble(const Json&);
    static bool IsString(const Json&);
    static bool IsBool(const Json&);

    static bool HasKey(const Json&, std::string_view key);
    static const Json& GetChild(const Json&, std::string_view key);
    static std::size_t GetArraySize(const Json&);
    static const Json& GetArrayElement(const Json&, std::size_t index);
    // 遍历对象成员：func(key, value)；func 返回 false 时提前终止。
    template <typename Func>
    static void ForEachMember(const Json&, Func&&);

    static int GetInt(const Json&);
    static unsigned int GetUint(const Json&);
    static std::int64_t GetInt64(const Json&);
    static std::uint64_t GetUint64(const Json&);
    static double GetDouble(const Json&);
    static std::string GetString(const Json&);
    static bool GetBool(const Json&);
};

// JsonBuilder：从 string_view 解析出 Json，并持有解析结果（含所需分配器），
// 其生命周期随持有它的 JsonParser；解析失败时调用 ErrorHandler::OnJsonParseError。
template <typename Json, typename ErrorHandler>
class JsonBuilder {
public:
    JsonBuilder() = default;
    JsonBuilder(std::string_view json, ErrorHandler error_handler = {});
    JsonBuilder(const JsonBuilder&) = delete;
    JsonBuilder(JsonBuilder&&) = delete;
    JsonBuilder& operator=(const JsonBuilder&) = delete;
    JsonBuilder& operator=(JsonBuilder&&) = delete;

    const Json& json() const { return json_; }

private:
    Json json_;
};

//NOLINTEND

}  // namespace sc::json

#endif  // SCCOMMON_JSON_TRAITS_H
