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

// This file is used to generate all the variants of this class.
// You must manually run generate.py after any modification.

// clang-format off

#ifndef VGC_GEOMETRY_RECT2X_H
#define VGC_GEOMETRY_RECT2X_H

#include <algorithm> // minmax

#include <vgc/core/array.h>
#include <vgc/geometry/api.h>
#include "vec2x.h"

namespace vgc::geometry {

/// \class vgc::core::Rect2x
/// \brief 2D rectangle using %scalar-type-description%.
///
/// This class represents a 2D rectangle using %scalar-type-description%.
///
/// The rectangle is internally represented as a min corner `pMin()` and a max
/// corner `pMax()`. You can create a rectangle by providing these min/max
/// corners directly:
///
/// -  `Rect2x(const Vec2x& pMin, const Vec2x& pMax)`
/// -  `Rect2x(float xMin, float xMax, float yMin, float yMax)`
///
/// If `xMin > xMax` or `yMin > yMax`, then the rectangle is considered empty
/// (`isEmpty()` will return true), and the `unitedWith()` operation may not
/// work as you may expect, see its documentation for more details.
///
/// Alternatively, you can create a `Rect2x` by providing its `position` and
/// `size`:
///
/// - `Rect2x::fromPositionSize(const Vec2x& position, const Vec2x& size)`
/// - `Rect2x::fromPositionSize(const Vec2x& position, float width, float height)`
/// - `Rect2x::fromPositionSize(float x, float y, const Vec2x& size)`
/// - `Rect2x::fromPositionSize(float x, float y, float width, float height)`
///
/// If `width < 0` or `height < 0`, then the rectangle is considered empty.
///
/// Assuming that the x-axis points right and the y-axis points down, then
/// `position()` represents the top-left corner (= `pMin()`) and `position() +
/// size()` represents the bottom-right corner (= `pMax()`).
///
/// If you want to make sure that a rectangle isn't empty, you can create a
/// `Rect2x` then call `normalize()` or `normalized()`, which swaps the min and
/// max coordinates such that `xMin() <= xMax()` and `yMin() <= yMax()`.
///
/// Note that a rectangle with `xMin() == xMax()` or `yMin() == yMax()` isn't
/// considered empty: it is simply reduced to a segment or a point.
///
// VGC_GEOMETRY_API <- Omitted on purpose.
//                     If needed, manually export individual functions.
class Rect2x {
public:
    using ScalarType = float;
    static constexpr Int dimension = 2;

    /// Creates an uninitialized `Rect2x`.
    ///
    Rect2x(core::NoInit) {
    }

    /// Creates a zero-initialized `Rect2x`.
    ///
    /// This is equivalent to `Rect2x(0, 0, 0, 0)`.
    ///
    constexpr Rect2x()
        : pMin_()
        , pMax_() {
    }

    /// Creates a `Rect2x` defined by the two points `pMin` and `pMax`.
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
    constexpr Rect2x(const Vec2x& pMin, const Vec2x& pMax)
        : pMin_(pMin)
        , pMax_(pMax) {
    }

    /// Creates a `Rect2x` defined by the two points (`xMin`, `yMin`) and
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
    constexpr Rect2x(float xMin, float yMin, float xMax, float yMax)
        : pMin_(xMin, yMin)
        , pMax_(xMax, yMax) {
    }

    /// Creates a `Rect2x` from a `position` and `size`.
    ///
    /// This is equivalent to `Rect2x(position, position + size)`.
    ///
    /// If either `size[0] < 0` or `size[1] < 0`, then the rectangle is
    /// considered empty.
    ///
    /// You can ensure that the rectangle isn't empty by calling `normalize()`
    /// after this function.
    ///
    static constexpr Rect2x fromPositionSize(
        const Vec2x& position, const Vec2x& size) {

        return Rect2x(position, position + size);
    }

    /// Creates a `Rect2x` from a `position`, a `width`, and a `height`.
    ///
    /// This is equivalent to `Rect2x(position, position + Vec2x(width, height))`.
    ///
    /// If either `width < 0` or `height < 0`, then the rectangle is considered
    /// empty.
    ///
    /// You can ensure that the rectangle isn't empty by calling `normalize()`
    /// after this function.
    ///
    static constexpr Rect2x fromPositionSize(
        const Vec2x& position, float width, float height) {

        return Rect2x(position, position + Vec2x(width, height));
    }

