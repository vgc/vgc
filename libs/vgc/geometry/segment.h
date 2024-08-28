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

// This file was automatically generated, please do not edit directly.
// Instead, edit tools/vec2x.h then run tools/generate.py.

#ifndef VGC_GEOMETRY_SEGMENT_H
#define VGC_GEOMETRY_SEGMENT_H

#include <vgc/core/arithmetic.h>
#include <vgc/core/enum.h>
#include <vgc/geometry/api.h>

namespace vgc::geometry {

/// \enum vgc::geometry::SegmentIntersectionType
/// \brief The nature of an intersection between two segments.
///
enum class SegmentIntersectionType : UInt8 {
    Empty,
    Point,
    Segment
};

VGC_GEOMETRY_API
VGC_DECLARE_ENUM(SegmentIntersectionType)

} // namespace vgc::geometry

#endif // VGC_GEOMETRY_SEGMENT_H
