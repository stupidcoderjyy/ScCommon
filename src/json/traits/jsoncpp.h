//
// Created by PC on 2026/8/30.
//

#ifndef SCCOMMON_JSON_TRAITS_JSONCPP_H
#define SCCOMMON_JSON_TRAITS_JSONCPP_H

#include <json/json.h>

#include <cstddef>
#include <cstdint>
#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>

#include "../json_traits.h"

namespace sc::json {
// JsonCpp 适配：JsonType = Json::Value。
template <>
struct JsonTraits<Json::Value> {
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

    static bool HasKey(const Json& json, std::string_view key) {
        return json.isMember(std::string(key));
    }
    static const Json& GetChild(const Json& json, std::string_view key) {
        return json[std::string(key)];
    }
    static const Json* TryGetChild(const Json& json, std::string_view key) {
        if (!json.isObject()) {
            return nullptr;
        }
        // 指针区间版 find：不构造临时 std::string，命中返回节点指针
        return json.find(key.data(), key.data() + key.size());
    }
    static std::size_t GetArraySize(const Json& json) { return json.size(); }
    static const Json& GetArrayElement(const Json& json, std::size_t index) {
        return json[static_cast<Json::ArrayIndex>(index)];
    }
    template <typename Func>
    static void ForEachMember(const Json& json, Func&& func) {
        // 直接迭代，避免 getMemberNames() 每次返回 vector<string> 的分配与键拷贝
        for (auto it = json.begin(); it != json.end(); ++it) {
            const std::string key = it.name();
            if (!func(std::string_view(key), *it)) {
                break;
            }
        }
    }

    static int GetInt(const Json& json) { return json.asInt(); }
    static unsigned int GetUint(const Json& json) { return json.asUInt(); }
    static std::int64_t GetInt64(const Json& json) { return json.asInt64(); }
    static std::uint64_t GetUint64(const Json& json) { return json.asUInt64(); }
    static double GetDouble(const Json& json) { return json.asDouble(); }
    static std::string GetString(const Json& json) { return json.asString(); }
    static bool GetBool(const Json& json) { return json.asBool(); }
};  // namespace sc::json

template <typename ErrorHandler>
class JsonBuilder<Json::Value, ErrorHandler> {
public:
    JsonBuilder() = default;

    explicit JsonBuilder(std::string_view json, ErrorHandler error_handler = {}) {
        Json::CharReaderBuilder builder;
        std::unique_ptr<Json::CharReader> reader(builder.newCharReader());
        Json::Value parsed;
        std::string errors;
        if (!reader->parse(json.data(), json.data() + json.size(), &parsed, &errors)) {
            std::runtime_error e("jsoncpp parse error: " + errors);
            error_handler.OnJsonParseError(json, e);
            throw std::move(e);
        }
        json_ = std::move(parsed);
    }

    JsonBuilder(const JsonBuilder&) = delete;
    JsonBuilder(JsonBuilder&&) = delete;
    JsonBuilder& operator=(const JsonBuilder&) = delete;
    JsonBuilder& operator=(JsonBuilder&&) = delete;

    [[nodiscard]] const Json::Value& json() const { return json_; }

private:
    Json::Value json_;
};

}  // namespace sc::json

#endif  // SCCOMMON_JSON_TRAITS_JSONCPP_H
