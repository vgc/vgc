// Copyright 2022 The VGC Developers
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

// This file was automatically generated, please do not edit directly.
// Instead, edit tools/range1x.h then run tools/generate.py.

// clang-format off

#ifndef VGC_GEOMETRY_RANGE1D_H
#define VGC_GEOMETRY_RANGE1D_H

#include <algorithm> // minmax

#include <vgc/core/arithmetic.h>
#include <vgc/core/array.h>
#include <vgc/geometry/api.h>

namespace vgc::geometry {

/// \class vgc::core::Range1d
/// \brief 2D range using double-precision floating points.
///
/// This class represents a 1D range using double-precision floating points.
///
/// The range is internally represented as a min value `pMin()` and a max
/// value `pMax()`. You can create a range by providing these min/max
/// values directly:
///
/// -  `Range1d(double min, double max)`
///
/// If `pMin() > pMax()`, then the range is considered empty
/// (`isEmpty()` will return true), and the `unitedWith()` operation may not
/// work as you may expect, see its documentation for more details.
///
/// Alternatively, you can create a `Range1d` by providing its `position` and
/// `size`:
///
/// - `Range1d::fromPositionSize(double position, double size)`
///
/// If `size < 0`, then the range is considered empty.
///
/// If you want to make sure that a range isn't empty, you can create a
/// `Range1d` then call `normalize()` or `normalized()`, which swaps the min and
/// max coordinates such that `pMin() <= pMax()`.
///
/// Note that a range with `pMin() == pMax()` isn't considered empty: it is simply reduced to a segment or a point.
///
// VGC_GEOMETRY_API <- Omitted on purpose.
//                     If needed, manually export individual functions.
class Range1d {
public:
    using ScalarType = double;
    static constexpr Int dimension = 1;

    VGC_WARNING_PUSH
    VGC_WARNING_MSVC_DISABLE(26495) // member variable uninitialized
    /// Creates an uninitialized `Range1d`.
    ///
    Range1d(core::NoInit) noexcept {
    }
    VGC_WARNING_POP

    /// Creates a zero-initialized `Range1d`.
    ///
    /// This is equivalent to `Range1d(0, 0)`.
    ///
    constexpr Range1d() noexcept
        : pMin_()
        , pMax_() {
    }

    /// Creates a `Range1d` defined by the two points `pMin` and `pMax`.
    ///
    /// The range is considered empty if the following condition is true:
    ///
    /// - `pMin > pMax`
    ///
    /// You can ensure that the range isn't empty by calling `normalize()`
    /// after this constructor.
    ///
    constexpr Range1d(double pMin, double pMax) noexcept
        : pMin_(pMin)
        , pMax_(pMax) {
    }


    /// Creates a `Range1d` from a `position` and `size`.
    ///
    /// This is equivalent to `Range1d(position, position + size)`.
    ///
    /// If `size < 0`, then the range is considered empty.
    ///
    /// You can ensure that the range isn't empty by calling `normalize()`
    /// after this function.
    ///
    static constexpr Range1d fromPositionSize(
        double position, double size) {

        return Range1d(position, position + size);
    }

    /// The empty `Range1d` defined by `Range1d(inf, -inf)` where
    /// `inf = core::infinity<double>`.
    ///
    /// Note that this is not the only possible empty range: for example,
    /// `Range1d(1, 0)` is also empty. However, `Range1d::empty` is the only
    /// empty range that satisfy `range.unite(empty) == range` for all
    /// ranges, and therefore is typically the most useful empty range.
    ///
    static const Range1d empty;

    /// Returns whether the range is empty.
    ///
    /// A range is considered empty if and only if `size() < 0`.
    ///
    /// Equivalently, a range is considered empty if and only if
    /// `pMin() > pMax()`.
    ///
    constexpr bool isEmpty() const {
        return pMin_ > pMax_;
    }

