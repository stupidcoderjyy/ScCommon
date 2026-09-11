//
// Created by jiangyuyun3 on 2026/9/11.
//

#include <gtest/gtest.h>

#include <cstddef>
#include <string>
#include <type_traits>
#include <vector>

#include "json/json_path.h"

using namespace sc::json;

namespace {

template <std::size_t N>
std::vector<std::string> ToStrings(const Parts<N>& parts) {
    std::vector<std::string> tokens;
    tokens.reserve(parts.Size());
    for (std::size_t i = 0; i < parts.Size(); ++i) {
        tokens.emplace_back(parts[i]);
    }
    return tokens;
}

}  // namespace

TEST(JsonPath, SpecExample) {
    // "a.b[2].c.d" -> {"a", "b", "[2]", "c", "d"}
    constexpr auto path = "a.b[2].c.d"_path;
    static_assert(path.Size() == 5);
    static_assert(std::is_same_v<std::remove_cvref_t<decltype(path)>, Parts<5>>);
    EXPECT_EQ((std::vector<std::string>{"a", "b", "[2]", "c", "d"}), ToStrings(path));
}

TEST(JsonPath, TokensAreAccessibleAtCompileTime) {
    constexpr auto path = "a.b[2].c.d"_path;
    static_assert(path[0] == "a");
    static_assert(path[1] == "b");
    static_assert(path[2] == "[2]");
    static_assert(path[3] == "c");
    static_assert(path[4] == "d");
    SUCCEED();
}

TEST(JsonPath, NestedKeysOnly) {
    constexpr auto path = "a.b.c"_path;
    static_assert(path.Size() == 3);
    EXPECT_EQ((std::vector<std::string>{"a", "b", "c"}), ToStrings(path));
}

TEST(JsonPath, SingleKey) {
    constexpr auto path = "model_source"_path;
    static_assert(path.Size() == 1);
    EXPECT_EQ((std::vector<std::string>{"model_source"}), ToStrings(path));
}

TEST(JsonPath, RootArrayIndex) {
    constexpr auto path = "[0].a"_path;
    static_assert(path.Size() == 2);
    static_assert(path[0] == "[0]");
    EXPECT_EQ((std::vector<std::string>{"[0]", "a"}), ToStrings(path));
}

TEST(JsonPath, NestedArrayIndex) {
    constexpr auto path = "obj_list[1].id"_path;
    static_assert(path.Size() == 3);
    EXPECT_EQ((std::vector<std::string>{"obj_list", "[1]", "id"}), ToStrings(path));
}

TEST(JsonPath, ConsecutiveIndices) {
    constexpr auto path = "a[0][1]"_path;
    static_assert(path.Size() == 3);
    EXPECT_EQ((std::vector<std::string>{"a", "[0]", "[1]"}), ToStrings(path));
}

TEST(JsonPath, EmptyPath) {
    constexpr auto path = ""_path;
    static_assert(path.Size() == 0);
    EXPECT_TRUE(ToStrings(path).empty());
}

// 说明：以下写法均为编译期错误（由 static_assert 拦截），无法放进普通用例里，
// 所以在本文件之外用编译失败检查覆盖：
//   "a..b"_path、".a"_path、"a."_path   —— 空段
//   "a[2"_path                          —— '[' 未闭合
//   "a[]"_path                          —— 空下标
//   "a[abc]"_path                       —— 下标非数字
TEST(JsonPath, ClosingBracketIsPartOfKey) {
    // ']' 不在 '[' 之后时按普通 key 字符处理（与运行时解析一致）
    constexpr auto path = "a]b"_path;
    static_assert(path.Size() == 1);
    static_assert(path[0] == "a]b");
    EXPECT_EQ((std::vector<std::string>{"a]b"}), ToStrings(path));
}

TEST(JsonPath, IndexThenKey) {
    constexpr auto path = "a[0].b"_path;
    static_assert(path.Size() == 3);
    static_assert(path[0] == "a");
    static_assert(path[1] == "[0]");
    static_assert(path[2] == "b");
    EXPECT_EQ((std::vector<std::string>{"a", "[0]", "b"}), ToStrings(path));
}

TEST(JsonPath, IterationMatchesIndexing) {
    constexpr auto path = "a.b[2].c.d"_path;
    std::vector<std::string> tokens;
    for (const std::string_view token : path) {
        tokens.emplace_back(token);
    }
    EXPECT_EQ(ToStrings(path), tokens);
}

TEST(JsonPath, TokensOutliveTheirScope) {
    std::vector<std::string> tokens;
    {
        constexpr auto path = "a.b[2]"_path;
        tokens = ToStrings(path);
    }
    EXPECT_EQ((std::vector<std::string>{"a", "b", "[2]"}), tokens);
}
