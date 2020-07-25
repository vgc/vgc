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

#ifndef VGC_GEOMETRY_CURVECOMMAND_H
#define VGC_GEOMETRY_CURVECOMMAND_H

#include <vgc/core/arithmetic.h>
#include <vgc/core/array.h>
#include <vgc/geometry/api.h>

namespace vgc {

namespace geometry {

/// \enum vgc::geometry::CurveCommandType
/// \brief The type of a curve command (MoveTo, LineTo, etc.)
///
enum class CurveCommandType {
    ClosePath = 0, // Z (none)
    MoveTo,        // M (x y)+
    LineTo,        // L (x y)+
    HLineTo,       // H x+
    VLineTo,       // V y+
    CCurveTo,      // C (x1 y1 x2 y2 x y)+
    SCurveTo,      // S (x2 y2 x y)+
    QCurveTo,      // Q (x1 y1 x y)+
    TCurveTo,      // T (x y)+
    ArcTo          // A (rx ry x-axis-rotation large-arc-flag sweep-flag x y)+
};

/// \class vgc::geometry::CurveCommand
/// \brief Stores the type, relativeness, and number of arguments of a curve command.
///
class VGC_GEOMETRY_API CurveCommand {
public:
    /// Constructs a CurveCommand.
    ///
    CurveCommand(CurveCommandType type, bool isRelative, Int numArguments) :
        numArguments_(numArguments),
        type_(type),
        isRelative_(isRelative) {

    }

    /// Returns the type of the command.
    ///
    CurveCommandType type() const {
        return type_;
    }

    /// Returns whether this command use relative or absolute coordinates.
    ///
    bool isRelative() const {
        return isRelative_;
    }

    /// Returns the number of arguments this command applies to.
    ///
    Int numArguments() const {
        return numArguments_;
    }

private:
    Int numArguments_;
    CurveCommandType type_;
    bool isRelative_;
    // Note: members are ordered largest-first for better packing
};

using CurveCommandArray = core::Array<CurveCommand>;

} // namespace geometry
} // namespace vgc

#endif // VGC_GEOMETRY_CURVECOMMAND_H