    /// Normalizes in-place the range, that is, makes it non-empty by
    /// swapping its values such that `pMin() <= pMax()`.
    ///
    constexpr Range1d& normalize() {
        if (pMin_ > pMax_) {
            std::swap(pMin_, pMax_);
        }
        return *this;
    }

    /// Returns a normalized version of this range, that is, a non-empty
    /// version obtained by swapping its coordinates such that `pMin() <=
    /// pMax()`.
    ///
    constexpr Range1d normalized() {
        std::pair<double, double> p = std::minmax(pMin_, pMax_);
        return Range1d(p.first, p.second);
    }

    /// Returns the `position()` of the range.
    ///
    /// This is equivalent to `pMin()`.
    ///
    constexpr double position() const {
        return pMin_;
    }

    /// Updates the `position()` of the range, while keeping its `size()`
    /// constant. This modifies both `pMin()` and `pMax()`.
    ///
    constexpr void setPosition(double position) {
        pMax_ += (position - pMin_);
        pMin_ = position;
    }

    /// Returns the size of the range.
    ///
    /// This is equivalent to `pMax() - pMin()`.
    ///
    constexpr double size() const {
        return pMax_ - pMin_;
    }

    /// Updates the `size()` of the range, while keeping its `position()`
    /// constant. This modifies `pMax()` but not `pMin()`.
    ///
    constexpr void setSize(double size) {
        pMax_ = pMin_ + size;
    }

    /// Returns the min position of the range.
    ///
    constexpr double pMin() const {
        return pMin_;
    }

    /// Updates the min position `pMin()` of the range, while keeping the max
    /// value `pMax()` constant. This modifies both `position()` and `size()`.
    ///
    constexpr void setPMin(double pMin) {
        pMin_ = pMin;
    }

    /// Returns the max position of the range.
    ///
    constexpr double pMax() const {
        return pMax_;
    }

    /// Updates the max position `pMax()` of the range, while keeping the min
    /// position `pMin()` constant. This modifies `size()` but not `position()`.
    ///
    constexpr void setPMax(double pMax) {
        pMax_ = pMax;
    }

    /// Returns whether the two ranges `r1` and `r2` are equal.
    ///
    friend constexpr bool operator==(const Range1d& r1, const Range1d& r2) {
        return r1.pMin_ == r2.pMin_
            && r1.pMax_ == r2.pMax_;
    }

    /// Returns whether the two ranges `r1` and `r2` are different.
    ///
    friend constexpr bool operator!=(const Range1d& r1, const Range1d& r2) {
        return !(r1 == r2);
    }

    /// Returns whether this range and the given `other` range are
    /// almost equal within some relative tolerance, that is, if all their
    /// corresponding bounds are close to each other in the sense of
    /// `core::isClose()`.
    ///
    /// If you need an absolute tolerance (which is especially important some of
    /// the range bounds could be exactly zero), you should use `isNear()`
    /// instead.
    ///
    bool isClose(const Range1d& other, double relTol = 1e-9, double absTol = 0.0) const {
        return core::isClose(pMin_, other.pMin_, relTol, absTol)
            && core::isClose(pMax_, other.pMax_, relTol, absTol);
    }

    /// Returns whether the euclidean distances between bounds of this
    /// range and the corresponding bounds of the `other` range are
    /// all smaller or equal than the given absolute tolerance.
    ///
    /// If you need a relative tolerance rather than an absolute tolerance, you
    /// should use `isClose()` instead.
    ///
    bool isNear(const Range1d& other, double absTol) const {
        return core::isNear(pMin_, other.pMin_, absTol)
            && core::isNear(pMax_, other.pMax_, absTol);
    }

