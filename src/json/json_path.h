//
// Created by jiangyuyun3 on 2026/9/11.
//

#ifndef SCCOMMON_JSON_PATH_H
#define SCCOMMON_JSON_PATH_H
#include <algorithm>
#include <array>
#include <concepts>
#include <cstddef>
#include <string>
#include <string_view>
#include <vector>

namespace sc::json {

template <std::size_t N>
struct Parts {
    std::array<std::string_view, N> parts;
    constexpr std::size_t Size() const noexcept { return N; }  //NOLINT
    constexpr std::string_view operator[](std::size_t i) const noexcept { return parts[i]; }
    constexpr auto begin() const { return parts.begin(); }
    constexpr auto end() const { return parts.end(); }
};

consteval bool HasEmptySegment(std::string_view s) {
    if (s.empty()) return false;
    if (s.front() == '.' || s.back() == '.') return true;
    return s.find("..") != std::string_view::npos;
}

consteval bool HasInvalidBracket(std::string_view s) {
    std::size_t i = 0;
    while (i < s.size()) {
        const std::size_t open = s.find('[', i);
        if (open == std::string_view::npos) return false;
        const std::size_t close = s.find(']', open + 1);
        if (close == std::string_view::npos) return true;  // 未闭合
        const std::string_view digits = s.substr(open + 1, close - open - 1);
        if (digits.empty()) return true;  // "[]"
        for (const char c : digits) {
            if (c < '0' || c > '9') return true;  // "[abc]"
        }
        i = close + 1;
    }
    return false;
}

consteval bool NextToken(std::string_view s, std::size_t& i, std::size_t& start, std::size_t& end) {
    if (i >= s.size()) return false;

    if (s[i] == '[') {
        const std::size_t close = s.find(']', i);
        start = i;
        end = close == std::string_view::npos ? s.size() : close + 1;
        i = end;
    } else {
        start = i;
        while (i < s.size() && s[i] != '.' && s[i] != '[')
            ++i;
        end = i;
    }

    if (i < s.size() && s[i] == '.') ++i;  // 跳过一个分隔点
    return true;
}

consteval std::size_t CountParts(std::string_view s) {
    std::size_t n = 0, i = 0, a, b;
    while (NextToken(s, i, a, b))
        ++n;
    return n;
}

template <std::size_t N>
consteval Parts<N> MakeParts(std::string_view s) {
    Parts<N> r{};
    std::size_t n = 0, i = 0, a, b;
    while (NextToken(s, i, a, b))
        r.parts[n++] = s.substr(a, b - a);
    return r;
}

template <std::size_t N>
struct FixedString {
    std::array<char, N> data{};
    //必须支持隐式转换
    consteval FixedString(const char (&s)[N]) { std::copy_n(s, N, data.begin()); }  //NOLINT
    [[nodiscard]] consteval std::string_view view() const { return {data.data(), N - 1}; }
};

template <std::size_t N>
FixedString(const char (&)[N]) -> FixedString<N>;

template <FixedString S>
consteval auto operator""_path() {
    constexpr std::string_view view = S.view();
    static_assert(!HasEmptySegment(view),
            "JsonPath: empty segment is not allowed, e.g. \"a..b\", \".a\", \"a.\"");
    static_assert(!HasInvalidBracket(view),
            "JsonPath: '[' must be paired with ']' and contain digits only, e.g. \"a[2]\"");
    constexpr std::size_t N = CountParts(view);
    return MakeParts<N>(view);
}

// 路径概念：Parts<N>（编译期路径）与 RuntimePath（运行期路径）共同满足。
template <typename Path>
concept PathLike = requires(const Path& path, std::size_t index) {
    { path.Size() } -> std::convertible_to<std::size_t>;
    { path[index] } -> std::convertible_to<std::string_view>;
};

// 运行期路径：与 Parts<N> 产出相同的 token 序列（"a.b[2].c" → {"a","b","[2]","c"}）。
// 代价：不做编译期校验；非法路径（空段、'[' 未闭合等）在解析阶段由 OnPathResolveError 报告。
// token 由本对象持有：短 key 走 SSO 不产生堆分配。
class RuntimePath {
public:
    RuntimePath() = default;
    explicit RuntimePath(std::string_view path) { Parse(path); }

    [[nodiscard]] std::size_t Size() const noexcept { return tokens_.size(); }  //NOLINT
    [[nodiscard]] std::string_view operator[](std::size_t index) const noexcept {
        return tokens_[index];
    }
    [[nodiscard]] bool Empty() const noexcept { return tokens_.empty(); }

private:
    void Parse(std::string_view path) {
        std::size_t pos = 0;
        while (pos < path.size()) {
            const std::size_t dot = path.find('.', pos);
            const std::size_t end = dot == std::string_view::npos ? path.size() : dot;
            AppendSegment(path.substr(pos, end - pos));
            if (dot == std::string_view::npos) {
                break;
            }
            pos = dot + 1;
        }
    }

    // 一个路径段可能同时包含 key 与若干下标："b[2][3]" → "b", "[2]", "[3]"
    void AppendSegment(std::string_view segment) {
        std::size_t bracket = segment.find('[');
        if (const std::string_view key = segment.substr(0, bracket); !key.empty()) {
            tokens_.emplace_back(key);
        } else if (bracket == std::string_view::npos) {
            tokens_.emplace_back(segment);  // 空段：保留空 token，解析阶段报错
            return;
        }
        while (bracket != std::string_view::npos) {
            const std::size_t close = segment.find(']', bracket + 1);
            if (close == std::string_view::npos) {
                tokens_.emplace_back(segment.substr(bracket));  // 未闭合：解析阶段报错
                return;
            }
            tokens_.emplace_back(segment.substr(bracket, close - bracket + 1));
            bracket = segment.find('[', close + 1);
        }
    }

    std::vector<std::string> tokens_;
};

}  // namespace sc::json

#endif  //SCCOMMON_JSON_PATH_H
