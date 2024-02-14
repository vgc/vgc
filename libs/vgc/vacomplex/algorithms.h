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
class Node;

/// Returns the boundary of the given `nodes`, that is, the subset of
/// cells in `closure(nodes)` that are:
///
/// - not contained in `nodes`, or
/// - whose opening in `closure(nodes)` is homeomorphic to Hn = Rn x [0, infinity), or
/// - that are in the boundary of any of the above cells
///
///
/// \sa `Cell::boundary()`, `Cell::star()`,
///     `outerBoundary()`, `star()`, `closure()`, `opening()`.
///
VGC_VACOMPLEX_API
core::Array<Node*> boundary(core::ConstSpan<Node*> nodes);

/// Returns the boundary of the given `cells`, that is, the subset of
/// cells in `closure(nodes)` that are:
///
/// - not contained in `nodes`, or
/// - whose opening in `closure(nodes)` is homeomorphic to Hn = Rn x [0, infinity), or
/// - that are in the boundary of any of the above cells
///
///
/// \sa `Cell::boundary()`, `Cell::star()`,
///     `outerBoundary()`, `star()`, `closure()`, `opening()`.
///
VGC_VACOMPLEX_API
core::Array<Cell*> boundary(core::ConstSpan<Cell*> cells);

/// Returns the outer boundary of the given `nodes`.
///
/// This is equivalent to `boundary(closure(nodes))` but faster to compute.
///
/// \sa `Cell::boundary()`, `Cell::star()`,
///     `boundary()`, `star()`, `closure()`, `opening()`.
///
VGC_VACOMPLEX_API
core::Array<Node*> outerBoundary(core::ConstSpan<Node*> nodes);

/// Returns the outer boundary of the given `cells`.
///
/// This is equivalent to `boundary(closure(cells))` but faster to compute.
///
/// \sa `Cell::boundary()`, `Cell::star()`,
///     `boundary()`, `star()`, `closure()`, `opening()`.
///
VGC_VACOMPLEX_API
core::Array<Cell*> outerBoundary(core::ConstSpan<Cell*> cells);

/// Returns the union of the star of the given `nodes`.
///
/// \sa `Cell::boundary()`, `Cell::star()`,
///     `boundary()`, `outerBoundary()`, `closure()`, `opening()`.
///
VGC_VACOMPLEX_API
core::Array<Node*> star(core::ConstSpan<Node*> nodes);

/// Returns the union of the star of the given `cells`.
///
/// \sa `Cell::boundary()`, `Cell::star()`,
///     `boundary()`, `outerBoundary()`, `closure()`, `opening()`.
///
VGC_VACOMPLEX_API
core::Array<Cell*> star(core::ConstSpan<Cell*> cells);

/// Returns the union of the given `nodes` and their boundary.
///
/// \sa `Cell::boundary()`, `Cell::star()`,
///     `boundary()`, `outerBoundary()`, `star()`, `opening()`.
///
VGC_VACOMPLEX_API
core::Array<Node*> closure(core::ConstSpan<Node*> nodes);

/// Returns the union of the given `cells` and their boundary.
///
/// \sa `Cell::boundary()`, `Cell::star()`,
///     `boundary()`, `outerBoundary()`, `star()`, `opening()`.
///
VGC_VACOMPLEX_API
core::Array<Cell*> closure(core::ConstSpan<Cell*> cells);

/// Returns the union of the given `nodes` and their star.
///
/// \sa `Cell::boundary()`, `Cell::star()`,
///     `boundary()`, `outerBoundary()`, `star()`, `closure()`.
///
VGC_VACOMPLEX_API
core::Array<Node*> opening(core::ConstSpan<Node*> nodes);

/// Returns the union of the given `cells` and their star.
///
/// \sa `Cell::boundary()`, `Cell::star()`,
///     `boundary()`, `outerBoundary()`, `star()`, `closure()`.
///
VGC_VACOMPLEX_API
core::Array<Cell*> opening(core::ConstSpan<Cell*> cells);

} // namespace vgc::vacomplex

#endif // VGC_VACOMPLEX_ALGORITHMS_H
