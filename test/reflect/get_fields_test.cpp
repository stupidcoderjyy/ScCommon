//
// Created by jiangyuyun3 on 2026/10/14.
//

#include <cstddef>
#include <string>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <vector>

#include <gtest/gtest.h>

#include "reflect/pfr.hpp"

//NOLINTBEGIN

// ---------------- 计数用聚合 ----------------

struct Empty {};

struct OneField {
  int f1;
};

struct TwoFields {
  int f1;
  int f2;
};

struct ThreeFields {
  int f1;
  double f2;
  char f3;
};

// 带默认成员初始化，仍应数出全部字段
struct DefaultedFields {
  int f1 = 1;
  int f2 = 2;
};

// 字段类型不影响计数
struct MixedFields {
  int f1 = 0;
  std::string f2;
  std::vector<int> f3;
};

// 覆盖二分查找的边界：2 的幂及其后一个
struct Fields05 {
  int f1, f2, f3, f4, f5;
};

struct Fields08 {
  int f1, f2, f3, f4, f5, f6, f7, f8;
};

struct Fields16 {
  int f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15, f16;
};

struct Fields17 {
  int f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15, f16,
      f17;
};

struct Fields32 {
  int f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15, f16,
      f17, f18, f19, f20, f21, f22, f23, f24, f25, f26, f27, f28, f29, f30,
      f31, f32;
};

struct Fields33 {
  int f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15, f16,
      f17, f18, f19, f20, f21, f22, f23, f24, f25, f26, f27, f28, f29, f30,
      f31, f32, f33;
};

// ---------------- 取字段用聚合 ----------------

struct GetThreeFields {
  int f1;
  double f2;
  std::string f3;
};

struct GetMixedFields {
  int f1;
  std::vector<std::string> f2;
};

struct Fields10 {
  int f1, f2, f3, f4, f5, f6, f7, f8, f9, f10;
};

struct Fields63 {
  int f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15, f16,
      f17, f18, f19, f20, f21, f22, f23, f24, f25, f26, f27, f28, f29, f30,
      f31, f32, f33, f34, f35, f36, f37, f38, f39, f40, f41, f42, f43, f44,
      f45, f46, f47, f48, f49, f50, f51, f52, f53, f54, f55, f56, f57, f58,
      f59, f60, f61, f62, f63;
};

// ---------------- 注册用聚合 ----------------

struct Point {
  int x;
  int y;
};

struct Mixed {
  int i;
  std::string s;
  double d;
};

struct Unregistered {
  int x;
};

//NOLINTEND

REGISTER_FIELDS(Point, x, y)
REGISTER_FIELDS(Mixed, i, s, d)

