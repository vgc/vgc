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

#include <vgc/geometry/arc.h>
#include <vgc/geometry/bezier.h>
#include <vgc/geometry/curve.h>
#include <vgc/geometry/mat2d.h>
#include <vgc/geometry/tesselator.h>

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

void Curves2d::arcTo(
    const Vec2d& r,
    double xAxisRotation,
    bool largeArcFlag,
    bool sweepFlag,
    const Vec2d& p) {

    arcTo(r.x(), r.y(), xAxisRotation, largeArcFlag, sweepFlag, p.x(), p.y());
}

void Curves2d::arcTo(
    double rx,
    double ry,
    double xAxisRotation,
    bool largeArcFlag,
    bool sweepFlag,
    double x,
    double y) {

    data_.append(x);
    data_.append(y);
    data_.append(rx);
    data_.append(ry);
    data_.append(xAxisRotation);
    data_.append(largeArcFlag ? 1.0 : 0.0);
    data_.append(sweepFlag ? 1.0 : 0.0);
    commandData_.append({CurveCommandType::ArcTo, data_.length()});
}

namespace {

template<typename SegmentType>
class CenterlineEvaluator {
    const SegmentType& seg_;

public:
    CenterlineEvaluator(const SegmentType& seg)
        : seg_(seg) {
    }

    Vec2d operator()(double u) {
        return seg_.eval(u);
    }
};

template<bool side, typename SegmentType>
class OffsetLineEvaluator {
    const SegmentType& seg_;
    double halfwidth_;

public:
    OffsetLineEvaluator(const SegmentType& seg, double halfwidth)
        : seg_(seg)
        , halfwidth_(halfwidth) {
    }

    Vec2d operator()(double u) {
        Vec2d tangent = core::noInit;
        Vec2d position = seg_.eval(u, tangent);
        if constexpr (side) {
            return position + halfwidth_ * tangent.normalized().orthogonalized();
        }
        else {
            return position - halfwidth_ * tangent.normalized().orthogonalized();
        }
    }
};

class KeepPredicate {
public:
    KeepPredicate(double minDistance, double maxAngle)
        : minDistance_(minDistance)
        , maxAngle_(maxAngle) {
    }

    bool operator()(const Vec2d& s0, const Vec2d& s1, const Vec2d& s2) {
        Vec2d u01 = s1 - s0;
        Vec2d u12 = s2 - s1;
        double d01 = u01.length();
        double d12 = u12.length();
        if (d01 < minDistance_ && d12 < minDistance_) {
            return false;
        }
        else if (std::abs(u01.angle(u12)) < maxAngle_) {
            return false;
        }
        else {
            return true;
        }
    };

private:
    double minDistance_;
    double maxAngle_;
};

// For now, using minSamples = 3 is necessary because with only
// minSamples = 2, we would fail to capture a symmetric cubic Bézier
// with an inflexion point, see:
//
//  .--. P(0.5)
// o    \    o P(1)
// P(0)  '__'
//
// In this example, P(0.5) is exactly in the middle of P(0) and P(1).
//
// Using minSamples = 2 would just evaluate P(0.5), concluding that
// it's not needed, and only output the two samples P(0) and P(1).
//
// Using minSamples = 3 forces to also evaluate P(0.25) and P(0.75),
// reducing the likelihood to miss such false negative "keep
// predicate".
//
// This is not a perfect solution, we might still miss things if we're
// unlucky. The proper solution would be to have a more advanced keep
// predicate, for example taking into account actual curve tangents, or
// an estimation of the actual curve length. Another solution might be
// to change the AdaptiveSampler itself, to not give up as soon as one
// keep predicate

template<typename TFloat>
class StrokeVisitor {
private:
    struct IndexSpan {
        Int startIndex;
        Int numIndices;
    };
    struct SegmentData {
        Vec2d startTangent;
        Vec2d endTangent;
        IndexSpan leftSamples;  // at least 2
        IndexSpan rightSamples; // at least 2
        // Note: we use indices since we cannot directly use core::Span: it
        // would be invadidated when sampling the other segments.
    };

public:
    StrokeVisitor(
        core::Array<TFloat>& data,
        double width,
        const StrokeStyle& style,
        const Curves2dSampleParams& params)

        : data_(data)
        , halfwidth_(width / 2)
        , style_(style)
        , minSamples_(3) // see note above
        , maxSamples_(params.maxSamplesPerSegment())
        , keepPredicate_(params.minDistance(), params.maxAngle()) {
    }

