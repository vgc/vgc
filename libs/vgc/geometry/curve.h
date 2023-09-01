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

#ifndef VGC_GEOMETRY_CURVE_H
#define VGC_GEOMETRY_CURVE_H

#include <vgc/core/arithmetic.h>
#include <vgc/core/array.h>
#include <vgc/core/enum.h>
#include <vgc/core/span.h>
#include <vgc/geometry/api.h>
#include <vgc/geometry/vec2d.h>

namespace vgc::geometry {

enum class CurveSamplingQuality {
    Disabled,
    UniformVeryLow,
    AdaptiveLow,
    UniformHigh,
    AdaptiveHigh,
    UniformVeryHigh,
    Max_ = UniformVeryHigh
};

VGC_GEOMETRY_API
VGC_DECLARE_ENUM(CurveSamplingQuality)

enum class CurveSnapTransformationMode {
    LinearInArclength,
};

VGC_GEOMETRY_API
VGC_DECLARE_ENUM(CurveSnapTransformationMode)

/// Specifies the type of a variable attribute along the curve, that is,
/// how it is represented.
///
enum class AttributeVariability {

    /// Represents a constant attribute. For example, a 2D attribute
    /// would be formatted as [ u, v ].
    ///
    Constant,

    /// Represents a varying attribute specified per control point. For
    /// example, a 2D attribute would be formatted as [ u0, v0, u1, v1,
    /// ..., un, vn ].
    ///
    PerControlPoint
};

class VGC_GEOMETRY_API DistanceToCurve {
public:
    DistanceToCurve() noexcept = default;

    DistanceToCurve(
        double distance,
        double angleFromTangent,
        Int segmentIndex,
        double segmentParameter) noexcept

        : distance_(distance)
        , angleFromTangent_(angleFromTangent)
        , segmentParameter_(segmentParameter)
        , segmentIndex_(segmentIndex) {
    }

    double distance() const {
        return distance_;
    }

    void setDistance(double distance) {
        distance_ = distance;
    }

    double angleFromTangent() const {
        return angleFromTangent_;
    }

    /// Returns the index of:
    /// - a segment containing a closest point, or
    /// - the index of the last sample.
    ///
    Int segmentIndex() const {
        return segmentIndex_;
    }