namespace {

// ---------------- 字段计数 ----------------

static_assert(sc::reflect::FieldCount<Empty>() == 0);
static_assert(sc::reflect::FieldCount<OneField>() == 1);
static_assert(sc::reflect::FieldCount<TwoFields>() == 2);
static_assert(sc::reflect::FieldCount<ThreeFields>() == 3);
static_assert(sc::reflect::FieldCount<DefaultedFields>() == 2);
static_assert(sc::reflect::FieldCount<MixedFields>() == 3);
static_assert(sc::reflect::FieldCount<Fields05>() == 5);
static_assert(sc::reflect::FieldCount<Fields08>() == 8);
static_assert(sc::reflect::FieldCount<Fields16>() == 16);
static_assert(sc::reflect::FieldCount<Fields17>() == 17);
static_assert(sc::reflect::FieldCount<Fields32>() == 32);
static_assert(sc::reflect::FieldCount<Fields33>() == 33);

TEST(ReflectPfrFieldCount, Counts) {
  EXPECT_EQ(0u, sc::reflect::FieldCount<Empty>());
  EXPECT_EQ(1u, sc::reflect::FieldCount<OneField>());
  EXPECT_EQ(2u, sc::reflect::FieldCount<TwoFields>());
  EXPECT_EQ(3u, sc::reflect::FieldCount<ThreeFields>());
  EXPECT_EQ(2u, sc::reflect::FieldCount<DefaultedFields>());
  EXPECT_EQ(3u, sc::reflect::FieldCount<MixedFields>());
  EXPECT_EQ(5u, sc::reflect::FieldCount<Fields05>());
  EXPECT_EQ(8u, sc::reflect::FieldCount<Fields08>());
  EXPECT_EQ(16u, sc::reflect::FieldCount<Fields16>());
  EXPECT_EQ(17u, sc::reflect::FieldCount<Fields17>());
  EXPECT_EQ(32u, sc::reflect::FieldCount<Fields32>());
  EXPECT_EQ(33u, sc::reflect::FieldCount<Fields33>());
  constexpr std::size_t count = sc::reflect::FieldCount<ThreeFields>();
  EXPECT_EQ(3u, count);
  struct Local {
    int f1, f2, f3, f4, f5, f6, f7;
  };
  EXPECT_EQ(7u, sc::reflect::FieldCount<Local>());
}

// ---------------- 取字段引用 ----------------

TEST(ReflectPfrGetField, ReturnsReferenceForEachIndex) {
  GetThreeFields v{1, 2.5, "x"};
  EXPECT_EQ(1, sc::reflect::GetField<0>(v));
  EXPECT_DOUBLE_EQ(2.5, sc::reflect::GetField<1>(v));
  EXPECT_EQ("x", sc::reflect::GetField<2>(v));

  sc::reflect::GetField<0>(v) = 7;
  sc::reflect::GetField<1>(v) = 8.5;
  sc::reflect::GetField<2>(v) = "y";
  EXPECT_EQ(7, v.f1);
  EXPECT_DOUBLE_EQ(8.5, v.f2);
  EXPECT_EQ("y", v.f3);
}

TEST(ReflectPfrGetField, AddressMatchesMember) {
  GetThreeFields v{1, 2.0, "x"};
  EXPECT_EQ(&v.f1, &sc::reflect::GetField<0>(v));
  EXPECT_EQ(&v.f2, &sc::reflect::GetField<1>(v));
  EXPECT_EQ(&v.f3, &sc::reflect::GetField<2>(v));
}

TEST(ReflectPfrGetField, SingleFieldAggregate) {
  OneField v{5};
  EXPECT_EQ(5, sc::reflect::GetField<0>(v));
  sc::reflect::GetField<0>(v) = 6;
  EXPECT_EQ(6, v.f1);
}

TEST(ReflectPfrGetField, NonTrivialMemberType) {
  GetMixedFields v{1, {"a", "b"}};
  auto& list = sc::reflect::GetField<1>(v);
  list.push_back("c");
  EXPECT_EQ(3u, v.f2.size());
  EXPECT_EQ("c", v.f2.back());
}

TEST(ReflectPfrGetField, MutableObjectReturnsMutableReference) {
  GetThreeFields v{};
  static_assert(std::is_same_v<decltype(sc::reflect::GetField<0>(v)), int&>);
  static_assert(
      std::is_same_v<decltype(sc::reflect::GetField<2>(v)), std::string&>);
}

TEST(ReflectPfrGetField, ConstObjectReturnsConstReference) {
  const GetThreeFields v{1, 2.0, "x"};
  static_assert(
      std::is_same_v<decltype(sc::reflect::GetField<0>(v)), const int&>);
  static_assert(std::is_same_v<decltype(sc::reflect::GetField<2>(v)),
                               const std::string&>);
  EXPECT_EQ(1, sc::reflect::GetField<0>(v));
  EXPECT_EQ("x", sc::reflect::GetField<2>(v));
}

TEST(ReflectPfrGetField, MidSizeAggregate) {
  Fields10 v{};
  v.f1 = 1;
  v.f10 = 10;
  EXPECT_EQ(1, sc::reflect::GetField<0>(v));
  EXPECT_EQ(10, sc::reflect::GetField<9>(v));

  sc::reflect::GetField<4>(v) = 5;
  EXPECT_EQ(5, v.f5);
}

TEST(ReflectPfrGetField, LargeAggregate) {
  Fields33 v{};
  v.f33 = 33;
  EXPECT_EQ(33, sc::reflect::GetField<32>(v));
  sc::reflect::GetField<16>(v) = 17;
  EXPECT_EQ(17, v.f17);

  Fields63 w{};
  w.f63 = 63;
  EXPECT_EQ(63, sc::reflect::GetField<62>(w));
  sc::reflect::GetField<0>(w) = 1;
  EXPECT_EQ(1, w.f1);
}

// ---------------- 注册元信息 ----------------

static_assert(sc::reflect::is_registered_v<Point>);
static_assert(sc::reflect::is_registered_v<Mixed>);
static_assert(!sc::reflect::is_registered_v<Unregistered>);
static_assert(sc::reflect::RegisteredFields<Point>() == 2);
static_assert(sc::reflect::RegisteredFields<Mixed>() == 3);
static_assert(sc::reflect::GetFieldName<0, Point>() == "x");
static_assert(sc::reflect::GetFieldName<1, Mixed>() == "s");

TEST(ReflectDescriptor, RegistrationDetection) {
  EXPECT_TRUE(sc::reflect::is_registered_v<Point>);
  EXPECT_FALSE(sc::reflect::is_registered_v<Unregistered>);
}

TEST(ReflectDescriptor, FieldInfoCarriesNameAndMemberPointer) {
  constexpr auto info = sc::reflect::GetFieldInfo<1, Mixed>();
  static_assert(std::is_same_v<decltype(info.member), std::string Mixed::*>);
  EXPECT_EQ("s", info.name);

  constexpr auto first = sc::reflect::GetFieldInfo<0, Point>();
  static_assert(std::is_same_v<decltype(first.member), int Point::*>);
  EXPECT_EQ("x", first.name);
}

TEST(ReflectDescriptor, GetFieldNameMatchesMembers) {
  EXPECT_EQ("x", (sc::reflect::GetFieldName<0, Point>()));
  EXPECT_EQ("y", (sc::reflect::GetFieldName<1, Point>()));
  EXPECT_EQ("i", (sc::reflect::GetFieldName<0, Mixed>()));
  EXPECT_EQ("s", (sc::reflect::GetFieldName<1, Mixed>()));
  EXPECT_EQ("d", (sc::reflect::GetFieldName<2, Mixed>()));
}

TEST(ReflectDescriptor, GetFieldReturnsMutableReference) {
  Point p{1, 2};
  static_assert(std::is_same_v<decltype(sc::reflect::GetField<0>(p)), int&>);
  EXPECT_EQ(1, sc::reflect::GetField<0>(p));
  EXPECT_EQ(&p.x, &sc::reflect::GetField<0>(p));

  sc::reflect::GetField<0>(p) = 7;
  EXPECT_EQ(7, p.x);

  Mixed m{1, "s", 2.5};
  sc::reflect::GetField<1>(m) = "t";
  sc::reflect::GetField<2>(m) = 9.5;
  EXPECT_EQ("t", m.s);
  EXPECT_DOUBLE_EQ(9.5, m.d);
}

TEST(ReflectDescriptor, GetFieldConstReturnsConstReference) {
  const Mixed m{1, "s", 2.5};
  static_assert(
      std::is_same_v<decltype(sc::reflect::GetField<1>(m)), const std::string&>);
  EXPECT_EQ("s", sc::reflect::GetField<1>(m));
  EXPECT_DOUBLE_EQ(2.5, sc::reflect::GetField<2>(m));
}

TEST(ReflectDescriptor, UnregisteredFallsBackToPfr) {
  Unregistered u{5};
  static_assert(!sc::reflect::is_registered_v<Unregistered>);
  EXPECT_EQ(5, sc::reflect::GetField<0>(u));
  sc::reflect::GetField<0>(u) = 6;
  EXPECT_EQ(6, u.x);
}

}  // namespace
