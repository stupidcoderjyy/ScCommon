//
// Created by PC on 2026/8/30.
//

#ifndef SCCOMMON_JSON_EXTRACTORS_RAPIDJSON_H
#define SCCOMMON_JSON_EXTRACTORS_RAPIDJSON_H

#include <rapidjson/document.h>

#include <vector>

#include "../json_extractor.h"

namespace sc::common::json {

// RapidJSON 提取器适配：JsonType = rapidjson::Value。
// rapidjson::Value 的默认分配器是 MemoryPoolAllocator，深拷贝出的 Value
// 内存归内存池管理，无法安全地自持，因此 JsonArray 采用零拷贝指针视图：
// 元素指向源文档中的节点（源文档须比结果存活更久，这与 JsonParser 只读
// 引用根节点的线程安全模型一致）。
template <>
struct JsonArrayContainer<rapidjson::Value> {
    using type = std::vector<const rapidjson::Value*>;
};

template <>
struct Extractor<rapidjson::Value, std::vector<const rapidjson::Value*>> {
    using value_type = std::vector<const rapidjson::Value*>;

    static bool Extract(const rapidjson::Value& node, value_type& out) {
        if (!node.IsArray()) {
            return false;
        }
        out.reserve(node.Size());
        for (rapidjson::SizeType i = 0; i < node.Size(); ++i) {
            out.push_back(&node[i]);
        }
        return true;
    }
};

}

#endif  // SCCOMMON_JSON_EXTRACTORS_RAPIDJSON_H
