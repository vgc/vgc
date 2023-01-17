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

#ifndef VGC_CORE_ANIMTIME_H
#define VGC_CORE_ANIMTIME_H

#include <algorithm> // minmax
#include <functional>

#include <vgc/core/api.h>
#include <vgc/core/arithmetic.h>
#include <vgc/core/array.h>
#include <vgc/core/format.h>

namespace vgc::core {

class AnimTime;

class AnimDuration {
public:
    constexpr AnimDuration() noexcept = default;

    constexpr bool operator==(const AnimDuration& other) const {
        return x_ == other.x_;
    }

    constexpr bool operator!=(const AnimDuration& other) const {
        return !operator==(other);
    }

    constexpr bool operator<(const AnimDuration& other) const {
        return x_ < other.x_;
    }

    constexpr bool operator>(const AnimDuration& other) const {
        return x_ > other.x_;
    }

    constexpr bool operator<=(const AnimDuration& other) const {
        return x_ <= other.x_;
    }

    constexpr bool operator>=(const AnimDuration& other) const {
        return x_ >= other.x_;
    }

private:
    friend AnimTime;
    friend fmt::formatter<vgc::core::AnimDuration>;

    double x_ = 0.0;
};

class AnimTime {
public:
    constexpr AnimTime() noexcept = default;

    constexpr bool operator==(const AnimTime& other) const {
        return x_ == other.x_;
    }

    constexpr bool operator!=(const AnimTime& other) const {
        return !operator==(other);
    }

    constexpr bool operator<(const AnimTime& other) const {
        return x_ < other.x_;
    }

    constexpr bool operator>(const AnimTime& other) const {
        return x_ > other.x_;
    }

    constexpr bool operator<=(const AnimTime& other) const {
        return x_ <= other.x_;
    }

    constexpr bool operator>=(const AnimTime& other) const {
        return x_ >= other.x_;
    }

    constexpr AnimDuration operator-(const AnimTime& other) const {
        AnimDuration ret;
        ret.x_ = x_ - other.x_;
        return ret;
    }

    size_t hash() const {
        return std::hash<double>()(x_);
    }

    constexpr AnimTime operator+(const AnimDuration& duration) const {
        AnimTime ret;
        ret.x_ = x_ + duration.x_;
        return ret;
    }

    constexpr AnimTime operator-(const AnimDuration& duration) const {
        AnimTime ret;
        ret.x_ = x_ - duration.x_;
        return ret;
    }

    constexpr AnimTime& operator+=(const AnimDuration& duration) {
        x_ += duration.x_;
        return *this;
    }

    constexpr AnimTime& operator-=(const AnimDuration& duration) {
        x_ -= duration.x_;
        return *this;
    }

private:
    friend fmt::formatter<vgc::core::AnimTime>;

    double x_ = 0.0;
};

class AnimTimeRange {
public:
    using ScalarType = AnimTime;
    static constexpr Int dimension = 1;

    VGC_WARNING_PUSH
    VGC_WARNING_MSVC_DISABLE(26495) // member variable uninitialized
    /// Creates an uninitialized `AnimTimeRange`.
    ///
    AnimTimeRange(core::NoInit) noexcept {
    }
    VGC_WARNING_POP

    /// Creates a zero-initialized `AnimTimeRange`.
    ///
    /// This is equivalent to `AnimTimeRange(0, 0)`.
    ///
    constexpr AnimTimeRange() noexcept
        : tMin_()
        , tMax_() {
    }

    /// Creates an `AnimTimeRange` defined by the two times `tMin` and `tMax`.
    ///
    /// The range is considered empty if the following condition is true:
    ///
    /// - `tMin > tMax`
    ///
    /// You can ensure that the range isn't empty by calling `normalize()`
    /// after this constructor.
    ///
    constexpr AnimTimeRange(const AnimTime& tMin, const AnimTime& tMax) noexcept
        : tMin_(tMin)
        , tMax_(tMax) {
    }

    /// Creates a `AnimTimeRange` from a `startTime` and `duration`.
    ///
    /// This is equivalent to `AnimTimeRange(startTime, startTime + duration)`.
    ///
    /// If `duration < 0`, then the range is considered empty.
    ///
    /// You can ensure that the range isn't empty by calling `normalize()`
    /// after this function.
    ///
    static constexpr AnimTimeRange
    fromTimeDuration(AnimTime startTime, AnimDuration duration) {

        return AnimTimeRange(startTime, startTime + duration);
    }

