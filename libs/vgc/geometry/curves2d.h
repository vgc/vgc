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

#ifndef VGC_GEOMETRY_CURVES2D_H
#define VGC_GEOMETRY_CURVES2D_H

#include <vgc/core/doublearray.h>
#include <vgc/geometry/api.h>
#include <vgc/geometry/curvecommand.h>

namespace vgc {

namespace geometry {

/// \class vgc::geometry::Curves2d
/// \brief Sequence of double-precision 2D curves.
///
class VGC_GEOMETRY_API Curves2d {
public:
    /// Construct an empty sequence of curves.
    ///
    Curves2d() {

    }

    /// Returns the sequence of commands defining the curves.
    ///
    const CurveCommandArray& commands() const {
        return commands_;
    }

    /// Returns the array of all command arguments.
    ///
    const core::DoubleArray& arguments() const {
        return arguments_;
    }

    /// Adds a new command to this sequence of curves.
    ///
    void addCommand(CurveCommandType type, bool isRelative,
                    const core::DoubleArray& arguments);

private:
    CurveCommandArray commands_;
    core::DoubleArray arguments_;
};

} // namespace geometry
} // namespace vgc

#endif // VGC_GEOMETRY_CURVES2D_H
