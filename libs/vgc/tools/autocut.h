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

#include <vgc/tools/api.h>

#include <vgc/vacomplex/cell.h>

namespace vgc::tools {

/// \class vgc::tools::AutoCutParams
/// \brief Parameters for the autoCut() algorithm.
///
class VGC_TOOLS_API AutoCutParams {
public:
    /// Returns the tolerance to use for intersection tests.
    ///
    double tolerance() const {
        return tolerance_;
    }

    /// Sets the value for `tolerance()`.
    ///
    void setTolerance(double value) {
        tolerance_ = value;
    }

    /// Whether to compute self-intersections for the given edge.
    ///
    bool cutItself() const {
        return cutItself_;
    }

    /// Sets the value for `interesectWithSelf()`.
    ///
    void setCutItself(bool value) {
        cutItself_ = value;
    }

    /// Whether to compute intersections between the given edge and other edges.
    ///
    bool cutEdges() const {
        return cutEdges_;
    }

    /// Sets the value for `cutEdges()`.
    ///
    void setCutEdges(bool value) {
        cutEdges_ = value;
    }

    /// Whether to compute intersections between the given edge and faces.
    ///
    bool cutFaces() const {
        return cutFaces_;
    }

    /// Sets the value for `cutEdges()`.
    ///
    void setCutFaces(bool value) {
        cutFaces_ = value;
    }

private:
    double tolerance_ = 1.e-6;
    bool cutItself_ = true;
    bool cutEdges_ = true;
    bool cutFaces_ = true;
};

/// Computes intersections between the given edge and other edges/faces, and
/// split them as appropriate.
///
VGC_TOOLS_API
void autoCut(vacomplex::KeyEdge* edge, const AutoCutParams& params);

} // namespace vgc::tools
