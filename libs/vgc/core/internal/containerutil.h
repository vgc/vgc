// Copyright 2020 The VGC Developers
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

#ifndef VGC_CORE_INTERNAL_CONTAINERUTIL_H
#define VGC_CORE_INTERNAL_CONTAINERUTIL_H

#include <iterator>
#include <type_traits>

namespace vgc {
namespace core {
namespace internal {

// Checks whether the given template argument is a signed integer.
template<typename I>
using RequireSignedInteger =
    typename std::enable_if<
        std::is_integral<I>::value &&
        std::is_signed<I>::value>::type;

// Checks whether the given template argument is an input iterator.
template<typename It>
using RequireInputIterator =
    typename std::enable_if<std::is_convertible<
        typename std::iterator_traits<It>::iterator_category,
        std::input_iterator_tag>::value>::type;

} // namespace internal
} // namespace core
} // namespace vgc

#endif // VGC_CORE_INTERNAL_CONTAINERUTIL_H