    /// An empty `AnimTimeRange`.
    ///
    /// Note that this is not the only possible empty.
    /// However, `AnimTimeRange::empty` is the only empty range that satisfy
    /// `range.unite(empty) == range` for all ranges, and therefore is
    /// typically the most useful empty range.
    ///
    static const AnimTimeRange empty;

    /// Returns whether the range is empty.
    ///
    /// A range is considered empty if and only if `duration() < 0`.
    ///
    /// Equivalently, a range is considered empty if and only if
    /// `tMin() > tMax()`.
    ///
    constexpr bool isEmpty() const {
        return tMin_ > tMax_;
    }

    /// Normalizes in-place the range, that is, makes it non-empty by
    /// swapping its values such that `tMin() <= tMax()`.
    ///
    constexpr AnimTimeRange& normalize() {
        if (tMin_ > tMax_) {
            std::swap(tMin_, tMax_);
        }
        return *this;
    }

    /// Returns a normalized version of this range, that is, a non-empty
    /// version obtained by swapping its coordinates such that `tMin() <=
    /// tMax()`.
    ///
    constexpr AnimTimeRange normalized() {
        std::pair<AnimTime, AnimTime> p = std::minmax(tMin_, tMax_);
        return AnimTimeRange(p.first, p.second);
    }

    /// Returns the `startTime()` of the range.
    ///
    /// This is equivalent to `tMin()`.
    ///
    constexpr AnimTime startTime() const {
        return tMin_;
    }

    /// Updates the `startTime()` of the range, while keeping its duration
    /// constant. This modifies both `tMin()` and `tMax()`.
    ///
    constexpr void setStartTime(AnimTime t) {
        tMax_ += (t - tMin_);
        tMin_ = t;
    }

    /// Returns the duration of the range.
    ///
    /// This is equivalent to `tMax() - tMin()`.
    ///
    constexpr AnimDuration duration() const {
        return tMax_ - tMin_;
    }

    /// Updates the `duration()` of the range, while keeping its `startTime()`
    /// constant. This modifies `tMax()` but not `tMin()`.
    ///
    constexpr void setDuration(const AnimDuration& duration) {
        tMax_ = tMin_ + duration;
    }

    /// Returns the min time of the range.
    ///
    constexpr AnimTime tMin() const {
        return tMin_;
    }

    /// Updates the min time `tMin()` of the range, while keeping the max
    /// value `tMax()` constant. This modifies both `startTime()` and `duration()`.
    ///
    constexpr void setPMin(AnimTime tMin) {
        tMin_ = tMin;
    }

    /// Returns the max time of the range.
    ///
    constexpr AnimTime tMax() const {
        return tMax_;
    }

    /// Updates the max time `tMax()` of the range, while keeping the min
    /// time `tMin()` constant. This modifies `duration()` but not `setStartTime()`.
    ///
    constexpr void setPMax(AnimTime tMax) {
        tMax_ = tMax;
    }

    /// Returns whether the two ranges `r1` and `r2` are equal.
    ///
    friend constexpr bool operator==(const AnimTimeRange& r1, const AnimTimeRange& r2) {
        return r1.tMin_ == r2.tMin_ && r1.tMax_ == r2.tMax_;
    }

