// Copyright 2024 The VGC Developers
// See the COPYRIGHT file at the top-level directory of this distribution
// and at https://github.com/vgc/vgc/blob/master/COPYRIGHT
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef VGC_CORE_RANGES_H
#define VGC_CORE_RANGES_H

/// \file vgc/core/ranges.h
/// \brief Utilities for using ranges.
///
/// This header contains functions and structures for creating, manipulating,
/// and iterating over ranges.

#include <range/v3/all.hpp>

namespace vgc {

/// Namespace where range concepts and algorithms are defined.
/// For now, this is an alias of `ranges::cpp20` from range-v3.
/// Once we require C++20, this should become an alias for `std::ranges`.
///
namespace ranges = ::ranges::cpp20;

/// Namespace where range views are defined.
/// For now, this is an alias of `ranges::cpp20::views` from range-v3.
/// Once we require C++20, this should become an alias for `std::ranges::view`.
///
namespace views = ::ranges::cpp20::views;

} // namespace vgc

#endif // VGC_CORE_RANGES_H
