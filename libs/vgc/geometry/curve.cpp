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

#include <vgc/geometry/curve.h>

#include <vgc/core/algorithm.h>
#include <vgc/core/colors.h>
#include <vgc/core/vec2d.h>
#include <vgc/geometry/bezier.h>
#include <vgc/geometry/catmullrom.h>

namespace vgc {
namespace geometry {

namespace {

template <typename T>
void removeAllExceptLastElement(std::vector<T>& v) {
    v.front() = v.back();
    v.resize(1);
}

template <typename T>
void appendUninitializedElement(std::vector<T>& v) {
    v.emplace_back();
}

void computeSample(
        const core::Vec2d& q0, const core::Vec2d& q1, const core::Vec2d& q2, const core::Vec2d& q3,
        double w0, double w1, double w2, double w3,
        double u,
        core::Vec2d& leftPosition,
        core::Vec2d& rightPosition,
        core::Vec2d& normal)
{
    // Compute position and normal
    core::Vec2d position = cubicBezier(q0, q1, q2, q3, u);
    core::Vec2d tangent = cubicBezierDer(q0, q1, q2, q3, u);
    normal = tangent.normalized().orthogonalized();

    // Compute half-width
    double halfwidth = 0.5 * cubicBezier(w0, w1, w2, w3, u);

    // Compute left and right positions
    leftPosition  = position + halfwidth * normal;
    rightPosition = position - halfwidth * normal;
}

} // namespace

Curve::Curve(Type type) :
    type_(type),
    positionData_(),
    widthVariability_(AttributeVariability::PerControlPoint),
    widthData_(),
    color_(core::colors::black)
{

}

Curve::Curve(double constantWidth, Type type) :
    type_(type),
    positionData_(),
    widthVariability_(AttributeVariability::Constant),
    widthData_(1, constantWidth), // = vector containing a single element
    color_(core::colors::black)
{

}

double Curve::width() const
{
    return core::average(widthData_);
}

void Curve::addControlPoint(double x, double y)
{
    // Set position
    positionData_.push_back(x);
    positionData_.push_back(y);

    // Set width
    if (widthVariability() == AttributeVariability::PerControlPoint) {
        double width = 1.0;
        if (widthData_.size() > 0) {
            width = widthData_.back();
        }
        widthData_.push_back(width);
    }
}

void Curve::addControlPoint(const core::Vec2d& position)
{
    addControlPoint(position.x(), position.y());
}

void Curve::addControlPoint(double x, double y, double width)
{
    // Set position
    positionData_.push_back(x);
    positionData_.push_back(y);

    // Set width
    if (widthVariability() == AttributeVariability::PerControlPoint) {
        widthData_.push_back(width);
    }
}

void Curve::addControlPoint(const core::Vec2d& position, double width)
{
    addControlPoint(position.x(), position.y(), width);
}

std::vector<core::Vec2d> Curve::triangulate(
        double maxAngle,
        int minQuads,
        int maxQuads) const
{
    // Result of this computation.
    // Final size = 2 * nSamples
    //   where nSamples = nQuads + 1
    //
    std::vector<core::Vec2d> res;

    // For adaptive sampling, we need to remember a few things about all the
    // samples in the currently processed segment ("segment" means "part of the
    // curve between two control points").
    //
    // These vectors could be declared in an inner loop but we declare them
    // here for performance (reuse vector capacity). All these vectors have
    // the same size.
    //
    std::vector<core::Vec2d> leftPositions;
    std::vector<core::Vec2d> rightPositions;
    std::vector<core::Vec2d> normals;
    std::vector<double> uParams;

    // Remember which quads do not pass the angle test. The index is relative
    // to the vectors above (e.g., leftPositions).
    //
    std::vector<int> failedQuads;

    // Factor out computation of cos(maxAngle)
    double cosMaxAngle = std::cos(maxAngle);

    // Early return if not enough segments
    int numControlPoints = positionData_.size() / 2;
    int numSegments = numControlPoints - 1;
    if (numSegments < 1) {
        return res;
    }

    // Iterate over all segments
    for (int i = 0; i < numSegments; ++i)
    {
        // Get indices of Catmull-Rom control points for current segment
        int i0 = core::clamp(i-1, 0, numControlPoints-1);
        int i1 = core::clamp(i  , 0, numControlPoints-1);
        int i2 = core::clamp(i+1, 0, numControlPoints-1);
        int i3 = core::clamp(i+2, 0, numControlPoints-1);

        // Get positions of Catmull-Rom control points
        core::Vec2d p0(positionData_[2*i0], positionData_[2*i0+1]);
        core::Vec2d p1(positionData_[2*i1], positionData_[2*i1+1]);
        core::Vec2d p2(positionData_[2*i2], positionData_[2*i2+1]);
        core::Vec2d p3(positionData_[2*i3], positionData_[2*i3+1]);

        // Convert positions from Catmull-Rom to Bézier
        core::Vec2d q0, q1, q2, q3;
        uniformCatmullRomToBezier(p0, p1, p2, p3,
                                  q0, q1, q2, q3);

        // Convert widths from Constant or Catmull-Rom to Bézier. Note: we
        // could handle the 'Constant' case more efficiently, but we chose code
        // simplicity over performance here, over the assumption that it's unlikely
        // that width computation is a performance bottleneck.
        double w0, w1, w2, w3;
        if (widthVariability() == AttributeVariability::PerControlPoint)
        {
            double v0 = widthData()[i0];
            double v1 = widthData()[i1];
            double v2 = widthData()[i2];
            double v3 = widthData()[i3];
            uniformCatmullRomToBezier(v0, v1, v2, v3,
                                      w0, w1, w2, w3);
        }
        else // if (widthVariability() == AttributeVariability::Constant)
        {
            w0 = widthData()[0];
            w1 = w0;
            w2 = w0;
            w3 = w0;
        }

        // Compute first sample of segment
        if (i == 0) {
            // Compute first sample of first segment
            double u = 0;
            appendUninitializedElement(leftPositions);
            appendUninitializedElement(rightPositions);
            appendUninitializedElement(normals);
            computeSample(q0, q1, q2, q3, w0, w1, w2, w3, u,
                          leftPositions.back(),
                          rightPositions.back(),
                          normals.back());

            // Add this sample to res right now. For all the other samples, we
            // need to wait until adaptive sampling is complete.
            res.push_back(leftPositions.back());
            res.push_back(rightPositions.back());
        }
        else {
            // re-use last sample of previous segment
            removeAllExceptLastElement(leftPositions);
            removeAllExceptLastElement(rightPositions);
            removeAllExceptLastElement(normals);
        }
        uParams.clear();
        uParams.push_back(0);

        // Compute uniform samples for this segment
        int numQuads = 0;
        for (int j = 1; j <= minQuads; ++j)
        {
            double u = (double) j / (double) minQuads;
            appendUninitializedElement(leftPositions);
            appendUninitializedElement(rightPositions);
            appendUninitializedElement(normals);
            computeSample(q0, q1, q2, q3, w0, w1, w2, w3, u,
                          leftPositions.back(),
                          rightPositions.back(),
                          normals.back());

            uParams.push_back(u);
            ++numQuads;
        }

        // Compute adaptive samples for this segment
        while (numQuads < maxQuads)
        {
            // Find quads that don't pass the angle test.
            //
            // Quads are indexed from 0 to numQuads-1. A quad of index i is
            // defined by leftPositions[i], rightPositions[i],
            // leftPositions[i+1], and rightPositions[i+1].
            //
            failedQuads.clear();
            for (int i = 0; i < numQuads; ++i) {
                if (normals[i].dot(normals[i+1]) < cosMaxAngle) {
                    failedQuads.push_back(i);
                }
            }

            // All angles are < maxAngle => adaptive sampling is complete :)
            if (failedQuads.empty()) {
                break;
            }

            // We reached max number of quads :(
            numQuads += failedQuads.size();
            if (numQuads > maxQuads) {
                break;
            }

            // For each failed quad, we will recompute a sample at the
            // mid-u-parameter. We do this in-place in decreasing index
            // order so that we never overwrite samples.
            //
            // It's easier to understand the code by unrolling the loops
            // manually with the following example:
            //
            // uParams before = [ 0.0   0.2   0.4   0.6   0.8   1.0 ]
            // failedQuads    = [           1           3           ]
            // uParams after  = [ 0.0   0.2  *0.3*  0.4   0.6  *0.7*  0.8   1.0 ]
            //
            // The asterisks emphasize the two new samples.
            //
            int numSamplesBefore = uParams.size(); // 6
            int numSamplesAfter = uParams.size() + failedQuads.size(); // 8
            leftPositions.resize(numSamplesAfter);
            rightPositions.resize(numSamplesAfter);
            normals.resize(numSamplesAfter);
            uParams.resize(numSamplesAfter);
            int i = numSamplesBefore - 1; // 5
            for (int j = failedQuads.size() - 1; j >= 0; --j) { // j = 1, then j = 0
                int k = failedQuads[j];                         // k = 3, then k = 1

                // First, offset index of all samples after the failed quad
                int offset = j + 1;                   // offset = 2, then offset = 1
                while (i > k) {                       // i = [5, 4], then i = [3, 2]
                    leftPositions[i + offset] = leftPositions[i];
                    rightPositions[i + offset] = rightPositions[i];
                    normals[i + offset] = normals[i];
                    uParams[i + offset] = uParams[i]; // u[7] = 1.0, u[6] = 0.8, then
                                                      // u[4] = 0.6, u[3] = 0.4
                    --i;
                }

                // Then, for i == k, we compute the new sample.
                //
                // Note to maintainer: if you change this code, be very careful
                // to ensure that new values are always computed from old
                // values, not from already overwritten new values.
                //
                double u = 0.5 * (uParams[i] + uParams[i+1]); // u = 0.7, then u = 0.3
                computeSample(q0, q1, q2, q3, w0, w1, w2, w3, u,
                              leftPositions[i + offset],
                              rightPositions[i + offset],
                              normals[i + offset]);
                uParams[i + offset] = u;
            }
        }
        // Here are the different states of uParams for the given example:
        //
        // before:         [ 0.0   0.2   0.4   0.6   0.8   1.0 ]
        // resize:         [ 0.0   0.2   0.4   0.6   0.8   1.0   0.0   0.0 ]
        // offset j=1 i=5: [ 0.0   0.2   0.4   0.6   0.8   1.0   0.0   1.0 ]
        // offset j=1 i=4: [ 0.0   0.2   0.4   0.6   0.8   1.0   0.8   1.0 ]
        // new    j=1 i=3: [ 0.0   0.2   0.4   0.6   0.8   0.7   0.8   1.0 ]
        // offset j=0 i=3: [ 0.0   0.2   0.4   0.6   0.6   0.7   0.8   1.0 ]
        // offset j=0 i=2: [ 0.0   0.2   0.4   0.4   0.6   0.7   0.8   1.0 ]
        // new    j=0 i=1: [ 0.0   0.2   0.3   0.4   0.6   0.7   0.8   1.0 ]

        // Transfer local leftPositions and rightPositions into res
        int numSamples = leftPositions.size();
        for (int i = 1; i < numSamples; ++i) {
            res.push_back(leftPositions[i]);
            res.push_back(rightPositions[i]);
        }
    }

    return res;
}

} // namespace geometry
} // namespace vgc

/*
############################# Implementation notes #############################

[1]

In the future, we may want to extend the Curve class with:
    - more curve type (e.g., bezier, bspline, nurbs, ellipticalarc. etc.)
    - variable color
    - variable custom attributes (e.g., that can be passed to shaders)
    - dimension other than 2? Probably not: That may be a separate type of
      curve

Supporting other types of curves in the future is why we use a
std::vector<double> of size 2*n instead of a std::vector<core::Vec2d> of size n.
Indeed, other types of curve may need additional data, such as knot values,
homogeneous coordinates, etc.

A "cleaner" approach with more type-safety would be to have different
classes for different types of curves. Unfortunately, this has other
drawbacks, in particular, switching from one curve type to the other
dynamically would be harder. Also, it is quite useful to have a continuous
array of doubles that can directly be passed to C-style functions, such as
OpenGL, etc.

[2] Should the "Curve" class be called Curve2d?

*/