    void line(Vec2d p1, Vec2d p2) {

        if (p1 == p2) {
            // Skip LineTo
            return;
        }

        SegmentData data;

        Vec2d t = (p2 - p1).normalized();
        data.startTangent = t;
        data.endTangent = t;

        Vec2d offset = halfwidth_ * t.orthogonalized();
        Int leftOldLength = leftSamples_.length();
        Int rightOldLength = rightSamples_.length();
        leftSamples_.append(p1 + offset);
        rightSamples_.append(p1 - offset);
        leftSamples_.append(p2 + offset);
        rightSamples_.append(p2 - offset);
        data.leftSamples = {leftOldLength, 2};
        data.rightSamples = {rightOldLength, 2};

        segmentData_.append(data);
    }

    template<bool side, typename SegmentType>
    auto computeSegmentSampling(const SegmentType& seg, core::Array<Vec2d>& samples) {
        Int oldLength = samples.length();
        sampler_.sample(
            OffsetLineEvaluator<side, SegmentType>(seg, halfwidth_),
            keepPredicate_,
            minSamples_,
            maxSamples_,
            samples);
        Int newLength = samples.length();
        return IndexSpan{oldLength, newLength - oldLength};
    }

    template<typename SegmentType>
    void segment(const SegmentType& seg) {
        SegmentData data;
        data.startTangent = seg.evalDerivative(0).normalized();
        data.endTangent = seg.evalDerivative(1).normalized();
        data.leftSamples = computeSegmentSampling<1, SegmentType>(seg, leftSamples_);
        data.rightSamples = computeSegmentSampling<0, SegmentType>(seg, rightSamples_);
        segmentData_.append(data);
    }

    void
    addJoin_(const Vec2d& p1, const Vec2d& p2, const Vec2d& t1, const Vec2d& /*t2*/) {
        if (p1 == p2) {
            return;
        }
        Vec2d p1p2 = p2 - p1;
        double sinHalfThetaTimesP1P2 = p1p2.dot(t1);
        if (sinHalfThetaTimesP1P2 <= 0) {
            // inner join (nothing to do)
            return;
        }
        switch (style_.join()) {
        case StrokeJoin::Bevel:
            break;
        case StrokeJoin::Round: {
            Vec2d center = p1 - t1.orthogonalized() * halfwidth_;
            Vec2d xAxis = p1 - center;
            Vec2d yAxis = -xAxis.orthogonalized();
            double sinHalfTheta = sinHalfThetaTimesP1P2 / p1p2.length();
            double startAngle = 0;
            double endAngle = core::pi - 2 * std::asin(sinHalfTheta);
            auto arc = EllipticalArc2d::fromCenterParameters(
                center, xAxis, yAxis, startAngle, endAngle);
            auto evaluator = CenterlineEvaluator<EllipticalArc2d>(arc);
            sampler_.sample(
                evaluator, keepPredicate_, minSamples_, maxSamples_, capSamples_);
            if (capSamples_.length() > 2) {
                auto begin = capSamples_.cbegin();
                auto end = capSamples_.cend();
                ++begin; // don't append first sample of join
                --end;   // don't append last sample of join
                for (auto it = begin; it != end; ++it) {
                    const Vec2d& p = *it;
                    vertices_.append(Vec2f(p));
                }
            }
            break;
        }
        case StrokeJoin::Miter: {
            // https://www.w3.org/TR/SVG11/painting.html#StrokeMiterlimitProperty
            double d = p1p2.length();
            double miterRatio = d / sinHalfThetaTimesP1P2;
            if (miterRatio < style_.miterLimit()) {
                double halfMiterLength = halfwidth_ * miterRatio;
                Vec2d center = p1 - t1.orthogonalized() * halfwidth_;
                Vec2d miterDir = (p1p2 / d).orthogonalized();
                Vec2d miterPoint = center + miterDir * halfMiterLength;
                vertices_.append(Vec2f(miterPoint));
            }
            else {
                // fallback to Bevel (nothing to do)
            }
            break;
        }
        }
        capSamples_.clear();
    }

