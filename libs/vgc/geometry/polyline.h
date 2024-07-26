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

#ifndef VGC_GEOMETRY_POLYLINE_H
#define VGC_GEOMETRY_POLYLINE_H

#include <vgc/core/array.h>
#include <vgc/geometry/api.h>

namespace vgc::geometry {

/// \class vgc::geometry::Polyline
/// \brief Stores a sequence of points representing a polyline.
///
/// The `Polyline<TPoint>` class extends `core::Array<TPoint>` with helper
/// methods that are useful when the list of points represent a polyline.
///
/// Note that since its base class does not have a virtual destructor, it is
/// undefined behavior to destroy this class via a pointer to its base class.
///
template<typename TPoint>
class Polyline : public core::Array<TPoint> {
public:
    using ScalarType = decltype(TPoint()[0]);
    using BaseType = core::Array<TPoint>;

    // Forward all constructors
    using BaseType::BaseType;
};

} // namespace vgc::geometry

#endif // VGC_GEOMETRY_POLYLINE_H
