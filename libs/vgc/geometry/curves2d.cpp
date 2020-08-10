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

#include <vgc/geometry/curves2d.h>

namespace vgc {
namespace geometry {

void Curves2d::close()
{
    commandData_.append({CurveCommandType::Close, data_.length()});
}

void Curves2d::moveTo(const core::Vec2d& p)
{
    moveTo(p[0], p[1]);
}

void Curves2d::moveTo(double x, double y)
{
    data_.append(x);
    data_.append(y);
    commandData_.append({CurveCommandType::MoveTo, data_.length()});
}

void Curves2d::lineTo(const core::Vec2d& p)
{
    lineTo(p[0], p[1]);
}

void Curves2d::lineTo(double x, double y)
{
    // TODO (for all functions): support subcommands, that is, don't add a new
    // command if the last command is the same. We should simply add more params.
    data_.append(x);
    data_.append(y);
    commandData_.append({CurveCommandType::LineTo, data_.length()});
}

void Curves2d::quadraticBezierTo(const core::Vec2d& p1,
                                 const core::Vec2d& p2)
{
    quadraticBezierTo(p1[0], p1[1], p2[0], p2[1]);
}

void Curves2d::quadraticBezierTo(double x1, double y1,
                                 double x2, double y2)
{
    data_.append(x1);
    data_.append(y1);
    data_.append(x2);
    data_.append(y2);
    commandData_.append({CurveCommandType::QuadraticBezierTo, data_.length()});
}

void Curves2d::cubicBezierTo(const core::Vec2d& p1,
                             const core::Vec2d& p2,
                             const core::Vec2d& p3)
{
    cubicBezierTo(p1[0], p1[1], p2[0], p2[1], p3[0], p3[1]);
}

void Curves2d::cubicBezierTo(double x1, double y1,
                             double x2, double y2,
                             double x3, double y3)
{
    data_.append(x1);
    data_.append(y1);
    data_.append(x2);
    data_.append(y2);
    data_.append(x3);
    data_.append(y3);
    commandData_.append({CurveCommandType::CubicBezierTo, data_.length()});
}

namespace {

void addSample(
        core::DoubleArray& data,
        core::Vec2d& v0,
        core::Vec2d& v1,
        const core::Vec2d& p,
        const core::Vec2d& inset)
{
    core::Vec2d v2 = p + inset;
    core::Vec2d v3 = p - inset;
    data.insert(data.end(), {
        v0[0], v0[1], v1[0], v1[1], v2[0], v2[1],
        v2[0], v2[1], v1[0], v1[1], v3[0], v3[1]});
    v0 = v2;
    v1 = v3;
}

} // namespace

void Curves2d::stroke(core::DoubleArray& data, double width) const
{
    // XXX TODO: actually implement this function. For now, this is just a
    // temporary implementation (no computation of normals, no adaptive
    // sampling, etc.) for testing the rest of the architecture.

    // For now, assume fixed normal (like with calligraphy pen with wide flat with fixed orientation)
    core::Vec2d normal(1.0, 1.0);
    normal.normalize();
    core::Vec2d inset = 0.5 * width * normal;

    //    v0              v2
    //    *-------------->*
    //    |
    //    *--centerline-->* p
    //    |
    //    *-------------->*
    //    v1              v3
    //
    core::Vec2d v0, v1, p;
    for (vgc::geometry::Curves2dCommandRef c : commands()) {
        switch (c.type()) {
        case vgc::geometry::CurveCommandType::Close:
            break;
        case vgc::geometry::CurveCommandType::MoveTo:
            p = c.p();
            v0 = p + inset;
            v1 = p - inset;
            break;
        case vgc::geometry::CurveCommandType::LineTo:
            p = c.p(); addSample(data, v0, v1, p, inset);
            break;
        case vgc::geometry::CurveCommandType::QuadraticBezierTo:
            p = c.p1(); addSample(data, v0, v1, p, inset);
            p = c.p2(); addSample(data, v0, v1, p, inset);
            break;
        case vgc::geometry::CurveCommandType::CubicBezierTo:
            p = c.p1(); addSample(data, v0, v1, p, inset);
            p = c.p2(); addSample(data, v0, v1, p, inset);
            p = c.p3(); addSample(data, v0, v1, p, inset);
            break;
        }
    }
}

} // namespace geometry
} // namespace vgc