    //     1     join    2
    // o--->---o  ?  o--->---o  left samples
    //         p1    p2
    // o--->------o------>---o  centerline
    //     1             2
    //
    void addLeftJoin_(const SegmentData& segmentData1, const SegmentData& segmentData2) {
        core::Span<Vec2d> samples1 = getLeftSamples(segmentData1);
        core::Span<Vec2d> samples2 = getLeftSamples(segmentData2);
        const Vec2d& p1 = samples1.last();
        const Vec2d& p2 = samples2.first();
        const Vec2d& t1 = segmentData1.endTangent;
        const Vec2d& t2 = segmentData2.startTangent;
        addJoin_(p1, p2, t1, t2);
    }

    //     2             1
    // o---<------o------<---o  centerline
    //         p2    p1
    // o---<---o  ?  o---<---o  right samples
    //     2     join    1
    //
    void addRightJoin_(const SegmentData& segmentData1, const SegmentData& segmentData2) {
        core::Span<Vec2d> samples1 = getRightSamples(segmentData1);
        core::Span<Vec2d> samples2 = getRightSamples(segmentData2);
        const Vec2d& p1 = samples1.first();
        const Vec2d& p2 = samples2.last();
        const Vec2d& t1 = -segmentData1.startTangent;
        const Vec2d& t2 = -segmentData2.endTangent;
        addJoin_(p1, p2, t1, t2);
    }

    void addCap_(const Vec2d& p1, const Vec2d& p2) {
        switch (style_.cap()) {
        case StrokeCap::Butt:
            break;
        case StrokeCap::Round: {
            Vec2d center = 0.5 * (p1 + p2);
            Vec2d xAxis = p1 - center;
            Vec2d yAxis = -xAxis.orthogonalized();
            double startAngle = 0;
            double endAngle = core::pi;
            auto arc = EllipticalArc2d::fromCenterParameters(
                center, xAxis, yAxis, startAngle, endAngle);
            auto evaluator = CenterlineEvaluator<EllipticalArc2d>(arc);
            sampler_.sample(
                evaluator, keepPredicate_, minSamples_, maxSamples_, capSamples_);
            if (capSamples_.length() > 2) {
                auto begin = capSamples_.cbegin();
                auto end = capSamples_.cend();
                ++begin; // don't append first sample of cap
                --end;   // don't append last sample of cap
                for (auto it = begin; it != end; ++it) {
                    const Vec2d& p = *it;
                    vertices_.append(Vec2f(p));
                }
            }
            break;
        }
        case StrokeCap::Square: {
            Vec2d center = 0.5 * (p1 + p2);
            Vec2d xAxis = p1 - center;
            Vec2d yAxis = -xAxis.orthogonalized();
            vertices_.append(Vec2f(p1 + yAxis));
            vertices_.append(Vec2f(p2 + yAxis));
            break;
        }
        }
        capSamples_.clear();
    }

    void endOpenSubpath() {

        // Fast return if not enough samples.
        //
        // TODO: draw caps for single moveto and zero-length subpaths, see:
        //
        // https://www.w3.org/TR/SVG11/painting.html#StrokeProperties
        //
        // > A subpath (see Paths) consisting of a single moveto shall not
        // > be stroked. Any zero length subpath shall not be stroked if
        // > the ‘stroke-linecap’ property has a value of butt but shall be
        // > stroked if the ‘stroke-linecap’ property has a value of round
        // > or square, producing respectively a circle or a square
        // > centered at the given point. Examples of zero length subpaths
        // > include 'M 10,10 L 10,10', 'M 20,20 h 0', 'M 30,30 z' and
        // > 'M 40,40 c 0,0 0,0 0,0'.
        //
        Int numSegments = segmentData_.length();
        if (numSegments == 0) {
            return;
        }

        // TODO: fast path for Bevel?
        // TODO: handling of zero-length segments? (e.g., like in Stroke2d)

        // Left samples
        for (Int k = 0; k < numSegments; ++k) {
            const SegmentData& segmentData = segmentData_.getUnchecked(k);
            if (k > 0) {
                const SegmentData& prevSegmentData = segmentData_.getUnchecked(k - 1);
                addLeftJoin_(prevSegmentData, segmentData);
            }
            core::Span<Vec2d> samples = getLeftSamples(segmentData);
            for (const Vec2d& p : samples) {
                vertices_.append(Vec2f(p));
            }
        }
        addCap_(leftSamples_.last(), rightSamples_.last());

        // Right samples
        for (Int k = numSegments - 1; k >= 0; --k) {
            const SegmentData& segmentData = segmentData_.getUnchecked(k);
            if (k < numSegments - 1) {
                const SegmentData& nextSegmentData = segmentData_.getUnchecked(k + 1);
                addRightJoin_(nextSegmentData, segmentData);
            }
            core::Span<Vec2d> samples = getRightSamples(segmentData);
            for (auto it = samples.rbegin(); it != samples.rend(); ++it) {
                vertices_.append(Vec2f(*it));
            }
        }
        addCap_(rightSamples_.first(), leftSamples_.first());

        addContour_();
        endSubpath_();
    }