    /// Returns the parameter t between 0 and 1 such that
    /// `lerp(samples[segmentIndex], samples[segmentIndex + 1], t)`
    /// is a closest point.
    ///
    double segmentParameter() const {
        return segmentParameter_;
    }

private:
    double distance_ = 0;
    double angleFromTangent_ = 0;
    double segmentParameter_ = 0;
    Int segmentIndex_ = 0;
};

namespace detail {

template<typename TSample>
DistanceToCurve
distanceToCurve(const core::Array<TSample>& samples, const Vec2d& position) {

    DistanceToCurve result(core::DoubleInfinity, 0, 0, 0);
    constexpr double hpi = core::pi / 2;

    if (samples.isEmpty()) {
        return result;
    }

    auto it2 = samples.begin();
    Int i = 0;
    for (auto it1 = it2++; it2 != samples.end(); it1 = it2++, ++i) {
        const Vec2d p1 = it1->position();
        const Vec2d p2 = it2->position();
        const Vec2d p1p = position - p1;
        double d = p1p.length();
        if (d > 0) {
            Vec2d p1p2 = p2 - p1;
            double l = p1p2.length();
            if (l > 0) {
                Vec2d p1p2Dir = p1p2 / l;
                double tx = p1p2Dir.dot(p1p);
                if (tx >= 0 && tx <= l) { // does p project in segment?
                    double ty = p1p2Dir.det(p1p);
                    d = std::abs(ty);
                    if (d < result.distance()) {
                        if (d > 0) {
                            double angleFromTangent = (ty < 0) ? -hpi : hpi;
                            result = DistanceToCurve(d, angleFromTangent, i, tx / l);
                        }
                        else {
                            // (p on segment) => no better result can be found.
                            // The angle is ambiguous, we arbitrarily set to hpi.
                            return DistanceToCurve(0, hpi, i, tx / l);
                        }
                    }
                }
                else if (d < result.distance() && tx < 0) {
                    double angleFromTangent = 0;
                    if (i != 0) {
                        angleFromTangent = (it1->normal().dot(p1p) < 0) ? -hpi : hpi;
                    }
                    else {
                        angleFromTangent = it1->tangent().angle(p1p);
                    }
                    result = DistanceToCurve(d, angleFromTangent, i, 0);
                }
            }
        }
        else {
            // (p == sample) => no better result can be found.
            // The angle is ambiguous, we arbitrarily set to hpi.
            return DistanceToCurve(0, hpi, i, 0);
        }
    }

    // test last sample as point
    const TSample& sample = samples.last();
    Vec2d q = sample.position();
    Vec2d qp = position - q;
    double d = qp.length();
    if (d < result.distance()) {
        if (d > 0) {
            double angleFromTangent = sample.tangent().angle(qp);
            result = DistanceToCurve(d, angleFromTangent, i, 0);
        }
        else {
            // (p == sample) => no better result can be found.
            // The angle is ambiguous, we arbitrarily set to hpi.
            return DistanceToCurve(0, hpi, i, 0);
        }
    }

    return result;
}

} // namespace detail

/// \class vgc::geometry::CurveSamplingParameters
/// \brief Parameters for the sampling.
///
/// Using the default parameters, the sampling is "adaptive". This means that
/// the number of samples generated between two control points depends on the
/// curvature of the curve: the higher the curvature, the more samples are
/// generated. This ensures that consecutive segments never have an angle more
/// than `maxAngle` (expressed in radian), which ensures that the curve looks
/// "smooth" at any zoom level.
///
/// \verbatim
///                    _o p3
///                 _-`
///              _-` } maxAngle
/// o----------o`- - - - - - - - -
/// p1         p2
/// \endverbatim
///
/// Note that some algorithms may only take into account the angle between
/// consecutive segments of the centerline, while other may be width-aware,
/// that is, also ensure that the angle between consecutive segments of the
/// offset curves is less than `maxAngle`.
///
/// When the curve is a straight line between two control points, no intra
/// segment samples are needed. However, you can use `minIntraSegmentSamples`
/// if you wish to have at least a certain number of samples uniformly
/// generated between any two control points. Also, you can use
/// `maxIntraSegmentSamples` to limit how many samples are generated between
/// any two control points. This is necessary to break infinite loops in case
/// the curve contains a cusp between two control points.
///
/// If you wish to uniformly generate a fixed number of samples between control
/// points, simply set `maxAngle` to any value, and set
/// `minIntraSegmentSamples` and `maxIntraSegmentSamples` to the same value.
///
class VGC_GEOMETRY_API CurveSamplingParameters {
public:
    CurveSamplingParameters() = default;

    CurveSamplingParameters(CurveSamplingQuality quality);

    CurveSamplingParameters(
        double maxAngle,
        Int minIntraSegmentSamples,
        Int maxIntraSegmentSamples)

        : maxAngle_(maxAngle)
        , cosMaxAngle_(std::cos(maxAngle))
        , minIntraSegmentSamples_(minIntraSegmentSamples)
        , maxIntraSegmentSamples_(maxIntraSegmentSamples) {

        cosMaxAngle_ = std::cos(maxAngle);
    }

    double maxDs() const {
        return maxDs_;
    }

    void setMaxDs(double maxDs) {
        maxDs_ = maxDs;
    }

    double maxAngle() const {
        return maxAngle_;
    }

    void setMaxAngle(double maxAngle) {
        maxAngle_ = maxAngle;
        cosMaxAngle_ = std::cos(maxAngle);
    }

    double cosMaxAngle() const {
        return cosMaxAngle_;
    }

    Int minIntraSegmentSamples() const {
        return minIntraSegmentSamples_;
    }

    void setMinIntraSegmentSamples(Int minIntraSegmentSamples) {
        minIntraSegmentSamples_ = minIntraSegmentSamples;
    }

    Int maxIntraSegmentSamples() const {
        return maxIntraSegmentSamples_;
    }

    void setMaxIntraSegmentSamples(Int maxIntraSegmentSamples) {
        maxIntraSegmentSamples_ = maxIntraSegmentSamples;
    }

