//
// Created by jiangyuyun3 on 2026/9/8.
//

#ifndef SCCOMMON_NLOHMANN_N_H
#define SCCOMMON_NLOHMANN_N_H
#include "../json_traits.h"

#include <exception>
#include <limits>
#include <nlohmann/json.hpp>

template <>
struct sc::json::JsonTraits<nlohmann::json> {
    using Json = nlohmann::json;

    static bool IsObject(const Json& json) { return json.is_object(); }
    static bool IsArray(const Json& json) { return json.is_array(); }
    // Is<整型> 语义：JSON 整数值能无损转换到目标类型即为真（不要求存储类型一致）。
    static bool IsInt(const Json& json) {
        if (json.is_number_unsigned()) {
            return json.get<std::uint64_t>() <=
                   static_cast<std::uint64_t>(std::numeric_limits<int>::max());
        }
        if (json.is_number_integer()) {
            const std::int64_t v = json.get<std::int64_t>();
            return v >= std::numeric_limits<int>::min() && v <= std::numeric_limits<int>::max();
        }
        return false;
    }
    static bool IsUint(const Json& json) {
        if (json.is_number_unsigned()) {
            return json.get<std::uint64_t>() <=
                   static_cast<std::uint64_t>(std::numeric_limits<unsigned int>::max());
        }
        if (json.is_number_integer()) {
            const std::int64_t v = json.get<std::int64_t>();
            return v >= 0 &&
                   v <= static_cast<std::int64_t>(std::numeric_limits<unsigned int>::max());
        }
        return false;
    }
    static bool IsInt64(const Json& json) {
        if (json.is_number_unsigned()) {
            return json.get<std::uint64_t>() <=
                   static_cast<std::uint64_t>((std::numeric_limits<std::int64_t>::max)());
        }
        return json.is_number_integer();
    }
    static bool IsUint64(const Json& json) {
        if (json.is_number_integer()) {
            return json.get<std::int64_t>() >= 0;
        }
        return json.is_number_unsigned();
    }
    static bool IsDouble(const Json& json) { return json.is_number_float(); }
    static bool IsString(const Json& json) { return json.is_string(); }
    static bool IsBool(const Json& json) { return json.is_boolean(); }

    static bool HasKey(const Json& json, std::string_view key) {
        return json.contains(std::string(key));
    }
    static const Json& GetChild(const Json& json, std::string_view key) {
        return json.at(std::string(key));
    }
    static const Json* TryGetChild(const Json& json, std::string_view key) {
        if (!json.is_object()) {
            return nullptr;
        }
        const auto it = json.find(std::string(key));
        return it == json.end() ? nullptr : &*it;
    }
    static std::size_t GetArraySize(const Json& json) { return json.size(); }
    static const Json& GetArrayElement(const Json& json, std::size_t index) {
        return json.at(index);
    }
    template <typename Func>
    static void ForEachMember(const Json& json, Func&& func) {
        for (auto it = json.begin(); it != json.end(); ++it) {
            if (!func(std::string_view(it.key()), it.value())) {
                break;
            }
        }
    }

    static int GetInt(const Json& json) { return json.get<int>(); }
    static unsigned int GetUint(const Json& json) { return json.get<unsigned int>(); }
    static std::int64_t GetInt64(const Json& json) { return json.get<std::int64_t>(); }
    static std::uint64_t GetUint64(const Json& json) { return json.get<std::uint64_t>(); }
    static double GetDouble(const Json& json) { return json.get<double>(); }
    static std::string GetString(const Json& json) { return json.get<std::string>(); }
    static bool GetBool(const Json& json) { return json.get<bool>(); }
};

template <typename ErrorHandler>
class sc::json::JsonBuilder<nlohmann::json, ErrorHandler> {
public:
    JsonBuilder() = default;

    explicit JsonBuilder(std::string_view json, ErrorHandler error_handler = {}) {
        try {
            json_ = nlohmann::json::parse(json.data(), json.data() + json.size());
        } catch (std::exception& e) {
            error_handler.OnJsonParseError(json, e);
            throw;
        }
    }

    JsonBuilder(const JsonBuilder&) = delete;
    JsonBuilder(JsonBuilder&&) = delete;
    JsonBuilder& operator=(const JsonBuilder&) = delete;
    JsonBuilder& operator=(JsonBuilder&&) = delete;

    [[nodiscard]] const nlohmann::json& json() const { return json_; }

private:
    nlohmann::json json_;
};

#endif  //SCCOMMON_NLOHMANN_N_H
