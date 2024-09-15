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
// Instead, edit tools/rect2x.h then run tools/generate.py.

#ifndef VGC_GEOMETRY_RECT2_H
#define VGC_GEOMETRY_RECT2_H

#include <algorithm> // minmax

#include <vgc/core/array.h>
#include <vgc/core/templateutil.h>
#include <vgc/geometry/api.h>
#include <vgc/geometry/vec2.h>

namespace vgc::geometry {

/// \class vgc::core::Rect2
/// \brief Represents a 2D axis-aligned rectangle.
///
/// This class represents a 2D axis-aligned rectangle.
///
/// The rectangle is internally represented as a min corner `pMin()` and a max
/// corner `pMax()`. You can create a rectangle by providing these min/max
/// corners directly:
///
/// -  `Rect2(const Vec2& pMin, const Vec2& pMax)`
/// -  `Rect2(T xMin, T xMax, T yMin, T yMax)`
///
/// If `xMin > xMax` or `yMin > yMax`, then the rectangle is considered empty
/// (`isEmpty()` will return true), and the `unitedWith()` operation may not
/// work as you may expect, see its documentation for more details.
///
/// Alternatively, you can create a `Rect2` by providing its `position` and
/// `size`:
///
/// - `Rect2::fromPositionSize(const Vec2& position, const Vec2& size)`
/// - `Rect2::fromPositionSize(const Vec2& position, T width, T height)`
/// - `Rect2::fromPositionSize(T x, T y, const Vec2& size)`
/// - `Rect2::fromPositionSize(T x, T y, T width, T height)`
///
/// If `width < 0` or `height < 0`, then the rectangle is considered empty.
///
/// Assuming that the x-axis points right and the y-axis points down, then
/// `position()` represents the top-left corner (= `pMin()`) and `position() +
/// size()` represents the bottom-right corner (= `pMax()`).
///
/// If you want to make sure that a rectangle isn't empty, you can create a
/// `Rect2` then call `normalize()` or `normalized()`, which swaps the min and
/// max coordinates such that `xMin() <= xMax()` and `yMin() <= yMax()`.
///
/// Note that a rectangle with `xMin() == xMax()` or `yMin() == yMax()` isn't
/// considered empty: it is simply reduced to a segment or a point.
///
template<typename T>
class Rect2 {
public:
    using ScalarType = T;
    static constexpr Int dimension = 2;

    VGC_WARNING_PUSH
    VGC_WARNING_MSVC_DISABLE(26495) // member variable uninitialized
    /// Creates an uninitialized `Rect2`.
    ///
    Rect2(core::NoInit) noexcept {
    }
    VGC_WARNING_POP

    /// Creates a zero-initialized `Rect2`.
    ///
    /// This is equivalent to `Rect2(0, 0, 0, 0)`.
    ///
    constexpr Rect2() noexcept
        : pMin_()
        , pMax_() {
    }

    /// Creates a `Rect2` defined by the two points `pMin` and `pMax`.
    ///
    /// The rectangle is considered empty if any of the following conditions
    /// is true:
    ///
    /// - `pMin[0] > pMax[0]`
    /// - `pMin[1] > pMax[1]`
    ///
    /// You can ensure that the rectangle isn't empty by calling `normalize()`
    /// after this constructor.
    ///
    constexpr Rect2(const Vec2<T>& pMin, const Vec2<T>& pMax) noexcept
        : pMin_(pMin)
        , pMax_(pMax) {
    }

    /// Creates a `Rect2` defined by the two points (`xMin`, `yMin`) and
    /// (`xMax`, `yMax`).
    ///
    /// The rectangle is considered empty if any of the following conditions
    /// is true:
    ///
    /// - `xMin > xMax`
    /// - `yMin > yMax`
    ///
    /// You can ensure that the rectangle isn't empty by calling `normalize()`
    /// after this constructor.
    ///
    constexpr Rect2(T xMin, T yMin, T xMax, T yMax) noexcept
        : pMin_(xMin, yMin)
        , pMax_(xMax, yMax) {
    }

    /// Creates a `Rect2` from a `position` and `size`.
    ///
    /// This is equivalent to `Rect2(position, position + size)`.
    ///
    /// If either `size[0] < 0` or `size[1] < 0`, then the rectangle is
    /// considered empty.
    ///
    /// You can ensure that the rectangle isn't empty by calling `normalize()`
    /// after this function.
    ///
    static constexpr Rect2
    fromPositionSize(const Vec2<T>& position, const Vec2<T>& size) noexcept {
        return Rect2(position, position + size);
    }