    /// Returns the smallest range that contains both this range
    /// and the `other` range.
    ///
    /// ```
    /// Range1d r1(0, 1);
    /// Range1d r2(2, 3);
    /// Range1d r3 = r1.unitedWith(r2);            // == Range1d(0, 3)
    /// Range1d r4 = r1.unitedWith(Range1d.empty); // == Range1d(0, 1)
    /// ```
    ///
    /// Note that this function does not explicitly check whether ranges
    /// are empty, and simply computes the minimum of the min corners and the
    /// maximum of the max corners.
    ///
    /// Therefore, `r1.unitedWith(r2)` may return a range larger than `r1` even
    /// if `r2` is empty, as demonstrated below:
    ///
    /// ```
    /// Range1d r1(0, 1);
    /// Range1d r2(3, 2);
    /// bool b = r2.isEmpty();          // == true
    /// Range1d r3 = r1.unitedWith(r2); // == Range1d(0, 2) (!)
    /// ```
    ///
    /// This behavior may be surprising at first, but it is useful for
    /// performance reasons as well as continuity reasons. Indeed, a small
    /// pertubations of the input will never result in a large perturbations of
    /// the output:
    ///
    /// ```
    /// Range1d r1(0, 1);
    /// Range1d r2(1.9, 2);
    /// Range1d r3(2.0, 2);
    /// Range1d r4(2.1, 2);
    /// bool b2 = r2.isEmpty();        // == false
    /// bool b3 = r3.isEmpty();        // == false
    /// bool b4 = r4.isEmpty();        // == true
    /// Range1d s2 = r1.unitedWith(r2); // == Range1d(0, 2)
    /// Range1d s3 = r1.unitedWith(r3); // == Range1d(0, 2)
    /// Range1d s4 = r1.unitedWith(r4); // == Range1d(0, 2)
    /// ```
    ///
    /// This behavior is intended and will not change in future versions, so
    /// you can rely on it for your algorithms.
    ///
    constexpr Range1d unitedWith(const Range1d& other) const {
        return Range1d(
            (std::min)(pMin_, other.pMin_),
            (std::max)(pMax_, other.pMax_));
    }

    /// Returns the smallest range that contains both this range
    /// and the given `point`.
    ///
    /// This is equivalent to `unitedWith(Range1d(point))`.
    ///
    /// See `unitedWith(const Range1d&)` for more details, in particular about
    /// how it handles empty ranges: uniting an empty range with a
    /// point may result in a range larger than just the point.
    ///
    /// However, uniting `Range1d::empty` with a point always results in the
    /// range reduced to just the point.
    ///
    constexpr Range1d unitedWith(double point) const {
        return Range1d(
            (std::min)(pMin_, point),
            (std::max)(pMax_, point));
    }

    /// Unites this range in-place with the `other` range.
    ///
    /// See `unitedWith(other)` for more details, in particular about how it
    /// handles empty ranges (uniting with an empty range may increase the size
    /// of this range).
    ///
    constexpr Range1d& uniteWith(const Range1d& other) {
        *this = unitedWith(other);
        return *this;
    }

    /// Returns the smallest range that contains both this range
    /// and the given `point`.
    ///
    /// This is equivalent to `uniteWith(Range1d(point))`.
    ///
    /// See `unitedWith(const Range1d&)` for more details, in particular about
    /// how it handles empty ranges: uniting an empty range with a
    /// point may result in a range larger than just the point.
    ///
    /// However, uniting `Range1d::empty` with a point always results in the
    /// range reduced to just the point.
    ///
    constexpr Range1d& uniteWith(double other) {
        *this = unitedWith(other);
        return *this;
    }

