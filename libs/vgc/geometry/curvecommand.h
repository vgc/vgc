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

#ifndef VGC_GEOMETRY_CURVECOMMAND_H
#define VGC_GEOMETRY_CURVECOMMAND_H

#include <vgc/core/arithmetic.h>
#include <vgc/core/array.h>
#include <vgc/geometry/api.h>

namespace vgc::geometry {

/// \enum vgc::geometry::CurveCommandType
/// \brief The type of a curve command (MoveTo, LineTo, etc.)
///
enum class CurveCommandType : UInt8 {
    Close,
    MoveTo,
    LineTo,
    QuadraticBezierTo,
    CubicBezierTo
};

/// Writes the given CurveCommandType to the output stream.
///
template<typename OStream>
void write(OStream& out, CurveCommandType c) {
    using core::write;
    switch (c) {
    case CurveCommandType::Close:
        write(out, "Close");
        break;
    case CurveCommandType::MoveTo:
        write(out, "MoveTo");
        break;
    case CurveCommandType::LineTo:
        write(out, "LineTo");
        break;
    case CurveCommandType::QuadraticBezierTo:
        write(out, "QuadraticBezierTo");
        break;
    case CurveCommandType::CubicBezierTo:
        write(out, "CubicBezierTo");
        break;
    }
}

namespace detail {

// Stores the type and how to access the parameters of each command. Note that
// the parameters themselves are stored in a separate DoubleArray (= `data`).
//
// The param indices of the first command are from 0 to
// commandsData[0].endParamIndex - 1, for the second command they are from
// commandsData[0].endParamIndex to commandsData[1].endParamIndex - 1, etc.
//
// In the future, we may want to do benchmarks to determine whether performance
// increases by storing type and endParamIndex as separate arrays.
//
struct VGC_GEOMETRY_API CurveCommandData {
    CurveCommandType type;
    Int endParamIndex;
};

using CurveCommandDataArray = core::Array<CurveCommandData>;

} // namespace detail

} // namespace vgc::geometry

#endif // VGC_GEOMETRY_CURVECOMMAND_H