    friend bool
    operator==(const CurveSamplingParameters& a, const CurveSamplingParameters& b) {
        return (a.maxAngle_ == b.maxAngle_)
               && (a.minIntraSegmentSamples_ == b.minIntraSegmentSamples_)
               && (a.maxIntraSegmentSamples_ == b.maxIntraSegmentSamples_);
    }

    friend bool
    operator!=(const CurveSamplingParameters& a, const CurveSamplingParameters& b) {
        return !(a == b);
    }

private:
    double maxDs_ = core::DoubleInfinity;
    double maxAngle_ = 0.05; // 2PI / 0.05 ~= 125.66
    double cosMaxAngle_;
    Int minIntraSegmentSamples_ = 0;
    Int maxIntraSegmentSamples_ = 63;
    //bool isScreenspace_ = false;
};

namespace detail {

// TODO: We may want to have a lean version of AbstractStroke2d dedicated to
//       curves only (without thickness nor varying attributes). This class
//       is a draft of such interface.
//
/// \class vgc::geometry::AbstractCurve2d
/// \brief An abstract model of 2D curve (no thickness or other attributes).
///
class VGC_GEOMETRY_API AbstractCurve2d {
public:
    virtual ~AbstractCurve2d() = default;

    /// Returns whether the curve is closed.
    ///
    virtual bool isClosed() const = 0;

    /// Returns the number of segments of the curve.
    ///
    /// \sa `eval()`.
    ///
    virtual Int numSegments() const = 0;

    /// Returns the position of the curve point from segment `segmentIndex` at
    /// parameter `u`.
    ///
    virtual Vec2d eval(Int segmentIndex, double u) const = 0;

    /// Returns the position of the curve point from segment `segmentIndex` at
    /// parameter `u`. It additionally sets the value of `velocity` as the
    /// position derivative at `u` with respect to the parameter u.
    ///
    virtual Vec2d eval(Int segmentIndex, double u, Vec2d& velocity) const = 0;
};

void checkSegmentIndex(Int segmentIndex, Int numSegments);

using AdaptiveSamplingParameters = CurveSamplingParameters;

// TSample must be constructible from core::noInit.
template<typename TSample>
class AdaptiveSampler {
private:
    struct Node {
    public:
        Node() = default;

        TSample sample = core::noInit;
        double u;
        Node* previous = nullptr;
        Node* next = nullptr;
    };

public:
    // Samples the segment [data.segmentIndex, data.segmentIndex + 1], and appends the
    // result to outAppend.
    //
    // KeepPredicate signature must match:
    //   `bool(const TSample& previousSample,
    //         const TSample& sample,
    //         const TSample& nextSample)`
    //
    // The first sample of the segment is appended only if the cache `data` is new.
    // The last sample is always appended.
    //
    // `minSamples` and `maxSamples` are respectively the minimum and maximum number of samples
    // that this function should produce, including the first and last sample.
    //

