//
// Created by jiangyuyun3 on 2026/10/14.
//

#include <string>
#include <type_traits>
#include <vector>

#include <gtest/gtest.h>

#include "reflect/pfr.hpp"

namespace {

//NOLINTBEGIN

struct OneField {
  int f1;
};

struct ThreeFields {
  int f1;
  double f2;
  std::string f3;
};

struct MixedFields {
  int f1;
  std::vector<std::string> f2;
};

struct Fields10 {
  int f1; int f2; int f3; int f4; int f5;
  int f6; int f7; int f8; int f9; int f10;
};

struct Fields33 {
  int f1; int f2; int f3; int f4; int f5; int f6; int f7; int f8;
  int f9; int f10; int f11; int f12; int f13; int f14; int f15; int f16;
  int f17; int f18; int f19; int f20; int f21; int f22; int f23; int f24;
  int f25; int f26; int f27; int f28; int f29; int f30; int f31; int f32;
  int f33;
};

struct Fields63 {
  int f1; int f2; int f3; int f4; int f5; int f6; int f7; int f8;
  int f9; int f10; int f11; int f12; int f13; int f14; int f15; int f16;
  int f17; int f18; int f19; int f20; int f21; int f22; int f23; int f24;
  int f25; int f26; int f27; int f28; int f29; int f30; int f31; int f32;
  int f33; int f34; int f35; int f36; int f37; int f38; int f39; int f40;
  int f41; int f42; int f43; int f44; int f45; int f46; int f47; int f48;
  int f49; int f50; int f51; int f52; int f53; int f54; int f55; int f56;
  int f57; int f58; int f59; int f60; int f61; int f62; int f63;
};

//NOLINTEND

TEST(ReflectPfrGetField, ReturnsReferenceForEachIndex) {
  ThreeFields v{1, 2.5, "x"};
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
  ThreeFields v{1, 2.0, "x"};
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
  MixedFields v{1, {"a", "b"}};
  auto& list = sc::reflect::GetField<1>(v);
  list.push_back("c");
  EXPECT_EQ(3u, v.f2.size());
  EXPECT_EQ("c", v.f2.back());
}

TEST(ReflectPfrGetField, MutableObjectReturnsMutableReference) {
  ThreeFields v{};
  static_assert(std::is_same_v<decltype(sc::reflect::GetField<0>(v)), int&>);
  static_assert(
      std::is_same_v<decltype(sc::reflect::GetField<2>(v)), std::string&>);
}

TEST(ReflectPfrGetField, ConstObjectReturnsConstReference) {
  const ThreeFields v{1, 2.0, "x"};
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

}  // namespace