    /// Creates a `Rect2` from a `position`, a `width`, and a `height`.
    ///
    /// This is equivalent to `Rect2(position, position + Vec2(width, height))`.
    ///
    /// If either `width < 0` or `height < 0`, then the rectangle is considered
    /// empty.
    ///
    /// You can ensure that the rectangle isn't empty by calling `normalize()`
    /// after this function.
    ///
    static constexpr Rect2
    fromPositionSize(const Vec2<T>& position, T width, T height) noexcept {
        return Rect2(position, position + Vec2<T>(width, height));
    }

    /// Creates a `Rect2` from a position (`x`, `y`) and `size`.
    ///
    /// This is equivalent to `Rect2(x, y, x + size[0], y + size[1])`.
    ///
    /// If either `size[0] < 0` or `size[1] < 0`, then the rectangle is
    /// considered empty.
    ///
    /// You can ensure that the rectangle isn't empty by calling `normalize()`
    /// after this function.
    ///
    static constexpr Rect2 fromPositionSize(T x, T y, const Vec2<T>& size) noexcept {
        return Rect2(x, y, x + size[0], y + size[1]);
    }

    /// Creates a `Rect2` from a position (`x`, `y`), a `width`, and
    /// a `height`.
    ///
    /// This is equivalent to `Rect2(x, y, x + width, y + height)`.
    ///
    /// If either `width < 0` or `height < 0`, then the rectangle is considered
    /// empty.
    ///
    /// You can ensure that the rectangle isn't empty by calling `normalize()`
    /// after this function.
    ///
    static constexpr Rect2 fromPositionSize(T x, T y, T width, T height) noexcept {
        return Rect2(x, y, x + width, y + height);
    }

    /// Computes the bounding box of the given `points`.
    ///
    /// This function can be called for any range type `Range` whose elements
    /// are implicitly convertible to `Vec2`.
    ///
    template<typename Range, VGC_REQUIRES(core::isInputRange<Range>)>
    static constexpr Rect2 computeBoundingBox(const Range& points) {
        Rect2 res = Rect2::empty;
        for (const auto& point : points) {
            res.uniteWith(point);
        }
        return res;
    }

    /// Computes the bounding box of the points obtained by applying the
    /// `getPoint` unary operator to all the elements in the given `range`.
    ///
    template<typename Range, typename UnaryOp, VGC_REQUIRES(core::isInputRange<Range>)>
    static constexpr Rect2 computeBoundingBox(const Range& range, UnaryOp getPoint) {
        Rect2 res = Rect2::empty;
        for (const auto& element : range) {
            res.uniteWith(getPoint(element));
        }
        return res;
    }

    /// The empty `Rect2` defined by `Rect2(inf, inf, -inf, -inf)` where
    /// `inf = core::infinity<T>`.
    ///
    /// Note that this is not the only possible empty rectangle: for example,
    /// `Rect2(1, 1, 0, 0)` is also empty. However, `Rect2::empty` is the
    /// only empty rectangle that satisfy `rect.unite(empty) == rect` for all
    /// rectangles, and therefore is typically the most useful empty rectangle.
    ///
    static const Rect2 empty;

    /// Returns whether the rectangle is empty.
    ///
    /// A rectangle is considered empty if and only if `width() < 0` or
    /// `height() < 0`.
    ///
    /// Equivalently, a rectangle is considered empty if and only if
    /// `xMin() > xMax()` or `yMin() > yMax()`.
    ///
    /// \sa `isDegenerate()`.
    ///
    constexpr bool isEmpty() const {
        return pMin_[0] > pMax_[0] || pMin_[1] > pMax_[1];
    }

    /// Returns whether the rectangle is degenerate, that is, whether it is
    /// either empty, or reduced to a point, or reduced to a line segment.
    ///
    /// More precisely, a rectangle is considered degenerate if and only if
    /// `width() <= 0` or `height() <= 0`.
    ///
    /// Equivalently, a rectangle is considered degenerate if and only if
    /// `xMin() >= xMax()` or `yMin() >= yMax()`.
    ///
    /// \sa `isEmpty()`.
    ///
    constexpr bool isDegenerate() const {
        return pMin_[0] >= pMax_[0] || pMin_[1] >= pMax_[1];
    }

    /// Normalizes in-place the rectangle, that is, makes it non-empty by
    /// swapping its coordinates such that `xMin() <= xMax()` and `yMin() <=
    /// yMax()`.
    ///
    constexpr Rect2& normalize() {
        if (pMin_[0] > pMax_[0]) {
            std::swap(pMin_[0], pMax_[0]);
        }
        if (pMin_[1] > pMax_[1]) {
            std::swap(pMin_[1], pMax_[1]);
        }
        return *this;
    }

