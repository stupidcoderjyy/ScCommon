//
// Created by jiangyuyun3 on 2026/10/20.
//

#ifndef SCCOMMON_DEF_H
#define SCCOMMON_DEF_H

#include <cstddef>

#include "common/template_string.h"

namespace sc::reflect {

inline constexpr std::size_t kMaxFields = 64;

template <common::FixedString S>
consteval auto operator""_s() {
  return S;
}

}  // namespace sc::reflect

#endif  // SCCOMMON_DEF_H
