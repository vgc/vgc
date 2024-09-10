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

#ifndef VGC_GEOMETRY_SEGMENTINTERSECTOR2D_H
#define VGC_GEOMETRY_SEGMENTINTERSECTOR2D_H

#include <vgc/geometry/segment2d.h>
#include <vgc/geometry/segmentintersector.h>
#include <vgc/geometry/vec2d.h>

namespace vgc::geometry {

using SegmentIntersector2d = SegmentIntersector<double>;
extern template class SegmentIntersector<double>;

} // namespace vgc::geometry

#endif // VGC_GEOMETRY_SEGMENTINTERSECTOR2D_H
