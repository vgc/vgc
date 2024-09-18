// Copyright 2024 The VGC Developers
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

#ifndef VGC_GEOMETRY_SEGMENT2_H
#define VGC_GEOMETRY_SEGMENT2_H

#include <vgc/geometry/api.h>
#include <vgc/geometry/segment.h>
#include <vgc/geometry/vec2.h>

namespace vgc::geometry {

/// \class vgc::geometry::SegmentIntersection2
/// \brief Information about the intersection between two 2D segments.
///
template<typename T>
class SegmentIntersection2 {
public:
    using ScalarType = T;
    static constexpr Int dimension = 2;

    /// Creates an empty intersection.
    ///
    constexpr SegmentIntersection2() noexcept
        : type_(SegmentIntersectionType::Empty) {
    }

    /// Creates a point intersection at the given position and parameters.
    ///
    constexpr SegmentIntersection2(const Vec2<T>& p, T t1, T t2) noexcept
        : p_(p)
        , q_(p)
        , s1_(t1)
        , t1_(t1)
        , s2_(t2)
        , t2_(t2)
        , type_(SegmentIntersectionType::Point) {
    }

    /// Creates a segment intersection at the given positions and parameters.
    ///
    constexpr SegmentIntersection2(
        const Vec2<T>& p,
        const Vec2<T>& q,
        T s1,
        T t1,
        T s2,
        T t2) noexcept

        : p_(p)
        , q_(q)
        , s1_(s1)
        , t1_(t1)
        , s2_(s2)
        , t2_(t2)
        , type_(SegmentIntersectionType::Segment) {
    }

    /// Returns the type of the intersection, that is, whether the intersection
    /// is empty, a point, or a segment.
    ///
    constexpr SegmentIntersectionType type() const {
        return type_;
    }

    /// Returns the "start" position of the intersection.
    ///
    /// If `type()` is `Empty`, this value is undefined.
    ///
    /// If `type()` is `Point`, this is the intersection point, and `p() == q()`.
    ///
    /// If `type()` is `Segment`, this is the start of the shared sub-segment.
    ///
    constexpr const Vec2<T>& p() const {
        return p_;
    }

    /// Returns the "end" position of the intersection.
    ///
    /// If `type()` is `Empty`, this value is undefined.
    ///
    /// If `type()` is `Point`, this is the intersection point, and `p() == q()`.
    ///
    /// If `type()` is `Segment`, this is the end of the shared sub-segment.
    ///
    constexpr const Vec2<T>& q() const {
        return q_;
    }

    /// Returns the parameter `t` along the first segment `(a1, b1)` such that
    /// `p()` is approximately equal to `lerp(a1, b1, t)`.
    ///
    /// If `type()` is `Empty`, this value is undefined.
    ///
    /// If `type()` is `Point`, `s1() == t1()`.
    ///
    constexpr T s1() const {
        return s1_;
    }

    /// Returns the parameter `t` along the first segment `(a1, b1)` such that
    /// `q()` is approximately equal to `lerp(a1, b1, t)`.
    ///
    /// If `type()` is `Empty`, this value is undefined.
    ///
    /// If `type()` is `Point`, `s1() == t1()`.
    ///
    constexpr T t1() const {
        return t1_;
    }

    /// Returns the parameter `t` along the second segment `(a2, b2)` such that
    /// `p()` is approximately equal to `lerp(a2, b2, t)`.
    ///
    /// If `type()` is `Empty`, this value is undefined.
    ///
    /// If `type()` is `Point`, `s2() == t2()`.
    ///
    constexpr T s2() const {
        return s2_;
    }

    /// Returns the parameter `t` along the second segment `(a2, b2)` such that
    /// `q()` is approximately equal to `lerp(a2, b2, t)`.
    ///
    /// If `type()` is `Empty`, this value is undefined.
    ///
    /// If `type()` is `Point`, `s2() == t2()`.
    ///
    constexpr T t2() const {
        return t2_;
    }

    /// Returns whether `i1` and `i2` are equal.
    ///
    friend constexpr bool
    operator==(const SegmentIntersection2& i1, const SegmentIntersection2& i2) {
        if (i1.type_ != i2.type_) {
            return false;
        }
        switch (i1.type_) {
        case SegmentIntersectionType::Empty:
            return true;
        case SegmentIntersectionType::Point:
        case SegmentIntersectionType::Segment:
            return i1.p_ == i2.p_ && i1.q_ == i2.q_ && i1.s1_ == i2.s1_
                   && i1.t1_ == i2.t1_ && i1.s2_ == i2.s2_ && i1.t2_ == i2.t2_;
        }
        return false;
    }

