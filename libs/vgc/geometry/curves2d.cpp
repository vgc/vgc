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

#include <vgc/geometry/curves2d.h>

#include <tesselator.h> // libtess2

namespace vgc::geometry {

void Curves2d::close() {
    commandData_.append({CurveCommandType::Close, data_.length()});
}

void Curves2d::moveTo(const Vec2d& p) {
    moveTo(p[0], p[1]);
}

void Curves2d::moveTo(double x, double y) {
    data_.append(x);
    data_.append(y);
    commandData_.append({CurveCommandType::MoveTo, data_.length()});
}

void Curves2d::lineTo(const Vec2d& p) {
    lineTo(p[0], p[1]);
}

void Curves2d::lineTo(double x, double y) {
    // TODO (for all functions): support subcommands, that is, don't add a new
    // command if the last command is the same. We should simply add more params.
    data_.append(x);
    data_.append(y);
    commandData_.append({CurveCommandType::LineTo, data_.length()});
}

void Curves2d::quadraticBezierTo(const Vec2d& p1, const Vec2d& p2) {
    quadraticBezierTo(p1[0], p1[1], p2[0], p2[1]);
}

void Curves2d::quadraticBezierTo(double x1, double y1, double x2, double y2) {
    data_.append(x1);
    data_.append(y1);
    data_.append(x2);
    data_.append(y2);
    commandData_.append({CurveCommandType::QuadraticBezierTo, data_.length()});
}

void Curves2d::cubicBezierTo(const Vec2d& p1, const Vec2d& p2, const Vec2d& p3) {
    cubicBezierTo(p1[0], p1[1], p2[0], p2[1], p3[0], p3[1]);
}

void Curves2d::cubicBezierTo(
    double x1,
    double y1,
    double x2,
    double y2,
    double x3,
    double y3) {

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
    Vec2d position;
};

struct SampleBuffer {
    core::Array<Sample> samples;
    core::IntArray failed;
    core::IntArray added;
};

struct QuadraticSegment {
    Vec2d p0, p1, p2;

    Vec2d startPosition() const {
        return p0;
    }

    Vec2d endPosition() const {
        return p2;
    }

    Vec2d startTangent() const {
        return p1 - p0;
    }

    Vec2d endTangent() const {
        return p2 - p1;
    }