    /// Returns a normalized version of this rectangle, that is, a non-empty
    /// version obtained by swapping its coordinates such that `xMin() <=
    /// xMax()` and `yMin() <= yMax()`.
    ///
    constexpr Rect2 normalized() {
        std::pair<T, T> x = std::minmax(pMin_[0], pMax_[0]);
        std::pair<T, T> y = std::minmax(pMin_[1], pMax_[1]);
        return Rect2(x.first, y.first, x.second, y.second);
    }

    /// Returns the `position()` of the rectangle.
    ///
    /// This is equivalent to `pMin()`.
    ///
    constexpr Vec2<T> position() const {
        return pMin_;
    }

    /// Updates the `position()` of the rectangle, while keeping its `size()`
    /// constant. This modifies both `pMin()` and `pMax()`.
    ///
    constexpr void setPosition(const Vec2<T>& position) {
        pMax_ += (position - pMin_);
        pMin_ = position;
    }

    /// Updates the `position()` of the rectangle, while keeping its `size()`
    /// constant. This modifies both `pMin()` and `pMax()`.
    ///
    constexpr void setPosition(T x, T y) {
        setPosition(Vec2(x, y));
    }

    /// Returns the x-coordinate of the `position()` of the rectangle.
    ///
    /// This is equivalent to `xMin()`.
    ///
    constexpr T x() const {
        return pMin_[0];
    }

    /// Updates the x-coordinate of the `position()` of the rectangle, while
    /// keeping its `width()` constant. This modifies both `xMin()` and
    /// `xMax()`.
    ///
    constexpr void setX(T x) {
        pMax_[0] += x - pMin_[0];
        pMin_[0] = x;
    }

    /// Returns the y-coordinate of the `position()` of the rectangle.
    ///
    /// This is equivalent to `yMin()`.
    ///
    constexpr T y() const {
        return pMin_[1];
    }

    /// Updates the y-coordinate of the `position()` of the rectangle, while
    /// keeping its `height()` constant. This modifies both `yMin()` and
    /// `yMax()`.
    ///
    constexpr void setY(T y) {
        pMax_[1] += y - pMin_[1];
        pMin_[1] = y;
    }

    /// Returns the size of the rectangle.
    ///
    /// This is equivalent to `pMax() - pMin()`.
    ///
    constexpr Vec2<T> size() const {
        return pMax_ - pMin_;
    }

    /// Updates the `size()` of the rectangle, while keeping its `position()`
    /// constant. This modifies `pMax()` but not `pMin()`.
    ///
    constexpr void setSize(const Vec2<T>& size) {
        pMax_ = pMin_ + size;
    }

    /// Updates the `size()` of the rectangle, while keeping its `position()`
    /// constant. This modifies `pMax()` but not `pMin()`.
    ///
    constexpr void setSize(T width, T height) {
        setSize(Vec2<T>(width, height));
    }

    /// Returns the width of the rectangle, that is `xMax() - xMin()`.
    ///
    constexpr T width() const {
        return pMax_[0] - pMin_[0];
    }

    /// Updates the `width()` of the rectangle, while keeping `x()`
    /// constant. This modifies `xMax()` but not `xMin()`.
    ///
    constexpr void setWidth(T width) {
        pMax_[0] = pMin_[0] + width;
    }

    /// Returns the height of the rectangle, that is `yMax() - yMin()`.
    ///
    constexpr T height() const {
        return pMax_[1] - pMin_[1];
    }

    /// Updates the `height()` of the rectangle, while keeping `y()`
    /// constant. This modifies `yMax()` but not `yMin()`.
    ///
    constexpr void setHeight(T height) {
        pMax_[1] = pMin_[1] + height;
    }

    /// Returns the min corner of the rectangle.
    ///
    constexpr Vec2<T> pMin() const {
        return pMin_;
    }

    /// Updates the min corner `pMin()` of the rectangle, while keeping the max
    /// corner `pMax()` constant. This modifies both `position()` and `size()`.
    ///
    constexpr void setPMin(const Vec2<T>& pMin) {
        pMin_ = pMin;
    }

    /// Updates the min corner `pMin()` of the rectangle, while keeping the max
    /// corner `pMax()` constant. This modifies both `position()` and `size()`.
    ///
    constexpr void setPMin(T xMin, T yMin) {
        setPMin(Vec2<T>(xMin, yMin));
    }

    /// Returns the max corner of the rectangle.
    ///
    constexpr Vec2<T> pMax() const {
        return pMax_;
    }

    /// Updates the max corner `pMax()` of the rectangle, while keeping the min
    /// corner `pMin()` constant. This modifies `size()` but not `position()`.
    ///
    constexpr void setPMax(const Vec2<T>& pMax) {
        pMax_ = pMax;
    }

    /// Updates the max corner `pMax()` of the rectangle, while keeping the min
    /// corner `pMin()` constant. This modifies `size()` but not `position()`.
    ///
    constexpr void setPMax(T xMax, T yMax) {
        setPMax(Vec2<T>(xMax, yMax));
    }