    /// Returns whether `s1` and `s2` are different.
    ///
    friend constexpr bool
    operator!=(const SegmentIntersection2& i1, const SegmentIntersection2& i2) {
        return !(i1 == i2);
    }

private:
    Vec2<T> p_ = core::noInit;
    Vec2<T> q_ = core::noInit;
    T s1_; // no-init
    T t1_; // no-init
    T s2_; // no-init
    T t2_; // no-init
    SegmentIntersectionType type_;
};

/// Alias for `SegmentIntersection2<float>`.
///
using SegmentIntersection2f = SegmentIntersection2<float>;

/// Alias for `SegmentIntersection2<double>`.
///
using SegmentIntersection2d = SegmentIntersection2<double>;

} // namespace vgc::geometry

template<typename T>
struct fmt::formatter<vgc::geometry::SegmentIntersection2<T>> {
    constexpr auto parse(format_parse_context& ctx) {
        auto it = ctx.begin(), end = ctx.end();
        if (it != end && *it != '}') {
            throw format_error("invalid format");
        }
        return it;
    }
    template<typename FormatContext>
    auto format(const vgc::geometry::SegmentIntersection2<T>& i, FormatContext& ctx) {
        switch (i.type()) {
        case vgc::geometry::SegmentIntersectionType::Empty:
            return format_to(ctx.out(), "{{}}");
        case vgc::geometry::SegmentIntersectionType::Point:
            return format_to(ctx.out(), "{{p={}, t1={}, t2={}}}", i.p(), i.t1(), i.t2());
        case vgc::geometry::SegmentIntersectionType::Segment:
            return format_to(
                ctx.out(),
                "{{p={}, q={}, s1={}, t1={}, s2={}, t2={}}}",
                i.p(),
                i.q(),
                i.s1(),
                i.t1(),
                i.s2(),
                i.t2());
        }
        return ctx.out();
    }
};

namespace vgc::geometry {

template<typename T, typename OStream>
void write(OStream& out, const SegmentIntersection2<T>& i) {
    std::string s = core::format("{}", i);
    write(out, s);
}

/// Computes the intersection between the segment [a1, b1] and the segment [a2, b2].
///
/// \sa `Segment2d::intersect()`.
///
template<typename T>
SegmentIntersection2<T> segmentIntersect(
    const Vec2<T>& a1,
    const Vec2<T>& b1,
    const Vec2<T>& a2,
    const Vec2<T>& b2);

// We explicitly instanciate the float and double versions to avoid having to
// include and compile detail/segmentintersect.h in all translation units that
// include segment2.h.
//
extern template VGC_GEOMETRY_API SegmentIntersection2f
segmentIntersect(const Vec2f&, const Vec2f&, const Vec2f&, const Vec2f&);

extern template VGC_GEOMETRY_API SegmentIntersection2d
segmentIntersect(const Vec2d&, const Vec2d&, const Vec2d&, const Vec2d&);

/// \class vgc::geometry::Segment2
/// \brief Represents a 2D line segment.
///
/// This class represents a 2D line segment.
///
/// The segment is internally represented by its start point `a()` and its end
/// point `b()`.
///
template<typename T>
class Segment2 {
public:
    using ScalarType = T;
    static constexpr Int dimension = 2;

    VGC_WARNING_PUSH
    VGC_WARNING_MSVC_DISABLE(26495) // member variable uninitialized
    /// Creates an uninitialized `Segment2`.
    ///
    Segment2(core::NoInit) noexcept
        : data_{Vec2<T>(core::noInit), Vec2<T>(core::noInit)} {
    }
    VGC_WARNING_POP

    /// Creates a zero-initialized `Segment2`.
    ///
    /// This is equivalent to `Segment2(0, 0, 0, 0)`.
    ///
    constexpr Segment2() noexcept
        : data_{} {
    }

    /// Creates a `Segment2` defined by the two points `a` and `b`.
    ///
    constexpr Segment2(const Vec2<T>& a, const Vec2<T>& b) noexcept
        : data_{a, b} {
    }

    /// Creates a `Segment2` defined by the two points (`ax`, `ay`) and
    /// (`bx`, `by`).
    ///
    constexpr Segment2(T ax, T ay, T bx, T by) noexcept
        : data_{Vec2<T>(ax, ay), Vec2<T>(bx, by)} {
    }

