//
// Created by jiangyuyun3 on 2026/9/7.
//

#ifndef SCCOMMON_JSON_EXTRACTOR_N_H
#define SCCOMMON_JSON_EXTRACTOR_N_H

#include "json_traits.h"

#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <vector>

namespace sc::json {

template <typename Json, typename T>
struct Extractor {
    using ValueType = T;
    static bool Extract(const Json& json, T& val);
};

template <typename Json, typename T>
using JsonElement = Extractor<Json, T>;

template <typename Json>
using JsonInt = JsonElement<Json, int>;

template <typename Json>
using JsonInt64 = JsonElement<Json, std::int64_t>;

template <typename Json>
using JsonDouble = JsonElement<Json, double>;

template <typename Json>
using JsonString = JsonElement<Json, std::string>;

template <typename Json>
using JsonBool = JsonElement<Json, bool>;

// 数组元素为零拷贝指针视图。
template <typename Json>
using JsonArray = JsonElement<Json, std::vector<const Json*>>;

// 对象节点为零拷贝指针视图。
template <typename Json>
using JsonObject = JsonElement<Json, const Json*>;

template <typename Json, typename T>
bool Extractor<Json, T>::Extract(const Json& json, T& val) {
    if constexpr (std::is_same_v<T, int>) {
        if (!JsonTraits<Json>::IsInt(json)) return false;
        val = JsonTraits<Json>::GetInt(json);
    } else if constexpr (std::is_same_v<T, unsigned int>) {
        if (!JsonTraits<Json>::IsUint(json)) return false;
        val = JsonTraits<Json>::GetUint(json);
    } else if constexpr (std::is_same_v<T, std::int64_t>) {
        if (!JsonTraits<Json>::IsInt64(json)) return false;
        val = JsonTraits<Json>::GetInt64(json);
    } else if constexpr (std::is_same_v<T, std::uint64_t>) {
        if (!JsonTraits<Json>::IsUint64(json)) return false;
        val = JsonTraits<Json>::GetUint64(json);
    } else if constexpr (std::is_same_v<T, double>) {
        if (!JsonTraits<Json>::IsDouble(json)) return false;
        val = JsonTraits<Json>::GetDouble(json);
    } else if constexpr (std::is_same_v<T, std::string>) {
        if (!JsonTraits<Json>::IsString(json)) return false;
        val = JsonTraits<Json>::GetString(json);
    } else if constexpr (std::is_same_v<T, bool>) {
        if (!JsonTraits<Json>::IsBool(json)) return false;
        val = JsonTraits<Json>::GetBool(json);
    } else if constexpr (std::is_same_v<T, const Json*>) {
        if (!JsonTraits<Json>::IsObject(json)) return false;
        val = &json;
    } else if constexpr (std::is_same_v<T, std::vector<const Json*>>) {
        if (!JsonTraits<Json>::IsArray(json)) return false;
        const std::size_t size = JsonTraits<Json>::GetArraySize(json);
        val.reserve(size);
        for (std::size_t i = 0; i < size; ++i) {
            val.push_back(&JsonTraits<Json>::GetArrayElement(json, i));
        }
    } else {
        throw std::runtime_error("Unsupported json type");
    }
    return true;
}

}  // namespace sc::json

#endif  // SCCOMMON_JSON_EXTRACTOR_N_H