    /// Returns the min x-coordinate of the rectangle.
    ///
    /// Note: this can be larger than `xMax()` if this rectangle is empty.
    ///
    constexpr T xMin() const {
        return pMin_[0];
    }

    /// Updates the min x-coordinate `xMin()` of the rectangle, while keeping
    /// the max x-coordinate `xMax()` constant. This modifies both `x()` and
    /// `width()`.
    ///
    constexpr void setXMin(T xMin) {
        pMin_[0] = xMin;
    }

    /// Returns the max x-coordinate of the rectangle.
    ///
    /// Note: this can be smaller than `xMin()` if this rectangle is empty.
    ///
    constexpr T xMax() const {
        return pMax_[0];
    }

    /// Updates the max x-coordinate `xMax()` of the rectangle, while keeping
    /// the min x-coordinate `xMin()` constant. This modifies `width()` but not
    /// `x()`.
    ///
    constexpr void setXMax(T xMax) {
        pMax_[0] = xMax;
    }

    /// Returns the min y-coordinate of the rectangle.
    ///
    /// Note: this can be larger than `yMax()` if this rectangle is empty.
    ///
    constexpr T yMin() const {
        return pMin_[1];
    }

    /// Updates the min y-coordinate `yMin()` of the rectangle, while keeping
    /// the max y-coordinate `yMax()` constant. This modifies both `y()` and
    /// `height()`.
    ///
    constexpr void setYMin(T yMin) {
        pMin_[1] = yMin;
    }

    /// Returns the max y-coordinate of the rectangle.
    ///
    /// Note: this can be smaller than `yMin()` if this rectangle is empty.
    ///
    constexpr T yMax() const {
        return pMax_[1];
    }

    /// Updates the max y-coordinate `yMax()` of the rectangle, while keeping
    /// the min y-coordinate `yMin()` constant. This modifies `height()` but not
    /// `y()`.
    ///
    constexpr void setYMax(T yMax) {
        pMax_[1] = yMax;
    }

    /// Returns the position of one of the four corners of the rectangle.
    ///
    /// The two given indices must be either `0` or `1`, where `0` means "min"
    /// and `1` means "max".
    ///
    /// ```cpp
    /// Rect2d rect;
    /// Vec2d topLeft = rect.corner(0, 0);
    /// Vec2d topRight = rect.corner(1, 0);
    /// Vec2d bottomRight = rect.corner(1, 1);
    /// Vec2d bottomLeft = rect.corner(0, 1);
    /// ```
    ///
    /// In the example above, the names given to the variables assume a
    /// top-left coordinate system, that is, with the X-axis pointing right and
    /// the Y-axis pointing down.
    ///
    constexpr Vec2<T> corner(Int xIndex, Int yIndex) const {
        return Vec2<T>(xIndex ? xMax() : xMin(), yIndex ? yMax() : yMin());
    }

    /// Returns the position of one of the four corners of the rectangle.
    ///
    /// The given index must be either `0`, `1`, `2`, or `3`.
    ///
    /// ```cpp
    /// Rect2d rect;
    /// Vec2d topLeft = rect.corner(0);
    /// Vec2d topRight = rect.corner(1);
    /// Vec2d bottomRight = rect.corner(2);
    /// Vec2d bottomLeft = rect.corner(3);
    /// ```
    ///
    /// In the example above, the names given to the variables assume a
    /// top-left coordinate system, that is, with the X-axis pointing right and
    /// the Y-axis pointing down.
    ///
    constexpr Vec2<T> corner(Int index) const {
        switch (index) {
        case 0:
            return Vec2<T>(xMin(), yMin());
        case 1:
            return Vec2<T>(xMax(), yMin());
        case 2:
            return Vec2<T>(xMax(), yMax());
        default:
            return Vec2<T>(xMin(), yMax());
        }
    }

    /// Returns whether the two rectangles `r1` and `r2` are equal.
    ///
    friend constexpr bool operator==(const Rect2& r1, const Rect2& r2) {
        return r1.pMin_ == r2.pMin_ && r1.pMax_ == r2.pMax_;
    }

    /// Returns whether the two rectangles `r1` and `r2` are different.
    ///
    friend constexpr bool operator!=(const Rect2& r1, const Rect2& r2) {
        return !(r1 == r2);
    }

    /// Returns whether this rectangle and the given `other` rectangle are
    /// almost equal within some relative tolerance, that is, if all their
    /// respective corners are close to each other in the sense of
    /// `Vec2::isClose()`.
    ///
    /// If you need an absolute tolerance (which is especially important some of
    /// the rectangle corners could be exactly zero), you should use `isNear()`
    /// instead.
    ///
    bool isClose(
        const Rect2& other,
        T relTol = core::defaultRelativeTolerance<T>,
        T absTol = 0.0) const {

        return pMin_.isClose(other.pMin_, relTol, absTol)
               && pMax_.isClose(other.pMax_, relTol, absTol);
    }