    /// Accesses the `i`-th point of this segment, where `i` must be either
    /// `0` or `1`, corresponding respectively to `a()` and `b()`.
    ///
    constexpr const Vec2<T>& operator[](Int i) const {
        return data_[i];
    }

    /// Mutates the `i`-th point of this segment, where `i` must be either
    /// `0` or `1`, corresponding respectively to `a()` and `b()`.
    ///
    constexpr Vec2<T>& operator[](Int i) {
        return data_[i];
    }

    /// Returns the start point of the segment.
    ///
    constexpr const Vec2<T>& a() const {
        return data_[0];
    }

    /// Returns the end point of the segment.
    ///
    constexpr const Vec2<T>& b() const {
        return data_[1];
    }

    /// Modifies the start point of the segment.
    ///
    constexpr void setA(const Vec2<T>& a) {
        data_[0] = a;
    }

    /// Modifies the end point of the segment.
    ///
    constexpr void setB(const Vec2<T>& b) {
        data_[1] = b;
    }

    /// Returns the x-coordinate of the start point of the segment.
    ///
    constexpr T ax() const {
        return data_[0][0];
    }

    /// Returns the y-coordinate of the start point of the segment.
    ///
    constexpr T ay() const {
        return data_[0][1];
    }

    /// Returns the x-coordinate of the end point of the segment.
    ///
    constexpr T bx() const {
        return data_[1][0];
    }

    /// Returns the y-coordinate of the end point of the segment.
    ///
    constexpr T by() const {
        return data_[1][1];
    }

    /// Modifies the x-coordinate of the start point of the segment.
    ///
    constexpr void setAx(T ax) {
        data_[0][0] = ax;
    }

    /// Modifies the y-coordinate of the start point of the segment.
    ///
    constexpr void setAy(T ay) {
        data_[0][1] = ay;
    }

    /// Modifies the x-coordinate of the end point of the segment.
    ///
    constexpr void setBx(T bx) {
        data_[1][0] = bx;
    }

    /// Modifies the y-coordinate of the end point of the segment.
    ///
    constexpr void setBy(T by) {
        data_[1][1] = by;
    }

    /// Returns whether the segment is degenerate, that is, whether it is
    /// reduced to a point.
    ///
    constexpr bool isDegenerate() const {
        return a() == b();
    }

    /// Computes the intersection between this segment and the `other` segment.
    ///
    /// \sa `geometry::segmentIntersect()`.
    ///
    SegmentIntersection2<T> intersect(const Segment2& other) const {
        return segmentIntersect(a(), b(), other.a(), other.b());
    }

    /// Adds in-place the points of `other` to the points of this `Segment2`.
    ///
    constexpr Segment2& operator+=(const Segment2& other) {
        data_[0] += other[0];
        data_[1] += other[1];
        return *this;
    }

    /// Returns the addition of the two segments `s1` and `s2`.
    ///
    friend constexpr Segment2 operator+(const Segment2& s1, const Segment2& s2) {
        return Segment2(s1) += s2;
    }

    /// Returns a copy of this `Segment2` (unary plus operator).
    ///
    constexpr Segment2 operator+() const {
        return *this;
    }

    /// Substracts in-place the points of `other` from the points of this `Segment2`.
    ///
    constexpr Segment2& operator-=(const Segment2& other) {
        data_[0] -= other[0];
        data_[1] -= other[1];
        return *this;
    }

    /// Returns the substraction of `s1` and `s2`.
    ///
    friend constexpr Segment2 operator-(const Segment2& s1, const Segment2& s2) {
        return Segment2(s1) -= s2;
    }

    /// Returns the opposite of this `Segment2` (unary minus operator).
    ///
    constexpr Segment2 operator-() const {
        return Segment2(-data_[0], -data_[1]);
    }

    /// Multiplies in-place all the points in this `Segment2` by
    /// the scalar `s`.
    ///
    constexpr Segment2& operator*=(T s) {
        data_[0] *= s;
        data_[1] *= s;
        return *this;
    }

    /// Returns the multiplication of all the points in this `Segment2` by
    /// the scalar `s`.
    ///
    constexpr Segment2 operator*(T s) const {
        return Segment2(*this) *= s;
    }

    /// Returns the multiplication of the scalar `s` with the all the points
    /// in the segment `t`.
    ///
    friend constexpr Segment2 operator*(T s, const Segment2& segment) {
        return segment * s;
    }