    void endClosedSubpath() {

        Int numSegments = segmentData_.length();
        if (numSegments == 0) {
            return;
        }

        // Left samples
        for (Int k = 0; k < numSegments; ++k) {
            const SegmentData& segmentData = segmentData_.getUnchecked(k);
            Int prevIndex = (k + numSegments - 1) % numSegments;
            const SegmentData& prevSegmentData = segmentData_.getUnchecked(prevIndex);
            addLeftJoin_(prevSegmentData, segmentData);
            core::Span<Vec2d> samples = getLeftSamples(segmentData);
            for (const Vec2d& p : samples) {
                vertices_.append(Vec2f(p));
            }
        }
        addContour_();

        // Right samples
        for (Int k = numSegments - 1; k >= 0; --k) {
            const SegmentData& segmentData = segmentData_.getUnchecked(k);
            Int nextIndex = (k + 1) % numSegments;
            const SegmentData& nextSegmentData = segmentData_.getUnchecked(nextIndex);
            addRightJoin_(nextSegmentData, segmentData);
            core::Span<Vec2d> samples = getRightSamples(segmentData);
            for (auto it = samples.rbegin(); it != samples.rend(); ++it) {
                vertices_.append(Vec2f(*it));
            }
        }
        addContour_();

        endSubpath_();
    }

    void addContour_() {
        tess_.addContour(vertices_);
        vertices_.clear();
    }

    void endSubpath_() {
        segmentData_.clear();
        leftSamples_.clear();
        rightSamples_.clear();
    }

    void endCurves() {
        tess_.tesselate(data_, WindingRule::NonZero);
    }

private:
    // Final output
    core::Array<TFloat>& data_;

    // Input params
    double halfwidth_;
    const StrokeStyle& style_;
    Int minSamples_;
    Int maxSamples_;

    // Buffers
    detail::AdaptiveSampler<Vec2d> sampler_;
    KeepPredicate keepPredicate_;
    Tesselator tess_;

    // Subpath buffers
    core::Span<Vec2d> getLeftSamples(const SegmentData& segmentData) {
        IndexSpan indices = segmentData.leftSamples;
        core::Span span = leftSamples_;
        return span.subspan(indices.startIndex, indices.numIndices);
    };
    core::Span<Vec2d> getRightSamples(const SegmentData& segmentData) {
        IndexSpan indices = segmentData.rightSamples;
        core::Span<Vec2d> span = rightSamples_;
        return span.subspan(indices.startIndex, indices.numIndices);
    };
    core::Array<SegmentData> segmentData_;
    core::Array<Vec2d> leftSamples_;
    core::Array<Vec2d> rightSamples_;
    core::Array<Vec2d> capSamples_;
    core::Array<Vec2f> vertices_;
    // Note: we use Vec2f for vertices, otherwise Tesselator would cast them
    // anyway to a temporary buffer of floats.
};

template<typename TFloat>
class FillVisitor {
public:
    FillVisitor(
        core::Array<TFloat>& data,
        FillStyle fillStyle,
        const Curves2dSampleParams& params)

        : data_(data)
        , windingRule_(fillStyle.windingRule())
        , minSamples_(3) // see note above StrokeVisitor
        , maxSamples_(params.maxSamplesPerSegment())
        , keepPredicate_(params.minDistance(), params.maxAngle()) {
    }

    void line(Vec2d p1, Vec2d p2) {
        if (p1 == p2) {
            // Skip LineTo
            return;
        }
        samples_.append(p1);
        samples_.append(p2);
        // TODO: Do not add p1 if already there from last sample?
    }

    template<typename SegmentType>
    void segment(const SegmentType& seg) {
        sampler_.sample(
            CenterlineEvaluator<SegmentType>(seg),
            keepPredicate_,
            minSamples_,
            maxSamples_,
            samples_);
        // TODO: Do not add first sample if already there from last sample?
    }