    /// Creates a `Rect2x` from a position (`x`, `y`) and `size`.
    ///
    /// This is equivalent to `Rect2x(x, y, x + size[0], y + size[1])`.
    ///
    /// If either `size[0] < 0` or `size[1] < 0`, then the rectangle is
    /// considered empty.
    ///
    /// You can ensure that the rectangle isn't empty by calling `normalize()`
    /// after this function.
    ///
    static constexpr Rect2x fromPositionSize(
        float x, float y, const Vec2x& size) {

        return Rect2x(x, y, x + size[0], y + size[1]);
    }

    /// Creates a `Rect2x` from a position (`x`, `y`), a `width`, and
    /// a `height`.
    ///
    /// This is equivalent to `Rect2x(x, y, x + width, y + height)`.
    ///
    /// If either `width < 0` or `height < 0`, then the rectangle is considered
    /// empty.
    ///
    /// You can ensure that the rectangle isn't empty by calling `normalize()`
    /// after this function.
    ///
    static constexpr Rect2x fromPositionSize(
        float x, float y, float width, float height) {

        return Rect2x(x, y, x + width, y + height);
    }

    /// The empty `Rect2x` defined by `Rect2x(inf, inf, -inf, -inf)` where
    /// `inf = core::infinity<float>`.
    ///
    /// Note that this is not the only possible empty rectangle: for example,
    /// `Rect2x(1, 1, 0, 0)` is also empty. However, `Rect2x::empty` is the
    /// only empty rectangle that satisfy `rect.unite(empty) == rect` for all
    /// rectangles, and therefore is typically the most useful empty rectangle.
    ///
    static const Rect2x empty;

    /// Returns whether the rectangle is empty.
    ///
    /// A rectangle is considered empty if and only if `width() < 0` or
    /// `height() < 0`.
    ///
    /// Equivalently, a rectangle is considered empty if and only if
    /// `xMin() > xMax()` or `yMin() > yMax()`.
    ///
    constexpr bool isEmpty() const {
        return pMin_[0] > pMax_[0] || pMin_[1] > pMax_[1];
    }

