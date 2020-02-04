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

#ifndef VGC_CORE_DOUBLEARRAY_H
#define VGC_CORE_DOUBLEARRAY_H

#include <string>

#include <vgc/core/api.h>
#include <vgc/core/array.h>

namespace vgc {
namespace core {

using DoubleArray = Array<double>;

/// Converts the given string into a DoubleArray. Raises ParseError if the given
/// string does not represent a DoubleArray.
///
VGC_CORE_API
DoubleArray toDoubleArray(const std::string& s);

} // namespace core
} // namespace vgc

#endif // VGC_CORE_DOUBLEARRAY_H