    /// Returns the intersection between this range and the `other`
    /// range.
    ///
    /// ```
    /// Range1d r1(0, 3);
    /// Range1d r2(2, 4);
    /// Range1d r3(5, 6);
    /// Range1d r4(2, 1); // (empty)
    ///
    /// Range1d s2 = r1.intersectedWith(r2);            // == Range1d(2, 3)
    /// Range1d s3 = r1.intersectedWith(r3);            // == Range1d(5, 3)  (empty)
    /// Range1d s4 = r1.intersectedWith(r4);            // == Range1d(2, 1)  (empty)
    /// Range1d s5 = r1.intersectedWith(Range1d.empty); // == Range1d.empty        (empty)
    /// ```
    ///
    /// This function simply computes the maximum of the min corners and the
    /// minimum of the max corners.
    ///
    /// Unlike `unitedWith()`, this always work as you would expect, even when
    /// intersecting with empty ranges. In particular, the intersection
    /// with an empty range always results in an empty range.
    ///
    constexpr Range1d intersectedWith(const Range1d& other) const {
        return Range1d(
            (std::max)(pMin_, other.pMin_),
            (std::min)(pMax_, other.pMax_));
    }

    /// Intersects this range in-place with the `other` range.
    ///
    /// See `intersectedWith(other)` for more details.
    ///
    constexpr Range1d& intersectWith(const Range1d& other) {
        *this = intersectedWith(other);
        return *this;
    }

    /// Returns whether this range has a non-empty intersection with the
    /// `other` range.
    ///
    /// This methods only works as intended when used with non-empty ranges or
    /// with `Range1d::empty`.
    ///
    constexpr bool intersects(const Range1d& other) const {
        return other.pMin_ <= pMax_
            && pMin_ <= other.pMax_;
    }

    /// Returns whether this range entirely contains the `other` range.
    ///
    /// This methods only works as intended when used with non-empty ranges or
    /// with `Range1d::empty`.
    ///
    constexpr bool contains(const Range1d& other) const {
        return other.pMax_ <= pMax_
            && pMin_ <= other.pMin_;
    }

    /// Returns whether this range contains the given `point`.
    ///
    /// If this range is an empty range, then this method always return false.
    ///
    constexpr bool contains(double point) const {
        return point <= pMax_
            && pMin_ <= point;
    }

private:
    double pMin_;
    double pMax_;
};

inline constexpr Range1d Range1d::empty = Range1d(
    core::infinity<double>, -core::infinity<double>);

/// Alias for `vgc::core::Array<vgc::core::Range1d>`.
///
using Range1dArray = core::Array<Range1d>;

/// Overloads `setZero(T& x)`.
///
/// \sa `vgc::core::zero<T>()`
///
inline void setZero(Range1d& r) {
    r = Range1d();
}

/// Writes the range `r` to the output stream.
///
template<typename OStream>
void write(OStream& out, const Range1d& r) {
    write(out, '(', r.pMin(), ", ", r.pMax(), ')');
}

/// Reads a `Range1d` from the input stream, and stores it in the output
/// parameter `r`. Leading whitespaces are allowed. Raises `ParseError` if the
/// stream does not start with a valid string representation of a `Range1d`.
/// Raises `RangeError` if one of its coordinates is outside the representable
/// range of a double.
///
template <typename IStream>
void readTo(Range1d& r, IStream& in) {
    double pMin, pMax;
    core::skipWhitespaceCharacters(in);
    core::skipExpectedCharacter(in, '(');
    core::readTo(pMin, in);
    core::skipWhitespaceCharacters(in);
    core::skipExpectedCharacter(in, ',');
    core::readTo(pMax, in);
    core::skipWhitespaceCharacters(in);
    core::skipExpectedCharacter(in, ')');
    r = Range1d(pMin, pMax);
}

} // namespace vgc::geometry

// see https://fmt.dev/latest/api.html#formatting-user-defined-types
template <>
struct fmt::formatter<vgc::geometry::Range1d> {
    constexpr auto parse(format_parse_context& ctx) {
        auto it = ctx.begin(), end = ctx.end();
        if (it != end && *it != '}') {
            throw format_error("invalid format");
        }
        return it;
    }
    template <typename FormatContext>
    auto format(const vgc::geometry::Range1d& r, FormatContext& ctx) {
        return format_to(ctx.out(),"({}, {})",
                         r.pMin(), r.pMax());
    }
};

#endif // VGC_GEOMETRY_RANGE1D_H