    /// Returns whether the euclidean distances between the corners of this
    /// rectangle and the corresponding corners of the given `other` rectangle
    /// are all smaller or equal than the given absolute tolerance.
    ///
    /// If you need a per-coordinate comparison rather than using the euclidean
    /// distance, you should use `allNear()` instead.
    ///
    /// If you need a relative tolerance rather than an absolute tolerance, you
    /// should use `isClose()` instead.
    ///
    bool isNear(const Rect2& other, T absTol) const {
        return pMin_.isNear(other.pMin_, absTol) && pMax_.isNear(other.pMax_, absTol);
    }

    /// Returns whether all coordinates in this rectangle are within some
    /// absolute tolerance of their corresponding coordinate in the given
    /// `other` rectangle.
    ///
    /// This is similar to `isNear(other)`, but completely decorellates the X
    /// and Y coordinates, which is faster to compute but does not reflect true
    /// euclidean distance.
    ///
    /// If you need a relative tolerance rather than an absolute tolerance, you
    /// should use `isClose()` instead.
    ///
    bool allNear(const Rect2& other, T absTol) const {
        return pMin_.allNear(other.pMin_, absTol) && pMax_.allNear(other.pMax_, absTol);
    }

    /// Returns the result of clamping the given point `p` to this rectangle,
    /// that is, clamping:
    /// 1. `p.x()` to [`xMin()`, `xMax()`], and
    /// 2. `p.y()` to [`yMin()`, `yMax()`].
    ///
    /// If this rectangle is empty, then a warning is issued and `p` is clamped
    /// to the `normalized()` rectangle instead.
    ///
    Vec2<T> clamp(const Vec2<T>& p) const {
        return {core::clamp(p.x(), xMin(), xMax()), core::clamp(p.y(), yMin(), yMax())};
    }

    /// Returns the result of clamping both `other.pMin()` and `other.pMax()`
    /// to this rectangle.
    ///
    Rect2 clamp(const Rect2& other) const {
        return Rect2(clamp(other.pMin()), clamp(other.pMax()));
    }

    /// Returns the smallest rectangle that contains both this rectangle
    /// and the `other` rectangle.
    ///
    /// ```
    /// Rect2d r1(0, 0, 1, 1);
    /// Rect2d r2(2, 2, 3, 3);
    /// Rect2d r3 = r1.unitedWith(r2);           // == Rect2d(0, 0, 3, 3)
    /// Rect2d r4 = r1.unitedWith(Rect2d.empty); // == Rect2d(0, 0, 1, 1)
    /// ```
    ///
    /// Note that this function does not explicitly check whether rectangles
    /// are empty, and simply computes the minimum of the min corners and the
    /// maximum of the max corners.
    ///
    /// Therefore, `r1.unitedWith(r2)` may return a rectangle larger than `r1`
    /// even if `r2` is empty, as demonstrated below:
    ///
    /// ```
    /// Rect2d r1(0, 0, 1, 1);
    /// Rect2d r2(3, 3, 2, 2);
    /// bool b = r2.isEmpty();         // == true
    /// Rect2d r3 = r1.unitedWith(r2); // == Rect2d(0, 0, 2, 2) (!)
    /// ```
    ///
    /// This behavior may be surprising at first, but it is useful for
    /// performance reasons as well as continuity reasons. Indeed, a small
    /// pertubation of the input will never result in a large perturbation of
    /// the output:
    ///
    /// ```
    /// Rect2d r1(0, 0, 1, 1);
    /// Rect2d r2(1.9, 1.9, 2, 2);
    /// Rect2d r3(2.0, 2.0, 2, 2);
    /// Rect2d r4(2.1, 2.1, 2, 2);
    /// bool b2 = r2.isEmpty();        // == false
    /// bool b3 = r3.isEmpty();        // == false
    /// bool b4 = r4.isEmpty();        // == true
    /// Rect2d s2 = r1.unitedWith(r2); // == Rect2d(0, 0, 2, 2)
    /// Rect2d s3 = r1.unitedWith(r3); // == Rect2d(0, 0, 2, 2)
    /// Rect2d s4 = r1.unitedWith(r4); // == Rect2d(0, 0, 2, 2)
    /// ```
    ///
    /// This behavior is intended and will not change in future versions, so
    /// you can rely on it for your algorithms.
    ///
    constexpr Rect2 unitedWith(const Rect2& other) const {
        return Rect2(
            (std::min)(pMin_[0], other.pMin_[0]),
            (std::min)(pMin_[1], other.pMin_[1]),
            (std::max)(pMax_[0], other.pMax_[0]),
            (std::max)(pMax_[1], other.pMax_[1]));
    }

