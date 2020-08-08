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

} // namespace geometry
} // namespace vgc
