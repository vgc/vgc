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

#include <vgc/geometry/segment2.h>

#include <vgc/geometry/detail/segmentintersect.h>

namespace vgc::geometry {

template<typename T>
SegmentIntersection2<T> segmentIntersect(
    const Vec2<T>& a1,
    const Vec2<T>& b1,
    const Vec2<T>& a2,
    const Vec2<T>& b2) {

    return detail::segmentintersect::intersect(a1, b1, a2, b2);
}

template SegmentIntersection2f
segmentIntersect(const Vec2f&, const Vec2f&, const Vec2f&, const Vec2f&);

template SegmentIntersection2d
segmentIntersect(const Vec2d&, const Vec2d&, const Vec2d&, const Vec2d&);

} // namespace vgc::geometry