    /// Returns the smallest rectangle that contains both this rectangle
    /// and the given `point`.
    ///
    /// This is equivalent to `unitedWith(Rect2(point, point))`.
    ///
    /// See `unitedWith(const Rect2&)` for more details, in particular about
    /// how it handles empty rectangles: uniting an empty rectangle with a
    /// point may result in a rectangle larger than just the point.
    ///
    /// However, uniting `Rect2::empty` with a point always results in the
    /// rectangle reduced to just the point.
    ///
    constexpr Rect2 unitedWith(const Vec2<T>& point) const {
        return Rect2(
            (std::min)(pMin_[0], point[0]),
            (std::min)(pMin_[1], point[1]),
            (std::max)(pMax_[0], point[0]),
            (std::max)(pMax_[1], point[1]));
    }

    /// Unites this rectangle in-place with the `other` rectangle.
    ///
    /// See `unitedWith(other)` for more details, in particular about how it
    /// handles empty rectangles (uniting with an empty rectangle may increase
    /// the size of this rectangle).
    ///
    constexpr Rect2& uniteWith(const Rect2& other) {
        *this = unitedWith(other);
        return *this;
    }

    /// Returns the smallest rectangle that contains both this rectangle
    /// and the given `point`.
    ///
    /// This is equivalent to `uniteWith(Rect2(point, point))`.
    ///
    /// See `unitedWith(const Rect2&)` for more details, in particular about
    /// how it handles empty rectangles: uniting an empty rectangle with a
    /// point may result in a rectangle larger than just the point.
    ///
    /// However, uniting `Rect2::empty` with a point always results in the
    /// rectangle reduced to just the point.
    ///
    constexpr Rect2& uniteWith(const Vec2<T>& other) {
        *this = unitedWith(other);
        return *this;
    }

    /// Returns the intersection between this rectangle and the `other`
    /// rectangle.
    ///
    /// ```
    /// Rect2d r1(0, 0, 3, 3);
    /// Rect2d r2(2, 2, 4, 4);
    /// Rect2d r3(5, 5, 6, 6);
    /// Rect2d r4(2, 2, 1, 1); // (empty)
    ///
    /// Rect2d s2 = r1.intersectedWith(r2);           // == Rect2d(2, 2, 3, 3)
    /// Rect2d s3 = r1.intersectedWith(r3);           // == Rect2d(5, 5, 3, 3)  (empty)
    /// Rect2d s4 = r1.intersectedWith(r4);           // == Rect2d(2, 2, 1, 1)  (empty)
    /// Rect2d s5 = r1.intersectedWith(Rect2d.empty); // == Rect2d.empty        (empty)
    /// ```
    ///
    /// This function simply computes the maximum of the min corners and the
    /// minimum of the max corners.
    ///
    /// Unlike `unitedWith()`, this always work as you would expect, even when
    /// intersecting with empty rectangles. In particular, the intersection
    /// with an empty rectangle always results in an empty rectangle.
    ///
    constexpr Rect2 intersectedWith(const Rect2& other) const {
        return Rect2(
            (std::max)(pMin_[0], other.pMin_[0]),
            (std::max)(pMin_[1], other.pMin_[1]),
            (std::min)(pMax_[0], other.pMax_[0]),
            (std::min)(pMax_[1], other.pMax_[1]));
    }

    /// Intersects this rectangle in-place with the `other` rectangle.
    ///
    /// See `intersectedWith(other)` for more details.
    ///
    constexpr Rect2& intersectWith(const Rect2& other) {
        *this = intersectedWith(other);
        return *this;
    }

    /// Returns whether this rectangle has a non-empty intersection with the
    /// `other` rectangle.
    ///
    /// This methods only works as intended when used with non-empty rectangles or
    /// with `Rect2::empty`.
    ///
    constexpr bool intersects(const Rect2& other) const {
        return other.pMin_[0] <= pMax_[0]    //
               && other.pMin_[1] <= pMax_[1] //
               && pMin_[0] <= other.pMax_[0] //
               && pMin_[1] <= other.pMax_[1];
    }

    /// Returns whether this rectangle entirely contains the `other` rectangle.
    ///
    /// This methods only works as intended when used with non-empty rectangles or
    /// with `Rect2::empty`.
    ///
    constexpr bool contains(const Rect2& other) const {
        return other.pMax_[0] <= pMax_[0]    //
               && other.pMax_[1] <= pMax_[1] //
               && pMin_[0] <= other.pMin_[0] //
               && pMin_[1] <= other.pMin_[1];
    }

