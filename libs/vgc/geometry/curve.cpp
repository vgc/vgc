// Copyright 2017 The VGC Developers
// See the COPYRIGHT file at the top-level directory of this distribution
// and at https://github.com/vgc.io/vgc/blob/master/COPYRIGHT
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
#include <vgc/geometry/bezier.h>
#include <vgc/geometry/vec2d.h>

namespace vgc {
namespace geometry {

namespace {

int numSamples_(const std::vector<double>& data) {
    return data.size() / 2;
}

Vec2d position_(const std::vector<double>& data, int i) {
    return Vec2d(data[2*i], data[2*i+1]);
}

} // namespace

Curve::Curve(Type type) :
    type_(type),
    positionData_(),
    widthVariability_(AttributeVariability::PerSample),
    widthData_()
{

}

Curve::Curve(double constantWidth, Type type) :
    type_(type),
    positionData_(),
    widthVariability_(AttributeVariability::Constant),
    widthData_(1, constantWidth) // = vector containing a single element
{

}

double Curve::width() const
{
    return core::average(widthData_);
}

void Curve::addSample(double x, double y)
{
    // Set position
    positionData_.push_back(x);
    positionData_.push_back(y);

    // Set width
    if (widthVariability() == AttributeVariability::PerSample) {
        double width = 1.0;
        if (widthData_.size() > 0) {
            width = widthData_.back();
        }
        widthData_.push_back(width);
    }
}

void Curve::addSample(const Vec2d& position)
{
    addSample(position.x(), position.y());
}

void Curve::addSample(double x, double y, double width)
{
    // Set position
    positionData_.push_back(x);
    positionData_.push_back(y);

    // Set width
    if (widthVariability() == AttributeVariability::PerSample) {
        widthData_.push_back(width);
    }
}

void Curve::addSample(const Vec2d& position, double width)
{
    addSample(position.x(), position.y(), width);
}

std::vector<Vec2d> Curve::triangulate() const
{
    // XXX Stupid implementation for now.
    // TODO adaptive sampling, etc.

    // XXX For now, we simply evaluate a fixed number
    // of times per sample. Later, we'll do adaptive sampling.
    const int numEvalsPerSample = 10; // must be >= 1

    std::vector<Vec2d> res; // XXX Should we allow to pass this as output param to reuse capacity?

    // Early return if not enough segments
    const int numSamples = numSamples_(positionData_);
    const int numSegments = numSamples - 1;
    if (numSegments < 1) {
        return res;
    }

    // Iterates over all segments
    for (int i = 0; i < numSegments; ++i)
    {
        // Get indices of Catmull-Rom control points for current segment
        int i0 = core::clamp(i-1, 0, numSamples-1);
        int i1 = core::clamp(i  , 0, numSamples-1);
        int i2 = core::clamp(i+1, 0, numSamples-1);
        int i3 = core::clamp(i+2, 0, numSamples-1);

        // Get Catmull-Rom positions
        Vec2d p0 = position_(positionData_, i0);
        Vec2d p1 = position_(positionData_, i1);
        Vec2d p2 = position_(positionData_, i2);
        Vec2d p3 = position_(positionData_, i3);

        // Convert Catmull-Rom positions to Bézier positions.
        //
        // We choose a tension parameter k = 1/6 which ensures that if the
        // Catmull-Rom control points are aligned and uniformly spaced, then
        // the resulting curve is uniformly parameterized. This in fact
        // corresponds to a "uniform" Catmull-Rom. Indeed, a Catmull-Rom curve
        // is generally defined as a sequence of (t[i], p[i]) pairs, and the
        // derivative at p[i] is defined by:
        //
        //            p[i+1] - p[i-1]
        //     m[i] = ---------------
        //            t[i+1] - t[i-1]
        //
        // A *uniform* Catmull-Rom assumes that the "times" or "knot values"
        // t[i] are uniformly spaced, e.g.: [0, 1, 2, 3, 4, ... ]. In this
        // case, we have t[i+1] - t[i-1] = 2, so m[i] = (p[i+1] - p[i-1]) / 2.
        //
        // Now, recalling that for a cubic bezier B(t) defined for t in [0, 1]
        // by the control points q0, q1, q2, q3, we have:
        //
        //     dB/dt = 3(1-t)^2(q1-q0) + 6(1-t)t(q2-q1) + 3t^2(q3-q2),
        //
        // we can deduce that
        //
        //     q1 = q0 + (1/3) * dB/dt
        //
        // Therefore, if B(t) corresponds to the uniform Catmull-Rom subcurve
        // between p[i] and p[i+1], we have:
        //
        //    q1 = q0 + (1/3) * m[i]
        //       = q0 + (1/6) * (p[i+1] - p[i-1])
        //       = q0 + (1/6) * (p2 - p0)
        //
        const double k = 0.166666666666666667; // = 1/6 up to double precision
        Vec2d q0 = p1;
        Vec2d q1 = p1 + k * (p2 - p0);
        Vec2d q2 = p2 - k * (p3 - p1);
        Vec2d q3 = p2;

        // Get cubic Bézier control points for current segment width
        double w0 = 0;
        double w1 = 0;
        double w2 = 0;
        double w3 = 0;
        if (widthVariability() == AttributeVariability::PerSample)
        {
            // Catmull-Rom control points
            double v0 = widthData()[i0];
            double v1 = widthData()[i1];
            double v2 = widthData()[i2];
            double v3 = widthData()[i3];

            // Bezier control points
            w0 = v1;
            w1 = v1 + k * (v2 - v0);
            w2 = v2 - k * (v3 - v1);
            w3 = v2;
        }

        // Iterates over all evals. Note: first segment has one more eval than
        // the others. Total: numEvals = 1 + (numSamples - 1) * numEvalsPerSample.
        const int j1 = (i == 0) ? 0 : 1;
        for (int j = j1; j <= numEvalsPerSample; ++j)
        {
            const double u = (double) j / (double) numEvalsPerSample;

            Vec2d position = cubicBezier(q0, q1, q2, q3, u);
            Vec2d tangent = cubicBezierDer(q0, q1, q2, q3, u);
            Vec2d normal = tangent.normalized().orthogonalized();

            // Get width for this eval
            double w;
            switch(widthVariability()) {
            case AttributeVariability::Constant:
                w = widthData()[0];
                break;
            case AttributeVariability::PerSample:
                w = cubicBezier(w0, w1, w2, w3, u);
                break;
            }

            // Get left and right points of triangle strip
            double halfwidth = 0.5 * w;
            Vec2d leftPos  = position + halfwidth * normal;
            Vec2d rightPos = position - halfwidth * normal;

            // Add vertices to list of vertices
            res.push_back(leftPos);
            res.push_back(rightPos);
        }
    }

    return res;
}

} // namespace geometry
} // namespace vgc
