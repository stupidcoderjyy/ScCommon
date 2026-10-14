//
// Created by jiangyuyun3 on 2026/10/14.
//

#include <cstddef>
#include <string>
#include <vector>

#include <gtest/gtest.h>

#include "reflect/pfr.hpp"

namespace {

//NOLINTBEGIN

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

//NOLINTEND

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

TEST(ReflectPfrFieldCount, Test) {
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

}  // namespace