    /// Returns whether this rectangle contains the given `point`.
    ///
    /// If this rectangle is an empty rectangle, then this method always return false.
    ///
    constexpr bool contains(const Vec2<T>& point) const {
        return point[0] <= pMax_[0]    //
               && point[1] <= pMax_[1] //
               && pMin_[0] <= point[0] //
               && pMin_[1] <= point[1];
    }

    /// Returns whether this rectangle contains the given point (`x`, `y`).
    ///
    /// If this rectangle is an empty rectangle, then this method always return false.
    ///
    constexpr bool contains(T x, T y) const {
        return contains(Vec2<T>(x, y));
    }

    /// Returns whether this rectangle has a non-empty intersection with the
    /// polyline defined by the sequence of points from `first` to `last`.
    ///
    template<typename PointIt, typename PointPositionGetter = core::Identity>
    bool intersectsPolyline(
        PointIt first,
        PointIt last,
        PointPositionGetter positionGetter = {}) const {

        auto it0 = first;
        Vec2 p0 = positionGetter(*it0);
        int p0c = p0.x() > xMax() ? 2 : (p0.x() < xMin() ? 1 : 0);
        int p0r = p0.y() > yMax() ? 2 : (p0.y() < yMin() ? 1 : 0);
        if (!p0c && !p0r) {
            // p0 is inside the rect.
            return true;
        }

        for (auto it1 = it0 + 1; it1 != last; it0 = it1++) {
            // in segment outline-mode-selection box?
            Vec2 p1 = positionGetter(*it1);
            int p1c = p1.x() > xMax() ? 2 : (p1.x() < xMin() ? 1 : 0);
            int p1r = p1.y() > yMax() ? 2 : (p1.y() < yMin() ? 1 : 0);
            if (!p1c && !p1r) {
                // p1 is inside the rect.
                return true;
            }

            if (intersectsSegmentWithExternalEndpoints_(p0, p1, p0c, p0r, p1c, p1r)) {
                return true;
            }

            p0 = p1;
            p0c = p1c;
            p0r = p1r;
        }

        return false;
    }

    /// Returns whether this rectangle has a non-empty intersection with the
    /// segment defined by the given endpoints `p0` and `p1`.
    ///
    bool intersectsSegment(const Vec2<T>& p0, const Vec2<T>& p1) const {

        int p0c = p0.x() > xMax() ? 2 : (p0.x() < xMin() ? 1 : 0);
        int p0r = p0.y() > yMax() ? 2 : (p0.y() < yMin() ? 1 : 0);
        if (!p0c && !p0r) {
            // p0 is inside the rect.
            return true;
        }
        int p1c = p1.x() > xMax() ? 2 : (p1.x() < xMin() ? 1 : 0);
        int p1r = p1.y() > yMax() ? 2 : (p1.y() < yMin() ? 1 : 0);
        if (!p1c && !p1r) {
            // p1 is inside the rect.
            return true;
        }

        if (intersectsSegmentWithExternalEndpoints_(p0, p1, p0c, p0r, p1c, p1r)) {
            return true;
        }

        return false;
    }

private:
    Vec2<T> pMin_;
    Vec2<T> pMax_;