    Vec2d operator()(double u) const {
        return (1 - u) * (1 - u) * p0 //
               + 2 * u * (1 - u) * p1 //
               + u * u * p2;
    }
};

struct CubicSegment {
    Vec2d p0, p1, p2, p3;
    Vec2d startPosition() const {
        return p0;
    }
    Vec2d endPosition() const {
        return p3;
    }
    Vec2d startTangent() const {
        return p1 - p0;
    }
    Vec2d endTangent() const {
        return p3 - p2;
    }
    Vec2d operator()(double u) const {
        return (1 - u) * (1 - u) * (1 - u) * p0 //
               + 3 * u * (1 - u) * (1 - u) * p1 //
               + 3 * u * u * (1 - u) * p2       //
               + u * u * u * p3;
    }
};

template<typename SegmentType>
void sampleSegment(
    Curves2d& res,
    SampleBuffer& buffer,
    const Curves2dSampleParams& params,
    SegmentType segment) {

    double minDistanceSquared = params.minDistance() * params.minDistance();
    double maxAngle = params.maxAngle();
    Int maxSamplesPerSegment = params.maxSamplesPerSegment();

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

        // Find which angles are too large and followed by a segment
        // longer than the minDistance.
        failed.clear();
        for (Int i = 1; i < samples.length() - 1; ++i) {
            Vec2d a = samples[i].position - samples[i - 1].position;
            Vec2d b = samples[i + 1].position - samples[i].position;
            if (std::abs(a.angle(b)) > maxAngle
                && b.squaredLength() > minDistanceSquared) {

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
                added.append(i + 1);
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
            double u = 0.5 * (samples[i - 1].u + samples[i + 1].u);
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

Curves2d Curves2d::sample(const Curves2dSampleParams& params) const {

    Curves2d res;
    Vec2d p0, p1, p2, p3;
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
            sampleSegment(res, buffer, params, QuadraticSegment{p0, p1, p2});
            p0 = p2;
            break;
        case vgc::geometry::CurveCommandType::CubicBezierTo:
            p1 = c.p1();
            p2 = c.p2();
            p3 = c.p3();
            sampleSegment(res, buffer, params, CubicSegment{p0, p1, p2, p3});
            p0 = p3;
            break;
        }
    }

    return res;
}

namespace {

//    a    left-side    c
//    o---------------->o
//    |                 |
//    |                 |
//    o---------------->o
//    b   right-side    d
//
void insertQuad(
    core::DoubleArray& data,
    const Vec2d& a,
    const Vec2d& b,
    const Vec2d& c,
    const Vec2d& d) {

    // Two triangles: ABC and CBD
    data.insert(
        data.end(),
        {a[0],
         a[1],
         b[0],
         b[1],
         c[0],
         c[1], //
         c[0],
         c[1],
         b[0],
         b[1],
         d[0],
         d[1]});
}

void editQuadData(core::DoubleArray& data, Int i, const Vec2d& a, const Vec2d& b) {
    data[i + 0] = a[0];
    data[i + 1] = a[1];
    data[i + 2] = b[0];
    data[i + 3] = b[1];
    data[i + 8] = b[0];
    data[i + 9] = b[1];
}

// Each of the "process" methods computes n1, l1, and r1 based
// on c0, c1, c2, and n0:
//
//    l0  left-side     l1              l2
//    o---------------->o-------------->o
//    |       ^         |       ^       |
//    |       | n0      |c1     | n1    |
// c0 o---------------->o-------------->o c2
//    |   centerline    |               |
//    |                 |               |
//    o---------------->o-------------->o
//    r0  right-side    r1              r3
//
// For the first sample, we don't have c0 and n0.
// For the last sample, we don't have c2.
//
void processFirstSample(
    core::DoubleArray& /*data*/,
    double width,
    const Vec2d& c1,
    const Vec2d& c2,
    Vec2d& l1,
    Vec2d& r1,
    Vec2d& n1) {

    n1 = (c2 - c1).normalize().orthogonalized();
    l1 = c1 + 0.5 * width * n1;
    r1 = c1 - 0.5 * width * n1;
}

void processMiddleSample(
    core::DoubleArray& data,
    double width,
    const Vec2d& /*c0*/,
    const Vec2d& c1,
    const Vec2d& c2,
    const Vec2d& l0,
    Vec2d& l1,
    const Vec2d& r0,
    Vec2d& r1,
    const Vec2d& n0,
    Vec2d& n1) {

    // Compute n1
    n1 = (c2 - c1).normalize().orthogonalized();

    // Compute miterLenght. We have miterLength = width / sin(t/2)
    // where t is the angle between the two segments. See:
    //
    // https://www.w3.org/TR/SVG2/painting.html#StrokeMiterlimitProperty.
    //
    // We use the double angle formula giving sin(t/2) = sqrt((1-cos(t))/2),
    // and since n0 and n1 are normal vectors, cos(t) is just a dot product.
    //
    double cost = n0.dot(-n1);
    double sint2 = std::sqrt(0.5 * (1 - cost));
    double miterLength = width / sint2; // TODO: handle miterclip (sint2 close to 0)
    Vec2d miterDir = (n0 + n1).normalized();
    l1 = c1 + 0.5 * miterLength * miterDir;
    r1 = c1 - 0.5 * miterLength * miterDir;

    // Insert
    insertQuad(data, l0, r0, l1, r1);
}

void processLastOpenSample(
    core::DoubleArray& data,
    double width,
    const Vec2d& c0,
    const Vec2d& c1,
    const Vec2d& l0,
    Vec2d& l1,
    const Vec2d& r0,
    Vec2d& r1,
    const Vec2d& /*n0*/,
    Vec2d& n1) {
    n1 = (c1 - c0).normalize().orthogonalized();
    l1 = c1 + 0.5 * width * n1;
    r1 = c1 - 0.5 * width * n1;
    insertQuad(data, l0, r0, l1, r1);
}

} // namespace

void Curves2d::stroke(
    core::DoubleArray& data,
    double width,
    const Curves2dSampleParams& params) const {

    // Compute adaptive sampling
    Curves2d samples = sample(params);

    // Stroke samples
    Int numSamples = 0;
    Int firstVertexIndex = data.length();
    Vec2d firstPoint, secondPoint;
    Vec2d c0, c1, c2, l0, l1, r0, r1, n0, n1;
    for (Curves2dCommandRef c : samples.commands()) {
        if (c.type() == CurveCommandType::MoveTo) {
            if (numSamples > 1) {
                processLastOpenSample(data, width, c0, c1, l0, l1, r0, r1, n0, n1);
            }
            firstVertexIndex = data.length();
            firstPoint = c.p();
            c1 = firstPoint;
            numSamples = 1;
        }
        else if (c.type() == CurveCommandType::LineTo) {
            c2 = c.p();
            if (numSamples == 1) {
                secondPoint = c2;
                processFirstSample(data, width, c1, c2, l1, r1, n1);
            }
            else {
                processMiddleSample(data, width, c0, c1, c2, l0, l1, r0, r1, n0, n1);
            }
            c0 = c1;
            c1 = c2;
            l0 = l1;
            r0 = r1;
            n0 = n1;
            numSamples += 1;
        }
        else if (c.type() == CurveCommandType::Close) {
            if (numSamples > 2) {
                c2 = secondPoint;
                processMiddleSample(data, width, c0, c1, c2, l0, l1, r0, r1, n0, n1);
                editQuadData(data, firstVertexIndex, l1, r1);
                numSamples = 0;
            }
        }
    }
    if (numSamples > 1) {
        processLastOpenSample(data, width, c0, c1, l0, l1, r0, r1, n0, n1);
    }
}

namespace {

int getTessWindingRule(WindingRule windingRule) {
    switch (windingRule) {
    case WindingRule::Odd:
        return TESS_WINDING_ODD;
    case WindingRule::NonZero:
        return TESS_WINDING_NONZERO;
    case WindingRule::Positive:
        return TESS_WINDING_POSITIVE;
    case WindingRule::Negative:
        return TESS_WINDING_NEGATIVE;
    }
    return TESS_WINDING_NONZERO;
}

template<typename TFloat>
void fill_(core::Array<TFloat>& data, const Curves2d& samples, WindingRule windingRule_) {
    // Triangulate using libtess2
    TESSalloc* alloc = nullptr; // Default allocator
    int vertexSize = 2;         // Number of coordinates per vertex (must be 2 or 3)
    int windingRule = getTessWindingRule(windingRule_); // Winding rule
    int elementType = TESS_POLYGONS;
    // ^ Use sequence of polygons as output. Note: we could use
    // TESS_CONNECTED_POLYGONS to detect which edges have no neighbor
    // polygons, which can be useful for anti-aliasing.
    int maxPolySize = 3;                 // Triangles only
    const TESSreal normal[] = {0, 0, 1}; // Normal for 2D points is the Z unit vector
    TESStesselator* tess = tessNewTess(alloc);
    core::Array<TESSreal> coords;
    for (Curves2dCommandRef c : samples.commands()) {
        if (c.type() == CurveCommandType::MoveTo) {
            coords.clear();
            Vec2d p = c.p();
            coords.append(static_cast<TESSreal>(p[0]));
            coords.append(static_cast<TESSreal>(p[1]));
            // Note: currently, TESSreal == float.
            // Ideally, we'd like to be able to tesslate using either floats
            // or doubles. Once question would still be, in the case of
            // Curves2d::fill(FloatArray& out, ...), should we first cast
            // the contours to float then tesselate in float, or keep the
            // contours in double then tesselate in double then cast to float?
        }
        else if (c.type() == CurveCommandType::LineTo) {
            Vec2d p = c.p();
            coords.append(static_cast<TESSreal>(p[0]));
            coords.append(static_cast<TESSreal>(p[1]));
        }
        else if (c.type() == CurveCommandType::Close) {
            if (coords.size() > 4) { // ignore contour if 2 points or less
                tessAddContour(
                    tess,
                    vertexSize,
                    coords.data(),
                    sizeof(TESSreal) * vertexSize,
                    core::int_cast<int>(coords.length() / 2));
            }
        }
    }
    int success =
        tessTesselate(tess, windingRule, elementType, maxPolySize, vertexSize, normal);
    if (success) {
        const TESSreal* vertices = tessGetVertices(tess);
        const TESSindex* polygons = tessGetElements(tess);
        const int numPolygons = tessGetElementCount(tess);
        Int numOutputVertices = 0;
        for (int i = 0; i < numPolygons; ++i) {
            const TESSindex* p = &polygons[i * maxPolySize];
            int polySize = maxPolySize;
            while (p[polySize - 1] == TESS_UNDEF) {
                --polySize;
            }
            numOutputVertices += 6 * (polySize - 2);
        }
        data.reserve(data.length() + numOutputVertices);
        for (int i = 0; i < numPolygons; ++i) {
            const TESSindex* p = &polygons[i * maxPolySize];
            int polySize = maxPolySize;
            while (p[polySize - 1] == TESS_UNDEF) {
                --polySize;
            }
            for (int j = 0; j < polySize - 2; ++j) { // triangle fan
                const TESSreal* v1 = &vertices[p[j] * vertexSize];
                const TESSreal* v2 = &vertices[p[j + 1] * vertexSize];
                const TESSreal* v3 = &vertices[p[j + 2] * vertexSize];
                data.append(static_cast<TFloat>(v1[0]));
                data.append(static_cast<TFloat>(v1[1]));
                data.append(static_cast<TFloat>(v2[0]));
                data.append(static_cast<TFloat>(v2[1]));
                data.append(static_cast<TFloat>(v3[0]));
                data.append(static_cast<TFloat>(v3[1]));
            }
        }
    }
    else {
        // TODO: error reporting?
    }
}

} // namespace

void Curves2d::fill(
    core::DoubleArray& data,
    const Curves2dSampleParams& params,
    WindingRule windingRule) const {

    fill_(data, sample(params), windingRule);
}

void Curves2d::fill(
    core::FloatArray& data,
    const Curves2dSampleParams& params,
    WindingRule windingRule) const {

    fill_(data, sample(params), windingRule);
}

} // namespace vgc::geometry
