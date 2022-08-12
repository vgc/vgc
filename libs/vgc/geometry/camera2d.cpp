// Copyright 2021 The VGC Developers
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

#include <vgc/geometry/camera2d.h>

#include <cmath>

namespace vgc::geometry {

Camera2d::Camera2d()
    : center_(0, 0)
    , zoom_(1)
    , rotation_(0)
    , viewportWidth_(1)
    , viewportHeight_(1)
    , nearPlane_(-1)
    , farPlane_(1) {
}

Mat4d Camera2d::viewMatrix() const {

    const double cx = center().x();
    const double cy = center().y();
    const double w = viewportWidth();
    const double h = viewportHeight();

    Mat4d res;
    res.setToIdentity();
    res.translate(0.5 * w - cx, 0.5 * h - cy);
    res.rotate(rotation());
    res.scale(zoom(), zoom(), 1); // don't scale the Z coordinate
    return res;
}

Mat4d Camera2d::projectionMatrix() const {

    const double w = viewportWidth_;
    const double h = viewportHeight_;
    const double n = nearPlane_;
    const double f = farPlane_;

    // clang-format off
    return Mat4d(2/w , 0    , 0       , -1          ,
                 0   , -2/h , 0       , 1           ,
                 0   , 0    , 2/(n-f) , (n+f)/(n-f) ,
                 0   , 0    , 0       , 1           );
    // clang-format on

    // Notes:
    // 1. In the second row of the matrix, we perform
    //    the inversion of Y axis (SVG -> OpenGL conventions).
    // 2. I'm not 100% sure of the signs on the 3rd row. We should
    //    try with an object at (0, 0, 0.5) obsuring one at (0, 0, -0.5).
}

} // namespace vgc::geometry
