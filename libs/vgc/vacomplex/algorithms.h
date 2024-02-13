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

#ifndef VGC_VACOMPLEX_ALGORITHMS_H
#define VGC_VACOMPLEX_ALGORITHMS_H

#include <vgc/core/array.h>
#include <vgc/core/span.h>
#include <vgc/vacomplex/api.h>

namespace vgc::vacomplex {

class Cell;

/// Returns the union of the boundary of the given `cells`.
///
/// \sa `Cell::boundary()`, `Cell::star()`,
///     `star()`, `closure()`, `opening().
///
VGC_VACOMPLEX_API
core::Array<Cell*> boundary(core::ConstSpan<Cell*> cells);

/// Returns the union of the star of the given `cells`.
///
/// \sa `Cell::boundary()`, `Cell::star()`,
///     `boundary()`, `closure()`, `opening().
///
VGC_VACOMPLEX_API
core::Array<Cell*> star(core::ConstSpan<Cell*> cells);

/// Returns the union of the given `cells` and their boundary.
///
/// \sa `Cell::boundary()`, `Cell::star()`,
///     `boundary()`, `star()`, `opening().
///
VGC_VACOMPLEX_API
core::Array<Cell*> closure(core::ConstSpan<Cell*> cells);

/// Returns the union of the given `cells` and their star.
///
/// \sa `Cell::boundary()`, `Cell::star()`,
///     `boundary()`, `star()`, `closure()`.
///
VGC_VACOMPLEX_API
core::Array<Cell*> opening(core::ConstSpan<Cell*> cells);

} // namespace vgc::vacomplex

#endif // VGC_VACOMPLEX_ALGORITHMS_H