    bool intersectsSegmentWithExternalEndpoints_(
        const Vec2<T>& p0,
        const Vec2<T>& p1,
        int p0c,
        int p0r,
        int p1c,
        int p1r) const {

        if (p0c == p1c) {
            if (p0c != 0) {
                // p0 and p1 are both on the same side of rect in x.
                //
                //        ┆     ┆
                //   p0  q1────q2     p0
                //    │   │     │     /
                //   p1  q4────q3    /
                //        ┆     ┆  p1
                //
                return false;
            }
            else if (p0r != p1r) {
                // p0 and p1 are on both sides of the rect in y and
                // inside the rect bounds in x.
                //
                //       ┆  p0   ┆
                //      q1───┼──q2
                //       │   │   │
                //      q4───┼──q3
                //       ┆  p1   ┆
                //
                return true;
            }
        }
        if (p0r == p1r) {
            if (p0r != 0) {
                // p0 and p1 are both on the same side of rect in y.
                //
                //     p0──────p1
                //  ┄┄┄┄┄┄┄q1────q2┄┄┄┄┄┄┄
                //          │     │
                //  ┄┄┄┄┄┄┄q4────q3┄┄┄┄┄┄┄
                //            p0──────p1
                //
                return false;
            }
            else if (p0c != p1c) {
                // p0 and p1 are on both sides of the rect in x and
                // inside the rect bounds in y.
                //
                //  ┄┄┄┄┄┄┄q1────q2┄┄┄┄┄┄┄
                //     p0───┼─────┼───p1
                //          │     │
                //  ┄┄┄┄┄┄┄q4────q3┄┄┄┄┄┄┄
                //
                return true;
            }
        }
        if (p0 == p1) {
            // p0 and p1 are equal and outside of the rect
            return false;
        }
        // Here, the remaining cases are limited to (symmetries excluded):
        //
        //  p0 ┆    ┆       p0 ┆    ┆          ┆ p0 ┆
        //  ┄┄┄a────c┄┄┄    ┄┄┄a────c┄┄┄    ┄┄┄a────c┄┄┄
        //     │    │ p1       │    │          │    │ p1
        //  ┄┄┄b────d┄┄┄    ┄┄┄b────d┄┄┄    ┄┄┄b────d┄┄┄
        //     ┆    ┆          ┆    ┆ p1       ┆    ┆
        //
        // We can see that in every case, p0p1 intersects the rect if and only if
        // any corner is on p0p1 or corners are not all on the same side of p0p1.
        //
        Vec2<T> p0p1 = p1 - p0;
        auto computeAngleOrientation = [](const Vec2<T>& ab, const Vec2<T>& ac) {
            double det = ab.det(ac); // TODO: use more robust detSign()
            if (det == 0) {
                return 0x1;
            }
            return (det > 0) ? 0x4 : 0x2;
        };
        int o1 = computeAngleOrientation(p0p1, corner(0) - p0);
        int o2 = computeAngleOrientation(p0p1, corner(1) - p0);
        int o3 = computeAngleOrientation(p0p1, corner(2) - p0);
        int o4 = computeAngleOrientation(p0p1, corner(3) - p0);

        int ox = o1 | o2 | o3 | o4;
        if (ox == 6) {
            // some corners are on different sides of p0p1.
            return true;
        }
        else if (ox & 1) {
            // a corner is on p0p1.
            return true;
        }

        return false;
    }
};

// This definition must be out-of-class.
// See: https://stackoverflow.com/questions/11928089/
// static-constexpr-member-of-same-type-as-class-being-defined
//
template<typename T>
inline constexpr Rect2<T> Rect2<T>::empty = //
    Rect2(core::infinity<T>, core::infinity<T>, -core::infinity<T>, -core::infinity<T>);

/// Alias for `Rect2<float>`.
///
using Rect2f = Rect2<float>;

/// Alias for `Rect2<double>`.
///
using Rect2d = Rect2<double>;

/// Alias for `core::Array<Rect2<T>>`.
///
template<typename T>
using Rect2Array = core::Array<Rect2<T>>;

/// Alias for `core::Array<Rect2f>`.
///
using Rect2fArray = core::Array<Rect2f>;

/// Alias for `core::Array<Rect2d>`.
///
using Rect2dArray = core::Array<Rect2d>;

/// Overloads `setZero(T& x)`.
///
/// \sa `core::zero<T>()`
///
template<typename T>
inline void setZero(Rect2<T>& r) {
    r = Rect2<T>();
}

/// Writes the rectangle `r` to the output stream.
///
template<typename T, typename OStream>
void write(OStream& out, const Rect2<T>& r) {
    write(out, '(', r.xMin(), ", ", r.yMin(), ", ", r.xMax(), ", ", r.yMax(), ')');
}

/// Reads a `Rect2` from the input stream, and stores it in the output
/// parameter `r`. Leading whitespaces are allowed. Raises `ParseError` if the
/// stream does not start with a valid string representation of a `Rect2`.
/// Raises `RangeError` if one of its coordinates is outside the representable
/// range of a `T`.
///
template<typename T, typename IStream>
void readTo(Rect2<T>& r, IStream& in) {
    T xMin, yMin, xMax, yMax;
    core::skipWhitespacesAndExpectedCharacter(in, '(');
    core::readTo(xMin, in);
    core::skipWhitespacesAndExpectedCharacter(in, ',');
    core::readTo(yMin, in);
    core::skipWhitespacesAndExpectedCharacter(in, ',');
    core::readTo(xMax, in);
    core::skipWhitespacesAndExpectedCharacter(in, ',');
    core::readTo(yMax, in);
    core::skipWhitespacesAndExpectedCharacter(in, ')');
    r = Rect2<T>(xMin, yMin, xMax, yMax);
}

} // namespace vgc::geometry

template<typename T>
struct fmt::formatter<vgc::geometry::Rect2<T>> {
    constexpr auto parse(format_parse_context& ctx) {
        auto it = ctx.begin(), end = ctx.end();
        if (it != end && *it != '}') {
            throw format_error("invalid format");
        }
        return it;
    }
    template<typename FormatContext>
    auto format(const vgc::geometry::Rect2<T>& r, FormatContext& ctx) {
        return format_to(
            ctx.out(), "({}, {}, {}, {})", r.xMin(), r.yMin(), r.xMax(), r.yMax());
    }
};

#endif // VGC_GEOMETRY_RECT2_H