    /// Divides in-place the points of this `Segment2` by the scalar `s`.
    ///
    constexpr Segment2& operator/=(T s) {
        data_[0] /= s;
        data_[1] /= s;
        return *this;
    }

    /// Returns the division of the points in this `Segment2` by the scalar `s`.
    ///
    constexpr Segment2 operator/(T s) const {
        return Segment2(*this) /= s;
    }

    /// Returns whether `s1` and `s2` are equal.
    ///
    friend constexpr bool operator==(const Segment2& s1, const Segment2& s2) {
        return s1.data_[0] == s2.data_[0] //
               && s1.data_[1] == s2.data_[1];
    }

    /// Returns whether `s1` and `s2` are different.
    ///
    friend constexpr bool operator!=(const Segment2& s1, const Segment2& s2) {
        return s1.data_[0] != s2.data_[0] //
               || s1.data_[1] != s2.data_[1];
    }

    /// Compares `s1` and `s2` using lexicographic order on (ax, ay, bx, by),
    /// that is, lexicographic order on (a, b) where points are themselves
    /// compared using lexicographic order on (x, y).
    ///
    /// This order is often useful for plane-sweep algorithms.
    ///
    friend constexpr bool operator<(const Segment2& s1, const Segment2& s2) {
        // clang-format off
        return ( (s1.data_[0] < s2.data_[0]) ||
               (!(s2.data_[0] < s1.data_[0]) && 
               ( (s1.data_[1] < s2.data_[1]))));
        // clang-format on
    }

    /// Compares `s1` and `s2` using lexicographic order on (ax, ay, bx, by).
    ///
    friend constexpr bool operator<=(const Segment2& s1, const Segment2& s2) {
        return !(s2 < s1);
    }

    /// Compares `s1` and `s2` using lexicographic order on (ax, ay, bx, by).
    ///
    friend constexpr bool operator>(const Segment2& s1, const Segment2& s2) {
        return s2 < s1;
    }

    /// Compares `s1` and `s2` using lexicographic order on (ax, ay, bx, by).
    ///
    friend constexpr bool operator>=(const Segment2& s1, const Segment2& s2) {
        return !(s1 < s2);
    }

private:
    Vec2<T> data_[2];
};

/// Alias for `Segment2<float>`.
///
using Segment2f = Segment2<float>;

/// Alias for `Segment2<double>`.
///
using Segment2d = Segment2<double>;

/// Alias for `core::Array<Segment2<T>>`.
///
template<typename T>
using Segment2Array = core::Array<Segment2<T>>;

/// Alias for `core::Array<Segment2f>`.
///
using Segment2fArray = core::Array<Segment2f>;

/// Alias for `core::Array<Segment2d>`.
///
using Segment2dArray = core::Array<Segment2d>;

/// Overloads `setZero(T& x)`.
///
/// \sa `vgc::core::zero<T>()`
///
template<typename T>
inline void setZero(Segment2<T>& s) {
    s = Segment2<T>();
}

/// Writes the given `Segment2` to the output stream.
///
template<typename T, typename OStream>
void write(OStream& out, const Segment2<T>& s) {
    write(out, '(', s[0], ", ", s[1], ')');
}

/// Reads a `Segment2` from the input stream, and stores it in the given output
/// parameter `s`. Leading whitespaces are allowed. Raises `ParseError` if the
/// stream does not start with a `Segment2`. Raises `RangeError` if one of its
/// coordinates is outside the representable range of a double.
///
template<typename T, typename IStream>
void readTo(Segment2<T>& s, IStream& in) {
    skipWhitespacesAndExpectedCharacter(in, '(');
    readTo(s[0], in);
    skipWhitespacesAndExpectedCharacter(in, ',');
    readTo(s[1], in);
    skipWhitespacesAndExpectedCharacter(in, ')');
}

} // namespace vgc::geometry

template<typename T>
struct fmt::formatter<vgc::geometry::Segment2<T>> {
    constexpr auto parse(format_parse_context& ctx) {
        auto it = ctx.begin(), end = ctx.end();
        if (it != end && *it != '}') {
            throw format_error("invalid format");
        }
        return it;
    }
    template<typename FormatContext>
    auto format(const vgc::geometry::Segment2<T>& s, FormatContext& ctx) {
        return format_to(ctx.out(), "({}, {})", s[0], s[1]);
    }
};

#endif // VGC_GEOMETRY_SEGMENT2_H
