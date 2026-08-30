//
// Created by PC on 2026/8/30.
//

#ifndef SCCOMMON_JSON_EXTRACTOR_H
#define SCCOMMON_JSON_EXTRACTOR_H

#include <cstddef>
#include <string>
#include <vector>

#include "json_traits.h"

namespace sc::common::json {

// 提取器接口。自定义提取器需提供：
//   using value_type = T;
//   static bool Extract(const JsonType&, T& out);
template <typename Json, typename T>
struct Extractor;

template <typename Json>
struct Extractor<Json, int> {
    using value_type = int;

    static bool Extract(const Json& node, int& out) {
        if (!JsonTraits<Json>::IsNumberInteger(node)) {
            return false;
        }
        out = JsonTraits<Json>::GetInt(node);
        return true;
    }
};

template <typename Json>
struct Extractor<Json, double> {
    using value_type = double;

    static bool Extract(const Json& node, double& out) {
        if (!JsonTraits<Json>::IsDouble(node)) {
            return false;
        }
        out = JsonTraits<Json>::GetDouble(node);
        return true;
    }
};

template <typename Json>
struct Extractor<Json, std::string> {
    using value_type = std::string;

    static bool Extract(const Json& node, std::string& out) {
        if (!JsonTraits<Json>::IsString(node)) {
            return false;
        }
        out = JsonTraits<Json>::GetString(node);
        return true;
    }
};

template <typename Json>
struct Extractor<Json, bool> {
    using value_type = bool;

    static bool Extract(const Json& node, bool& out) {
        if (!JsonTraits<Json>::IsBool(node)) {
            return false;
        }
        out = JsonTraits<Json>::GetBool(node);
        return true;
    }
};

// JsonArray 递归存储 JSON 节点：value_type = std::vector<JsonType>。
template <typename Json>
struct Extractor<Json, std::vector<Json>> {
    using value_type = std::vector<Json>;

    static bool Extract(const Json& node, std::vector<Json>& out) {
        if (!JsonTraits<Json>::IsArray(node)) {
            return false;
        }
        const std::size_t size = JsonTraits<Json>::GetArraySize(node);
        out.reserve(size);
        for (std::size_t i = 0; i < size; ++i) {
            out.push_back(JsonTraits<Json>::GetArrayElement(node, i));
        }
        return true;
    }
};

// 预定义提取器。
template <typename Json>
using JsonInt = Extractor<Json, int>;

template <typename Json>
using JsonDouble = Extractor<Json, double>;

template <typename Json>
using JsonString = Extractor<Json, std::string>;

template <typename Json>
using JsonBool = Extractor<Json, bool>;

// 数组容器定制点：默认直接存放可拷贝的 JSON 节点（std::vector<Json>）。
// 对不可安全拷贝的库（如 rapidjson），可特化此模板改用零拷贝视图
//（如 std::vector<const Json*>），使 JsonArray 提取器随库自适应。
template <typename Json>
struct JsonArrayContainer {
    using type = std::vector<Json>;
};

template <typename Json>
using JsonArray = Extractor<Json, typename JsonArrayContainer<Json>::type>;

// JsonObject：自定义对象映射。先特化 Extractor<JsonType, MyObject>，
// 再通过 Extractor<JsonType, MyObject>（或其别名）作为提取器使用。
template <typename Json, typename T>
using JsonObject = Extractor<Json, T>;

}  // namespace sc::common::json

#endif  // SCCOMMON_JSON_EXTRACTOR_H