    /// Normalizes in-place the rectangle, that is, makes it non-empty by
    /// swapping its coordinates such that `xMin() <= xMax()` and `yMin() <=
    /// yMax()`.
    ///
    constexpr Rect2x& normalize() {
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
    constexpr Rect2x normalized() {
        std::pair<float, float> x = std::minmax(pMin_[0], pMax_[0]);
        std::pair<float, float> y = std::minmax(pMin_[1], pMax_[1]);
        return Rect2x(x.first, y.first, x.second, y.second);
    }

    /// Returns the `position()` of the rectangle.
    ///
    /// This is equivalent to `pMin()`.
    ///
    constexpr Vec2x position() const {
        return pMin_;
    }

    /// Updates the `position()` of the rectangle, while keeping its `size()`
    /// constant. This modifies both `pMin()` and `pMax()`.
    ///
    constexpr void setPosition(const Vec2x& position) {
        pMax_ += (position - pMin_);
        pMin_ = position;
    }

    /// Updates the `position()` of the rectangle, while keeping its `size()`
    /// constant. This modifies both `pMin()` and `pMax()`.
    ///
    constexpr void setPosition(float x, float y) {
        setPosition(Vec2x(x, y));
    }

    /// Returns the x-coordinate of the `position()` of the rectangle.
    ///
    /// This is equivalent to `xMin()`.
    ///
    constexpr float x() const {
        return pMin_[0];
    }

    /// Updates the x-coordinate of the `position()` of the rectangle, while
    /// keeping its `width()` constant. This modifies both `xMin()` and
    /// `xMax()`.
    ///
    constexpr void setX(float x) {
        pMax_[0] += x - pMin_[0];
        pMin_[0] = x;
    }

    /// Returns the y-coordinate of the `position()` of the rectangle.
    ///
    /// This is equivalent to `yMin()`.
    ///
    constexpr float y() const {
        return pMin_[1];
    }

    /// Updates the y-coordinate of the `position()` of the rectangle, while
    /// keeping its `height()` constant. This modifies both `yMin()` and
    /// `yMax()`.
    ///
    constexpr void setY(float y) {
        pMax_[1] += y - pMin_[1];
        pMin_[1] = y;
    }

    /// Returns the size of the rectangle.
    ///
    /// This is equivalent to `pMax() - pMin()`.
    ///
    constexpr Vec2x size() const {
        return pMax_ - pMin_;
    }

    /// Updates the `size()` of the rectangle, while keeping its `position()`
    /// constant. This modifies `pMax()` but not `pMin()`.
    ///
    constexpr void setSize(const Vec2x& size) {
        pMax_ = pMin_ + size;
    }

    /// Updates the `size()` of the rectangle, while keeping its `position()`
    /// constant. This modifies `pMax()` but not `pMin()`.
    ///
    constexpr void setSize(float width, float height) {
        setSize(Vec2x(width, height));
    }

    /// Returns the width of the rectangle, that is `xMax() - xMin()`.
    ///
    constexpr float width() const {
        return pMax_[0] - pMin_[0];
    }

    /// Updates the `width()` of the rectangle, while keeping `x()`
    /// constant. This modifies `xMax()` but not `xMin()`.
    ///
    constexpr void setWidth(float width) {
        pMax_[0] = pMin_[0] + width;
    }

    /// Returns the height of the rectangle, that is `yMax() - yMin()`.
    ///
    constexpr float height() const {
        return pMax_[1] - pMin_[1];
    }

    /// Updates the `height()` of the rectangle, while keeping `y()`
    /// constant. This modifies `yMax()` but not `yMin()`.
    ///
    constexpr void setHeight(float height) {
        pMax_[1] = pMin_[1] + height;
    }

    /// Returns the min corner of the rectangle.
    ///
    constexpr Vec2x pMin() const {
        return pMin_;
    }

    /// Updates the min corner `pMin()` of the rectangle, while keeping the max
    /// corner `pMax()` constant. This modifies both `position()` and `size()`.
    ///
    constexpr void setPMin(const Vec2x& pMin) {
        pMin_ = pMin;
    }

    /// Updates the min corner `pMin()` of the rectangle, while keeping the max
    /// corner `pMax()` constant. This modifies both `position()` and `size()`.
    ///
    constexpr void setPMin(float xMin, float yMin) {
        setPMin(Vec2x(xMin, yMin));
    }

    /// Returns the max corner of the rectangle.
    ///
    constexpr Vec2x pMax() const {
        return pMax_;
    }

    /// Updates the max corner `pMax()` of the rectangle, while keeping the min
    /// corner `pMin()` constant. This modifies `size()` but not `position()`.
    ///
    constexpr void setPMax(const Vec2x& pMax) {
        pMax_ = pMax;
    }

    /// Updates the max corner `pMax()` of the rectangle, while keeping the min
    /// corner `pMin()` constant. This modifies `size()` but not `position()`.
    ///
    constexpr void setPMax(float xMax, float yMax) {
        setPMax(Vec2x(xMax, yMax));
    }

    /// Returns the min x-coordinate of the rectangle.
    ///
    /// Note: this can be larger than `xMax()` if this rectangle is empty.
    ///
    constexpr float xMin() const {
        return pMin_[0];
    }

    /// Updates the min x-coordinate `xMin()` of the rectangle, while keeping
    /// the max x-coordinate `xMax()` constant. This modifies both `x()` and
    /// `width()`.
    ///
    constexpr void setXMin(float xMin) {
        pMin_[0] = xMin;
    }

    /// Returns the max x-coordinate of the rectangle.
    ///
    /// Note: this can be smaller than `xMin()` if this rectangle is empty.
    ///
    constexpr float xMax() const {
        return pMax_[0];
    }

    /// Updates the max x-coordinate `xMax()` of the rectangle, while keeping
    /// the min x-coordinate `xMin()` constant. This modifies `width()` but not
    /// `x()`.
    ///
    constexpr void setXMax(float xMax) {
        pMax_[0] = xMax;
    }

    /// Returns the min y-coordinate of the rectangle.
    ///
    /// Note: this can be larger than `yMax()` if this rectangle is empty.
    ///
    constexpr float yMin() const {
        return pMin_[1];
    }

    /// Updates the min y-coordinate `yMin()` of the rectangle, while keeping
    /// the max y-coordinate `yMax()` constant. This modifies both `y()` and
    /// `height()`.
    ///
    constexpr void setYMin(float yMin) {
        pMin_[1] = yMin;
    }

    /// Returns the max y-coordinate of the rectangle.
    ///
    /// Note: this can be smaller than `yMin()` if this rectangle is empty.
    ///
    constexpr float yMax() const {
        return pMax_[1];
    }

    /// Updates the max y-coordinate `yMax()` of the rectangle, while keeping
    /// the min y-coordinate `yMin()` constant. This modifies `height()` but not
    /// `y()`.
    ///
    constexpr void setYMax(float yMax) {
        pMax_[1] = yMax;
    }

    /// Returns whether the two rectangles `r1` and `r2` are equal.
    ///
    friend constexpr bool operator==(const Rect2x& r1, const Rect2x& r2) {
        return r1.pMin_ == r2.pMin_
            && r1.pMax_ == r2.pMax_;
    }

    /// Returns whether the two rectangles `r1` and `r2` are different.
    ///
    friend constexpr bool operator!=(const Rect2x& r1, const Rect2x& r2) {
        return !(r1 == r2);
    }

    /// Returns whether this rectangle and the given `other` rectangle are
    /// almost equal within some relative tolerance, that is, if all their
    /// respective corners are close to each other in the sense of
    /// `Vec2x::isClose()`.
    ///
    /// If you need an absolute tolerance (which is especially important some of
    /// the rectangle corners could be exactly zero), you should use `isNear()`
    /// instead.
    ///
    bool isClose(const Rect2x& other, float relTol = 1e-5f, float absTol = 0.0f) const {
        return pMin_.isClose(other.pMin_, relTol, absTol)
            && pMax_.isClose(other.pMax_, relTol, absTol);
    }

    /// Returns whether the euclidean distances between the corners of this
    /// rectangle and the corresponding corners of the given `other` rectangle are
    /// all smaller or equal than the given absolute tolerance.
    ///
    /// If you need a per-coordinate comparison rather than using the euclidean
    /// distance, you should use `allNear()` instead.
    ///
    /// If you need a relative tolerance rather than an absolute tolerance, you
    /// should use `isClose()` instead.
    ///
    bool isNear(const Rect2x& other, float absTol) const {
        return pMin_.isNear(other.pMin_, absTol)
            && pMax_.isNear(other.pMax_, absTol);
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
    bool allNear(const Rect2x& other, float absTol) const {
        return pMin_.allNear(other.pMin_, absTol)
            && pMax_.allNear(other.pMax_, absTol);
    }

    /// Returns the smallest rectangle that contains both this rectangle
    /// and the `other` rectangle.
    ///
    /// ```
    /// Rect2x r1(0, 0, 1, 1);
    /// Rect2x r2(2, 2, 3, 3);
    /// Rect2x r3 = r1.unitedWith(r2);           // == Rect2x(0, 0, 3, 3)
    /// Rect2x r4 = r1.unitedWith(Rect2x.empty); // == Rect2x(0, 0, 1, 1)
    /// ```
    ///
    /// Note that this function does not explicitly check whether rectangles
    /// are empty, and simply computes the minimum of the min corners and the
    /// maximum of the max corners.
    ///
    /// Therefore, `r1.unitedWith(r2)` may return a rectangle larger than `r1` even
    /// if `r2` is empty, as demonstrated below:
    ///
    /// ```
    /// Rect2x r1(0, 0, 1, 1);
    /// Rect2x r2(3, 3, 2, 2);
    /// bool b = r2.isEmpty();         // == true
    /// Rect2x r3 = r1.unitedWith(r2); // == Rect2x(0, 0, 2, 2) (!)
    /// ```
    ///
    /// This behavior may be surprising at first, but it is useful for
    /// performance reasons as well as continuity reasons. Indeed, a small
    /// pertubations of the input will never result in a large perturbations of
    /// the output:
    ///
    /// ```
    /// Rect2x r1(0, 0, 1, 1);
    /// Rect2x r2(1.9f, 1.9f, 2, 2);
    /// Rect2x r3(2.0f, 2.0f, 2, 2);
    /// Rect2x r4(2.1f, 2.1f, 2, 2);
    /// bool b2 = r2.isEmpty();        // == false
    /// bool b3 = r3.isEmpty();        // == false
    /// bool b4 = r4.isEmpty();        // == true
    /// Rect2x s2 = r1.unitedWith(r2); // == Rect2x(0, 0, 2, 2)
    /// Rect2x s3 = r1.unitedWith(r3); // == Rect2x(0, 0, 2, 2)
    /// Rect2x s4 = r1.unitedWith(r4); // == Rect2x(0, 0, 2, 2)
    /// ```
    ///
    /// This behavior is intended and will not change in future versions, so
    /// you can rely on it for your algorithms.
    ///
    constexpr Rect2x unitedWith(const Rect2x& other) const {
        return Rect2x(
            (std::min)(pMin_[0], other.pMin_[0]),
            (std::min)(pMin_[1], other.pMin_[1]),
            (std::max)(pMax_[0], other.pMax_[0]),
            (std::max)(pMax_[1], other.pMax_[1]));
    }

    /// Returns the smallest rectangle that contains both this rectangle
    /// and the given `point`.
    ///
    /// This is equivalent to `unitedWith(Rect2x(point, point))`.
    ///
    /// See `unitedWith(const Rect2x&)` for more details, in particular about
    /// how it handles empty rectangles: uniting an empty rectangle with a
    /// point may result in a rectangle larger than just the point.
    ///
    /// However, uniting `Rect2x::empty` with a point always results in the
    /// rectangle reduced to just the point.
    ///
    constexpr Rect2x unitedWith(const Vec2x& point) const {
        return Rect2x(
            (std::min)(pMin_[0], point[0]),
            (std::min)(pMin_[1], point[1]),
            (std::max)(pMax_[0], point[0]),
            (std::max)(pMax_[1], point[1]));
    }

    /// Unites this rectangle in-place with the `other` rectangle.
    ///
    /// See `unitedWith(other)` for more details, in particular about how it
    /// handles empty rectangles (uniting with an empty rectangle may increase the size
    /// of this rectangle).
    ///
    constexpr Rect2x& uniteWith(const Rect2x& other) {
        *this = unitedWith(other);
        return *this;
    }

    /// Returns the smallest rectangle that contains both this rectangle
    /// and the given `point`.
    ///
    /// This is equivalent to `uniteWith(Rect2x(point, point))`.
    ///
    /// See `unitedWith(const Rect2x&)` for more details, in particular about
    /// how it handles empty rectangles: uniting an empty rectangle with a
    /// point may result in a rectangle larger than just the point.
    ///
    /// However, uniting `Rect2x::empty` with a point always results in the
    /// rectangle reduced to just the point.
    ///
    constexpr Rect2x& uniteWith(const Vec2x& other) {
        *this = unitedWith(other);
        return *this;
    }

    /// Returns the intersection between this rectangle and the `other`
    /// rectangle.
    ///
    /// ```
    /// Rect2x r1(0, 0, 3, 3);
    /// Rect2x r2(2, 2, 4, 4);
    /// Rect2x r3(5, 5, 6, 6);
    /// Rect2x r4(2, 2, 1, 1); // (empty)
    ///
    /// Rect2x s2 = r1.intersectedWith(r2);           // == Rect2x(2, 2, 3, 3)
    /// Rect2x s3 = r1.intersectedWith(r3);           // == Rect2x(5, 5, 3, 3)  (empty)
    /// Rect2x s4 = r1.intersectedWith(r4);           // == Rect2x(2, 2, 1, 1)  (empty)
    /// Rect2x s5 = r1.intersectedWith(Rect2x.empty); // == Rect2x.empty        (empty)
    /// ```
    ///
    /// This function simply computes the maximum of the min corners and the
    /// minimum of the max corners.
    ///
    /// Unlike `unitedWith()`, this always work as you would expect, even when
    /// intersecting with empty rectangles. In particular, the intersection
    /// with an empty rectangle always results in an empty rectangle.
    ///
    constexpr Rect2x intersectedWith(const Rect2x& other) const {
        return Rect2x(
            (std::max)(pMin_[0], other.pMin_[0]),
            (std::max)(pMin_[1], other.pMin_[1]),
            (std::min)(pMax_[0], other.pMax_[0]),
            (std::min)(pMax_[1], other.pMax_[1]));
    }

    /// Intersects this rectangle in-place with the `other` rectangle.
    ///
    /// See `intersectedWith(other)` for more details.
    ///
    constexpr Rect2x& intersectWith(const Rect2x& other) {
        *this = intersectedWith(other);
        return *this;
    }

    /// Returns whether this rectangle has a non-empty intersection with the
    /// `other` rectangle.
    ///
    /// This methods only works as intended when used with non-empty rectangles or
    /// with `Rect2x::empty`.
    ///
    constexpr bool intersects(const Rect2x& other) const {
        return other.pMin_[0] <= pMax_[0]
            && other.pMin_[1] <= pMax_[1]
            && pMin_[0] <= other.pMax_[0]
            && pMin_[1] <= other.pMax_[1];
    }

    /// Returns whether this rectangle entirely contains the `other` rectangle.
    ///
    /// This methods only works as intended when used with non-empty rectangles or
    /// with `Rect2x::empty`.
    ///
    constexpr bool contains(const Rect2x& other) const {
        return other.pMax_[0] <= pMax_[0]
            && other.pMax_[1] <= pMax_[1]
            && pMin_[0] <= other.pMin_[0]
            && pMin_[1] <= other.pMin_[1];
    }

    /// Returns whether this rectangle contains the given `point`.
    ///
    /// If this rectangle is an empty rectangle, then this method always return false.
    ///
    constexpr bool contains(const Vec2x& point) const {
        return point[0] <= pMax_[0]
            && point[1] <= pMax_[1]
            && pMin_[0] <= point[0]
            && pMin_[1] <= point[1];
    }

    /// Returns whether this rectangle contains the given point (`x`, `y`).
    ///
    /// If this rectangle is an empty rectangle, then this method always return false.
    ///
    constexpr bool contains(float x, float y) const {
        return contains(Vec2x(x, y));
    }

private:
    Vec2x pMin_;
    Vec2x pMax_;
};

inline constexpr Rect2x Rect2x::empty = Rect2x(
     core::infinity<float>,  core::infinity<float>,
    -core::infinity<float>, -core::infinity<float>);

/// Alias for `vgc::core::Array<vgc::core::Rect2x>`.
///
using Rect2xArray = core::Array<Rect2x>;

/// Overloads `setZero(T& x)`.
///
/// \sa `vgc::core::zero<T>()`
///
inline void setZero(Rect2x& r) {
    r = Rect2x();
}

/// Writes the rectangle `r` to the output stream.
///
template<typename OStream>
void write(OStream& out, const Rect2x& r) {
    write(out, '(',
          r.xMin(), ", ", r.yMin(), ", ",
          r.xMax(), ", ", r.yMax(), ')');
}

/// Reads a `Rect2x` from the input stream, and stores it in the output
/// parameter `r`. Leading whitespaces are allowed. Raises `ParseError` if the
/// stream does not start with a valid string representation of a `Rect2x`.
/// Raises `RangeError` if one of its coordinates is outside the representable
/// range of a float.
///
template <typename IStream>
void readTo(Rect2x& r, IStream& in) {
    float xMin, yMin, xMax, yMax;
    core::skipWhitespaceCharacters(in);
    core::skipExpectedCharacter(in, '(');
    core::readTo(xMin, in);
    core::skipWhitespaceCharacters(in);
    core::skipExpectedCharacter(in, ',');
    core::readTo(yMin, in);
    core::skipWhitespaceCharacters(in);
    core::skipExpectedCharacter(in, ',');
    core::readTo(xMax, in);
    core::skipWhitespaceCharacters(in);
    core::skipExpectedCharacter(in, ',');
    core::readTo(yMax, in);
    core::skipWhitespaceCharacters(in);
    core::skipExpectedCharacter(in, ')');
    r = Rect2x(xMin, yMin, xMax, yMax);
}

} // namespace vgc::geometry

// see https://fmt.dev/latest/api.html#formatting-user-defined-types
template <>
struct fmt::formatter<vgc::geometry::Rect2x> {
    constexpr auto parse(format_parse_context& ctx) {
        auto it = ctx.begin(), end = ctx.end();
        if (it != end && *it != '}') {
            throw format_error("invalid format");
        }
        return it;
    }
    template <typename FormatContext>
    auto format(const vgc::geometry::Rect2x& r, FormatContext& ctx) {
        return format_to(ctx.out(),"({}, {}, {}, {})",
                         r.xMin(), r.yMin(), r.xMax(), r.yMax());
    }
};

#endif // VGC_GEOMETRY_RECT2X_H