    /// Returns whether the two ranges `r1` and `r2` are different.
    ///
    friend constexpr bool operator!=(const AnimTimeRange& r1, const AnimTimeRange& r2) {
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
    //bool
    //isClose(const AnimTimeRange& other, float relTol = 1e-5f, float absTol = 0.0f) const {
    //    return core::isClose(tMin_, other.tMin_, relTol, absTol)
    //           && core::isClose(tMax_, other.tMax_, relTol, absTol);
    //}

    /// Returns whether the euclidean distances between bounds of this
    /// range and the corresponding bounds of the `other` range are
    /// all smaller or equal than the given absolute tolerance.
    ///
    /// If you need a relative tolerance rather than an absolute tolerance, you
    /// should use `isClose()` instead.
    ///
    //bool isNear(const AnimTimeRange& other, float absTol) const {
    //    return core::isNear(tMin_, other.tMin_, absTol)
    //           && core::isNear(tMax_, other.tMax_, absTol);
    //}

    /// Returns the smallest range that contains both this range
    /// and the `other` range.
    ///
    /// ```
    /// AnimTimeRange r1(0, 1);
    /// AnimTimeRange r2(2, 3);
    /// AnimTimeRange r3 = r1.unitedWith(r2);                  // == AnimTimeRange(0, 3)
    /// AnimTimeRange r4 = r1.unitedWith(AnimTimeRange.empty); // == AnimTimeRange(0, 1)
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
    /// AnimTimeRange r1(0, 1);
    /// AnimTimeRange r2(3, 2);
    /// bool b = r2.isEmpty();                // == true
    /// AnimTimeRange r3 = r1.unitedWith(r2); // == AnimTimeRange(0, 2) (!)
    /// ```
    ///
    /// This behavior may be surprising at first, but it is useful for
    /// performance reasons as well as continuity reasons. Indeed, a small
    /// pertubations of the input will never result in a large perturbations of
    /// the output:
    ///
    /// ```
    /// AnimTimeRange r1(0, 1);
    /// AnimTimeRange r2(1.9f, 2);
    /// AnimTimeRange r3(2.0f, 2);
    /// AnimTimeRange r4(2.1f, 2);
    /// bool b2 = r2.isEmpty();               // == false
    /// bool b3 = r3.isEmpty();               // == false
    /// bool b4 = r4.isEmpty();               // == true
    /// AnimTimeRange s2 = r1.unitedWith(r2); // == AnimTimeRange(0, 2)
    /// AnimTimeRange s3 = r1.unitedWith(r3); // == AnimTimeRange(0, 2)
    /// AnimTimeRange s4 = r1.unitedWith(r4); // == AnimTimeRange(0, 2)
    /// ```
    ///
    /// This behavior is intended and will not change in future versions, so
    /// you can rely on it for your algorithms.
    ///
    constexpr AnimTimeRange unitedWith(const AnimTimeRange& other) const {
        return AnimTimeRange(
            (std::min)(tMin_, other.tMin_), (std::max)(tMax_, other.tMax_));
    }

    /// Returns the smallest range that contains both this range
    /// and the given `time`.
    ///
    /// This is equivalent to `unitedWith(AnimTimeRange(time))`.
    ///
    /// See `unitedWith(const AnimTimeRange&)` for more details, in particular about
    /// how it handles empty ranges: uniting an empty range with a
    /// time may result in a range larger than just the time.
    ///
    /// However, uniting `AnimTimeRange::empty` with a time always results in the
    /// range reduced to just the time.
    ///
    constexpr AnimTimeRange unitedWith(const AnimTime& time) const {
        return AnimTimeRange((std::min)(tMin_, time), (std::max)(tMax_, time));
    }

    /// Unites this range in-place with the `other` range.
    ///
    /// See `unitedWith(other)` for more details, in particular about how it
    /// handles empty ranges (uniting with an empty range may increase the size
    /// of this range).
    ///
    constexpr AnimTimeRange& uniteWith(const AnimTimeRange& other) {
        *this = unitedWith(other);
        return *this;
    }

    /// Returns the smallest range that contains both this range
    /// and the given `time`.
    ///
    /// This is equivalent to `uniteWith(AnimTimeRange(time))`.
    ///
    /// See `unitedWith(const AnimTimeRange&)` for more details, in particular about
    /// how it handles empty ranges: uniting an empty range with a
    /// time may result in a range larger than just the time.
    ///
    /// However, uniting `AnimTimeRange::empty` with a time always results in the
    /// range reduced to just the time.
    ///
    constexpr AnimTimeRange& uniteWith(const AnimTime& other) {
        *this = unitedWith(other);
        return *this;
    }

    /// Returns the intersection between this range and the `other`
    /// range.
    ///
    /// ```
    /// AnimTimeRange r1(0, 3);
    /// AnimTimeRange r2(2, 4);
    /// AnimTimeRange r3(5, 6);
    /// AnimTimeRange r4(2, 1); // (empty)
    ///
    /// AnimTimeRange s2 = r1.intersectedWith(r2);            // == AnimTimeRange(2, 3)
    /// AnimTimeRange s3 = r1.intersectedWith(r3);            // == AnimTimeRange(5, 3)  (empty)
    /// AnimTimeRange s4 = r1.intersectedWith(r4);            // == AnimTimeRange(2, 1)  (empty)
    /// AnimTimeRange s5 = r1.intersectedWith(AnimTimeRange.empty); // == AnimTimeRange.empty        (empty)
    /// ```
    ///
    /// This function simply computes the maximum of the min corners and the
    /// minimum of the max corners.
    ///
    /// Unlike `unitedWith()`, this always work as you would expect, even when
    /// intersecting with empty ranges. In particular, the intersection
    /// with an empty range always results in an empty range.
    ///
    constexpr AnimTimeRange intersectedWith(const AnimTimeRange& other) const {
        return AnimTimeRange(
            (std::max)(tMin_, other.tMin_), (std::min)(tMax_, other.tMax_));
    }

    /// Intersects this range in-place with the `other` range.
    ///
    /// See `intersectedWith(other)` for more details.
    ///
    constexpr AnimTimeRange& intersectWith(const AnimTimeRange& other) {
        *this = intersectedWith(other);
        return *this;
    }

    /// Returns whether this range has a non-empty intersection with the
    /// `other` range.
    ///
    /// This methods only works as intended when used with non-empty ranges or
    /// with `AnimTimeRange::empty`.
    ///
    constexpr bool intersects(const AnimTimeRange& other) const {
        return other.tMin_ <= tMax_ && tMin_ <= other.tMax_;
    }

    /// Returns whether this range entirely contains the `other` range.
    ///
    /// This methods only works as intended when used with non-empty ranges or
    /// with `AnimTimeRange::empty`.
    ///
    constexpr bool contains(const AnimTimeRange& other) const {
        return other.tMax_ <= tMax_ && tMin_ <= other.tMin_;
    }

    /// Returns whether this range contains the given `time`.
    ///
    /// If this range is an empty range, then this method always return false.
    ///
    constexpr bool contains(const AnimTime& time) const {
        return time <= tMax_ && tMin_ <= time;
    }

private:
    friend fmt::formatter<vgc::core::AnimTimeRange>;

    AnimTime tMin_;
    AnimTime tMax_;
};

inline constexpr AnimTimeRange AnimTimeRange::empty = AnimTimeRange(
    AnimTime(),
    AnimTime()); //core::infinity<float>, -core::infinity<float>);

/// Alias for `vgc::core::Array<vgc::core::AnimTimeRange>`.
///
using AnimTimeRangeArray = core::Array<AnimTimeRange>;

/// Overloads `setZero(T& x)`.
///
/// \sa `vgc::core::zero<T>()`
///
inline void setZero(AnimTimeRange& r) {
    r = AnimTimeRange();
}

/// Writes the range `r` to the output stream.
///
template<typename OStream>
void write(OStream& out, const AnimTimeRange& r) {
    write(out, '(', r.tMin(), ", ", r.tMax(), ')');
}

} // namespace vgc::core

template<>
struct std::hash<vgc::core::AnimTime> {
    std::size_t operator()(const vgc::core::AnimTime& x) const noexcept {
        return x.hash();
    }
};

template<>
struct fmt::formatter<vgc::core::AnimDuration> {
    constexpr auto parse(format_parse_context& ctx) {
        auto it = ctx.begin(), end = ctx.end();
        if (it != end && *it != '}') {
            throw format_error("invalid format");
        }
        return it;
    }
    template<typename FormatContext>
    auto format(const vgc::core::AnimDuration& d, FormatContext& ctx) {
        return format_to(ctx.out(), "{}", d.x_);
    }
};

template<>
struct fmt::formatter<vgc::core::AnimTime> {
    constexpr auto parse(format_parse_context& ctx) {
        auto it = ctx.begin(), end = ctx.end();
        if (it != end && *it != '}') {
            throw format_error("invalid format");
        }
        return it;
    }
    template<typename FormatContext>
    auto format(const vgc::core::AnimTime& t, FormatContext& ctx) {
        return format_to(ctx.out(), "{}", t.x_);
    }
};

template<>
struct fmt::formatter<vgc::core::AnimTimeRange> {
    constexpr auto parse(format_parse_context& ctx) {
        auto it = ctx.begin(), end = ctx.end();
        if (it != end && *it != '}') {
            throw format_error("invalid format");
        }
        return it;
    }
    template<typename FormatContext>
    auto format(const vgc::core::AnimTimeRange& r, FormatContext& ctx) {
        return format_to(ctx.out(), "({}, {})", r.tMin(), r.tMax());
    }
};

#endif // VGC_CORE_ANIMTIME_H