    template<typename USample, typename Evaluator, typename KeepPredicate>
    void sample(
        Evaluator&& evaluator,
        KeepPredicate&& keepPredicate,
        Int minSamples,
        Int maxSamples,
        core::Array<USample>& outAppend) {

        minSamples = std::max<Int>(minSamples, 2);
        maxSamples = std::max<Int>(minSamples, maxSamples);

        resetSampleTree_(maxSamples);

        // Setup first and last sample nodes of segment.
        Node* s0 = &sampleTree_[0];
        s0->sample = evaluator(0);
        s0->u = 0;
        Node* sN = &sampleTree_[1];
        sN->sample = evaluator(1);
        sN->u = 1;
        s0->previous = nullptr;
        s0->next = sN;
        sN->previous = s0;
        sN->next = nullptr;

        Int nextNodeIndex = 2;

        // Compute `minIntraSegmentSamples` uniform samples.
        Int minISS = minSamples - 2;
        double du = 1.0 / core::narrow_cast<double>(minISS + 1);
        double u = 0;
        Node* previousNode = s0;
        for (Int i = 0; i < minISS; ++i) {
            u += du;
            Node* node = &sampleTree_[nextNodeIndex];
            node->sample = evaluator(u);
            node->u = u;
            ++nextNodeIndex;
            linkNode_(node, previousNode);
            previousNode = node;
        }

        const Int sampleTreeLength = sampleTree_.length();
        Int previousLevelStartIndex = 2;
        Int previousLevelEndIndex = nextNodeIndex;

        // Fallback to using the last sample as previous level sample
        // when we added no uniform samples.
        if (previousLevelEndIndex == 2) {
            previousLevelStartIndex = 1;
        }

        while (nextNodeIndex < sampleTreeLength) {
            // Since we create a candidate on the left and right of each previous level node,
            // each pass can add as much as twice the amount of nodes of the previous level.
            for (Int i = previousLevelStartIndex; i < previousLevelEndIndex; ++i) {
                Node* previousLevelNode = &sampleTree_[i];
                // Try subdivide left.
                if (trySubdivide_(
                        std::forward<Evaluator>(evaluator),
                        std::forward<KeepPredicate>(keepPredicate),
                        nextNodeIndex,
                        previousLevelNode->previous,
                        previousLevelNode)
                    && nextNodeIndex == sampleTreeLength) {
                    break;
                }
                // We subdivide right only if it is not the last point.
                if (!previousLevelNode->next) {
                    continue;
                }
                // Try subdivide right.
                if (trySubdivide_(
                        std::forward<Evaluator>(evaluator),
                        std::forward<KeepPredicate>(keepPredicate),
                        nextNodeIndex,
                        previousLevelNode,
                        previousLevelNode->next)
                    && nextNodeIndex == sampleTreeLength) {
                    break;
                }
            }
            if (nextNodeIndex == previousLevelEndIndex) {
                // No new candidate, let's stop here.
                break;
            }
            previousLevelStartIndex = previousLevelEndIndex;
            previousLevelEndIndex = nextNodeIndex;
        }

        Node* node = &sampleTree_[0];
        while (node) {
            outAppend.emplaceLast(node->sample);
            node = node->next;
        }
    }

    // Overload taking AdaptiveSamplingParameters.
    //
    template<typename USample, typename Evaluator, typename KeepPredicate>
    void sample(
        Evaluator&& evaluator,
        KeepPredicate&& keepPredicate,
        const AdaptiveSamplingParameters& params,
        core::Array<USample>& outAppend) {

        const Int minISS = params.minIntraSegmentSamples(); // 0 -> 2 samples minimum
        const Int maxISS = params.maxIntraSegmentSamples(); // 1 -> 3 samples maximum
        const Int minSamples = std::max<Int>(0, minISS) + 2;
        const Int maxSamples = std::max<Int>(minSamples, maxISS + 2);

        sample(
            std::forward<Evaluator>(evaluator),
            std::forward<KeepPredicate>(keepPredicate),
            minSamples,
            maxSamples,
            outAppend);
    }

private:
    core::Span<Node> sampleTree_;
    std::unique_ptr<Node[]> sampleTreeStorage_;

    void resetSampleTree_(Int newStorageLength) {
        if (newStorageLength > sampleTree_.length()) {
            sampleTreeStorage_ = std::make_unique<Node[]>(newStorageLength);
            sampleTree_ = core::Span<Node>(sampleTreeStorage_.get(), newStorageLength);
        }
    }

    template<typename Evaluator, typename KeepPredicate>
    bool trySubdivide_(
        Evaluator&& evaluator,
        KeepPredicate&& keepPredicate,
        Int& nodeIndex,
        Node* n0,
        Node* n1) {

        Node* node = &sampleTree_[nodeIndex];
        double u = 0.5 * (n0->u + n1->u);
        node->sample = evaluator(u);
        if (keepPredicate(n0->sample, node->sample, n1->sample)) {
            ++nodeIndex;
            node->u = u;
            linkNode_(node, n0);
            return true;
        }
        return false;
    };

    static void linkNode_(Node* node, Node* previous) {
        Node* next = previous->next;
        next->previous = node;
        previous->next = node;
        node->previous = previous;
        node->next = next;
    };
};

} // namespace detail

} // namespace vgc::geometry

#endif // VGC_GEOMETRY_CURVE_H