    void endOpenSubpath() {
        tess_.addContour(samples_);
        samples_.clear();
    }

    void endClosedSubpath() {
        endOpenSubpath();
    }

    void endCurves() {
        tess_.tesselate(data_, windingRule_);
    }

private:
    // Final output
    core::Array<TFloat>& data_;

    // Input params
    WindingRule windingRule_;
    Int minSamples_;
    Int maxSamples_;

    // Buffers
    detail::AdaptiveSampler<Vec2d> sampler_;
    KeepPredicate keepPredicate_;
    Tesselator tess_;
    core::Array<Vec2d> samples_;
};

template<typename Visitor>
void visit_(const Curves2d& curves, Visitor& visitor) {

    // Note: if the first command is not a MoveTo, we behave as if there was an
    // implicit MoveTo(0, 0) before the first command
    //
    CurveCommandType lastCommandType = CurveCommandType::MoveTo;
    Vec2d firstPointOfSubpath(0, 0);
    Vec2d currentPoint(0, 0);

    for (Curves2dCommandRef c : curves.commands()) {
        switch (c.type()) {
        case CurveCommandType::Close:
            if (currentPoint != firstPointOfSubpath) {
                // Implicit LineTo
                visitor.line(currentPoint, firstPointOfSubpath);
                currentPoint = firstPointOfSubpath;
            }
            if (lastCommandType != CurveCommandType::Close
                && lastCommandType != CurveCommandType::MoveTo) {

                visitor.endClosedSubpath();
            }
            // Note: a Close followed by a Close or a MoveTo followed by a
            // Close does nothing. There is even no need to update
            // firstPointOfSubpath, since the next subpath will have the same
            // first point as the previous one, unless a MoveTo is called.
            break;
        case CurveCommandType::MoveTo:
            // A Close followed by a MoveTo or a MoveTo followed by a MoveTo is ignored.
            if (lastCommandType != CurveCommandType::Close
                && lastCommandType != CurveCommandType::MoveTo) {

                visitor.endOpenSubpath();
            }
            currentPoint = c.p();
            firstPointOfSubpath = currentPoint;
            // Note: a Close followed by a MoveTo or a MoveTo followed by a
            // MoveTo does not create a new subpath, but we still need to update
            // currentPoint and firstPointOfSubpath.
            break;
        case CurveCommandType::LineTo:
            visitor.line(currentPoint, c.p());
            currentPoint = c.p();
            break;
        case CurveCommandType::QuadraticBezierTo:
            visitor.segment(QuadraticBezier2d(currentPoint, c.p1(), c.p2()));
            currentPoint = c.p2();
            break;
        case CurveCommandType::CubicBezierTo:
            visitor.segment(CubicBezier2d(currentPoint, c.p1(), c.p2(), c.p3()));
            currentPoint = c.p3();
            break;
        case CurveCommandType::ArcTo:
            visitor.segment(EllipticalArc2d::fromSvgParameters(
                currentPoint,
                c.p(),
                c.r(),
                c.xAxisRotation(),
                c.largeArcFlag(),
                c.sweepFlag()));
            currentPoint = c.p();
            break;
        }
        lastCommandType = c.type();
    }

    if (lastCommandType != CurveCommandType::Close
        && lastCommandType != CurveCommandType::MoveTo) {

        visitor.endOpenSubpath();
    }

    visitor.endCurves();
}

} // namespace

void Curves2d::stroke(
    core::DoubleArray& data,
    double width,
    const StrokeStyle& style,
    const Curves2dSampleParams& params) const {

    StrokeVisitor<double> visitor(data, width, style, params);
    visit_(*this, visitor);
}

void Curves2d::stroke(
    core::FloatArray& data,
    double width,
    const StrokeStyle& style,
    const Curves2dSampleParams& params) const {

    StrokeVisitor<float> visitor(data, width, style, params);
    visit_(*this, visitor);
}

void Curves2d::fill(
    core::DoubleArray& data,
    FillStyle fillStyle,
    const Curves2dSampleParams& params) const {

    FillVisitor<double> visitor(data, fillStyle, params);
    visit_(*this, visitor);
}

void Curves2d::fill(
    core::FloatArray& data,
    FillStyle fillStyle,
    const Curves2dSampleParams& params) const {

    FillVisitor<float> visitor(data, fillStyle, params);
    visit_(*this, visitor);
}

} // namespace vgc::geometry
