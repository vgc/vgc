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

#include <vgc/core/intarray.h>

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

struct Sample {
    double u;
    core::Vec2d position;
};

struct SampleBuffer {
    core::Array<Sample> samples;
    core::IntArray failed;
    core::IntArray added;
};

struct QuadraticSegment {
    core::Vec2d p0, p1, p2;
    core::Vec2d startPosition() const { return p0; }
    core::Vec2d endPosition() const { return p2; }
    core::Vec2d startTangent() const { return p1 - p0; }
    core::Vec2d endTangent() const { return p2 - p1; }
    core::Vec2d operator()(double u) const {
        return (1-u)*(1-u)*p0 +
               2*u*(1-u)*p1 +
               u*u*p2;
    }
};

struct CubicSegment {
    core::Vec2d p0, p1, p2, p3;
    core::Vec2d startPosition() const { return p0; }
    core::Vec2d endPosition() const { return p3; }
    core::Vec2d startTangent() const { return p1 - p0; }
    core::Vec2d endTangent() const { return p3 - p2; }
    core::Vec2d operator()(double u) const {
        return (1-u)*(1-u)*(1-u)*p0 +
               3*u*(1-u)*(1-u)*p1 +
               3*u*u*(1-u)*p2 +
               u*u*u*p3;
    }
};

// TODO: expose this as part of the Vec2 interface, as well as "det".
double angle(const core::Vec2d& a, const core::Vec2d& b)
{
    double dot = a[0]*b[0] + a[1]*b[1]; // XXX can't use a.dot(b) cause non-const. See https://github.com/vgc/vgc/issues/434.
    double det = a[0]*b[1] - a[1]*b[0];
    return std::atan2(det, dot);
}

template<class SegmentType>
void sampleSegment(
        Curves2d& res,
        SampleBuffer& buffer,
        double maxAngle,
        Int maxSamplesPerSegment,
        SegmentType segment)
{
    core::Array<Sample>& samples = buffer.samples;
    core::IntArray& failed = buffer.failed;
    core::IntArray& added = buffer.added;

    // Initialization. The first and last samples are sentinel values to
    // be able to conveniently compute angles for first and last samples.
    samples.clear();
    samples.append(Sample{-1, segment.startPosition() - segment.startTangent()});
    samples.append(Sample{0, segment.startPosition()});
    samples.append(Sample{1, segment.endPosition()});
    samples.append(Sample{2, segment.endPosition() + segment.endTangent()});

    // Adaptive sampling
    while (samples.length() - 3 < maxSamplesPerSegment) {

        // Find which angles are too large
        failed.clear();
        for (Int i = 1; i < samples.length() - 1; ++i) {
            core::Vec2d a = samples[i].position - samples[i-1].position;
            core::Vec2d b = samples[i+1].position - samples[i].position;
            if (std::abs(angle(a, b)) > maxAngle) {
                failed.append(i);
            }
        }
        if (failed.isEmpty()) {
            break; // => success!
        }

        // Determine where to insert new samples. After this code block, each index
        // in `added` means "we should insert a new sample just before this index".
        added.clear();
        for (Int i : failed) {
            Int lastAdded = (added.isEmpty() ? -1 : added.last());
            if (i != 1 && i != lastAdded) {
                added.append(i);
                if (samples.length() + added.length() - 3 == maxSamplesPerSegment) {
                    break; // => stop adding samples (max reached)
                }
            }
            if (i != samples.length() - 2) {
                added.append(i+1);
                if (samples.length() + added.length() - 3 == maxSamplesPerSegment) {
                    break; // => stop adding samples (max reached)
                }
            }
        }

        // Push existing samples to the right to make space for new samples.
        // Note: the `added` array is also mutated to contain the new indices.
        Int numSamplesToAdd = added.length();
        Int numSamplesBefore = samples.length();
        Int numSamplesAfter = samples.length() + numSamplesToAdd;
        samples.resize(numSamplesAfter);
        for (Int i = numSamplesBefore - 1; numSamplesToAdd > 0; --i) {
            samples[i + numSamplesToAdd] = samples[i];
            if (i == added[numSamplesToAdd - 1]) {
                --numSamplesToAdd;
                added[numSamplesToAdd] = i + numSamplesToAdd;
            }
        }

        // Compute new samples. Note that the above guarantees that new samples
        // are never consecutive, so we can do u[i] = (u[i-1] + u[i+1]) / 2.
        for (Int i : added) {
            double u = 0.5 * (samples[i-1].u + samples[i+1].u);
            samples[i].u = u;
            samples[i].position = segment(u);
        }
    }

    // Append result
    for (Int i = 2; i < samples.length() - 1; ++i) {
        res.lineTo(samples[i].position);
    }
}

} // namespace

Curves2d Curves2d::sample(
        double maxAngle,
        Int maxSamplesPerSegment) const
{
    Curves2d res;
    core::Vec2d p0, p1, p2, p3;
    SampleBuffer buffer;

    for (vgc::geometry::Curves2dCommandRef c : commands()) {
        switch (c.type()) {
        case vgc::geometry::CurveCommandType::Close:
            res.close();
            break;
        case vgc::geometry::CurveCommandType::MoveTo:
            p0 = c.p();
            res.moveTo(p0);
            break;
        case vgc::geometry::CurveCommandType::LineTo:
            p0 = c.p();
            res.lineTo(p0);
            break;
        case vgc::geometry::CurveCommandType::QuadraticBezierTo:
            p1 = c.p1();
            p2 = c.p2();
            sampleSegment(res, buffer, maxAngle, maxSamplesPerSegment,
                          QuadraticSegment{p0, p1, p2});
            p0 = p2;
            break;
        case vgc::geometry::CurveCommandType::CubicBezierTo:
            p1 = c.p1();
            p2 = c.p2();
            p3 = c.p3();
            sampleSegment(res, buffer, maxAngle, maxSamplesPerSegment,
                          CubicSegment{p0, p1, p2, p3});
            p0 = p3;
            break;
        }
    }

    return res;
}

void Curves2d::stroke(core::DoubleArray& data, double width) const
{
    // XXX TODO: actually implement this function. For now, this is just a
    // temporary implementation (no computation of normals, etc.) for testing
    // the rest of the architecture.

    // Compute adaptive sampling
    Curves2d samples = sample();

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
    core::Vec2d v0, v1;
    for (Curves2dCommandRef c : samples.commands()) {
        if (c.type() == CurveCommandType::MoveTo) {
            core::Vec2d p = c.p();
            v0 = p + inset;
            v1 = p - inset;
        }
        else if (c.type() == CurveCommandType::LineTo) {
            core::Vec2d p = c.p();
            core::Vec2d v2 = p + inset;
            core::Vec2d v3 = p - inset;
            data.insert(data.end(), {
                v0[0], v0[1], v1[0], v1[1], v2[0], v2[1],
                v2[0], v2[1], v1[0], v1[1], v3[0], v3[1]});
            v0 = v2;
            v1 = v3;
        }
    }
}

} // namespace geometry
} // namespace vgc
