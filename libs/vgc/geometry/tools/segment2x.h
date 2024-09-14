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

// This file is used to generate all the variants of this class.
// You must manually run generate.py after any modification.

// clang-format off

#ifndef VGC_GEOMETRY_SEGMENT2X_H
#define VGC_GEOMETRY_SEGMENT2X_H

#include <vgc/geometry/api.h>
#include <vgc/geometry/segment.h>
#include <vgc/geometry/vec2.h>

namespace vgc::geometry {

/// \class vgc::geometry::Segment2xIntersection
/// \brief Stores information about the intersection between two Segment2x.
///
class VGC_GEOMETRY_API Segment2xIntersection {
public:
    /// Creates an empty intersection.
    ///  
    Segment2xIntersection()
        : type_(SegmentIntersectionType::Empty) {
    }

    /// Creates a point intersection at the given position and parameters.
    ///  
    Segment2xIntersection(const Vec2x& p, float t1, float t2)
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
    Segment2xIntersection(
        const Vec2x& p,
        const Vec2x& q,
        float s1,
        float t1,
        float s2,
        float t2)

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
    SegmentIntersectionType type() const {
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
    const Vec2x& p() const {
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
    const Vec2x& q() const {
        return q_;
    }

    /// Returns the parameter `t` along the first segment `(a1, b1)` such that
    /// `p()` is approximately equal to `lerp(a1, b1, t)`.
    ///
    /// If `type()` is `Empty`, this value is undefined.
    ///
    /// If `type()` is `Point`, `s1() == t1()`.
    ///
    float s1() const {
        return s1_;
    }

    /// Returns the parameter `t` along the first segment `(a1, b1)` such that
    /// `q()` is approximately equal to `lerp(a1, b1, t)`.
    ///
    /// If `type()` is `Empty`, this value is undefined.
    ///
    /// If `type()` is `Point`, `s1() == t1()`.
    ///
    float t1() const {
        return t1_;
    }

    /// Returns the parameter `t` along the second segment `(a2, b2)` such that
    /// `p()` is approximately equal to `lerp(a2, b2, t)`.
    ///
    /// If `type()` is `Empty`, this value is undefined.
    ///
    /// If `type()` is `Point`, `s2() == t2()`.
    ///
    float s2() const {
        return s2_;
    }

    /// Returns the parameter `t` along the second segment `(a2, b2)` such that
    /// `q()` is approximately equal to `lerp(a2, b2, t)`.
    ///
    /// If `type()` is `Empty`, this value is undefined.
    ///
    /// If `type()` is `Point`, `s2() == t2()`.
    ///
    float t2() const {
        return t2_;
    }

    /// Returns whether `i1` and `i2` are equal.
    ///
    friend constexpr bool operator==(const Segment2xIntersection& i1, const Segment2xIntersection& i2) {
        if (i1.type_ != i2.type_) {
            return false;
        }
        switch (i1.type_) {
        case SegmentIntersectionType::Empty:
            return true;
        case SegmentIntersectionType::Point:
        case SegmentIntersectionType::Segment:
            return i1.p_ == i2.p_
                && i1.q_ == i2.q_
                && i1.s1_ == i2.s1_
                && i1.t1_ == i2.t1_
                && i1.s2_ == i2.s2_
                && i1.t2_ == i2.t2_;
        }
        return false;
    }

    /// Returns whether `s1` and `s2` are different.
    ///
    friend constexpr bool operator!=(const Segment2xIntersection& i1, const Segment2xIntersection& i2) {
        return !(i1 == i2);
    }

private:
    Vec2x p_ = core::noInit;
    Vec2x q_ = core::noInit;
    float s1_; // no-init
    float t1_; // no-init
    float s2_; // no-init
    float t2_; // no-init
    SegmentIntersectionType type_;
};

} // namespace vgc::geometry

// see https://fmt.dev/latest/api.html#formatting-user-defined-types
template<>
struct fmt::formatter<vgc::geometry::Segment2xIntersection> {
    constexpr auto parse(format_parse_context& ctx) {
        auto it = ctx.begin(), end = ctx.end();
        if (it != end && *it != '}') {
            throw format_error("invalid format");
        }
        return it;
    }
    template<typename FormatContext>
    auto format(const vgc::geometry::Segment2xIntersection& i, FormatContext& ctx) {
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

template<typename OStream>
void write(OStream& out, const Segment2xIntersection& i) {
    std::string s = core::format("{}", i);
    write(out, s);
}

/// Computes the intersection between the segment [a1, b1] and the segment [a2, b2].
///
/// \sa `Segment2x::intersect()`.
/// 
VGC_GEOMETRY_API
Segment2xIntersection segmentIntersect(
    const Vec2x& a1,
    const Vec2x& b1,
    const Vec2x& a2,
    const Vec2x& b2);

/// \class vgc::geometry::Segment2x
/// \brief 2D line segment using %scalar-type-description%.
///
/// This class represents a 2D line segment using %scalar-type-description%.
///
/// The segment is internally represented by its start point `a()` and its end
/// point `b()`. This is ideal for storage as it takes a minimal amount of
/// memory.
/// 
/// However, some operations involving segments may require computing the
/// length of the segment and/or the unit vector `(b() - a()).normalized()`.
/// The class `NormalizedSegment2x` also stores this extra information and 
/// may be preferred in some cases.
/// 
/// You can convert a `Segment2x` to a `NormalizedSegment2x` by calling the
/// `normalized()` method.
/// 
// VGC_GEOMETRY_API <- Omitted on purpose.
//                     If needed, manually export individual functions.
class Segment2x {
public:
    using ScalarType = float;
    static constexpr Int dimension = 2;
    using IntersectionType = Segment2xIntersection;

    VGC_WARNING_PUSH
    VGC_WARNING_MSVC_DISABLE(26495) // member variable uninitialized
    /// Creates an uninitialized `Segment2x`.
    ///
    Segment2x(core::NoInit) noexcept
        : data_{Vec2x(core::noInit), Vec2x(core::noInit)} {
    }
    VGC_WARNING_POP

    /// Creates a zero-initialized `Segment2x`.
    ///
    /// This is equivalent to `Segment2x(0, 0, 0, 0)`.
    ///
    constexpr Segment2x() noexcept
        : data_{} {
    }

    /// Creates a `Segment2x` defined by the two points `a` and `b`.
    ///
    constexpr Segment2x(const Vec2x& a, const Vec2x& b) noexcept
        : data_{a, b} {
    }

    /// Creates a `Segment2x` defined by the two points (`ax`, `ay`) and
    /// (`bx`, `by`).
    ///
    constexpr Segment2x(float ax, float ay, float bx, float by) noexcept
        : data_{Vec2x(ax, ay), Vec2x(bx, by)} {
    }

    /// Accesses the `i`-th point of this `Segment2x`, where `i` must be either
    /// `0` or `1`, corresponsing respectively to `a()` and `b()`.
    ///
    constexpr const Vec2x& operator[](Int i) const {
        return data_[i];
    }

    /// Mutates the `i`-th point of this `Triangle2x`.
    ///
    constexpr Vec2x& operator[](Int i) {
        return data_[i];
    }

    /// Returns the start point of the segment.
    ///
    constexpr const Vec2x& a() const {
        return data_[0];
    }

    /// Returns the end point of the segment.
    ///
    constexpr const Vec2x& b() const {
        return data_[1];
    }

    /// Modifies the start point of the segment.
    /// 
    constexpr void setA(const Vec2x& a) {
        data_[0] = a;
    }

    /// Modifies the end point of the segment.
    /// 
    constexpr void setB(const Vec2x& b) {
        data_[1] = b;
    }

    /// Returns the x-coordinate of the start point of the segment.
    ///
    constexpr float ax() const {
        return data_[0][0];
    }

    /// Returns the y-coordinate of the start point of the segment.
    ///
    constexpr float ay() const {
        return data_[0][1];
    }

    /// Returns the x-coordinate of the end point of the segment.
    ///
    constexpr float bx() const {
        return data_[1][0];
    }

    /// Returns the y-coordinate of the end point of the segment.
    ///
    constexpr float by() const {
        return data_[1][1];
    }

    /// Modifies the x-coordinate of the start point of the segment.
    ///
    constexpr void setAx(float ax) {
        data_[0][0] = ax;
    }

    /// Modifies the y-coordinate of the start point of the segment.
    ///
    constexpr void setAy(float ay) {
        data_[0][1] = ay;
    }

    /// Modifies the x-coordinate of the end point of the segment.
    ///
    constexpr void setBx(float bx) {
        data_[1][0] = bx;
    }

    /// Modifies the y-coordinate of the end point of the segment.
    ///
    constexpr void setBy(float by) {
        data_[1][1] = by;
    }

    /// Returns whether the segment is degenerate, that is, whether it is
    /// reduced to a point.
    ///
    constexpr bool isDegenerate() const {
        return a() == b();
    }

    // TODO
/*
    /// Returns a copy of this rectangle with length and unit tangent/normal precomputed.
    /// 
    constexpr NormalizedSegment2x normalized() {
        return NormalizedSegment2x(*this);
    }
*/

    /// Computes the intersection between this segment and the `other` segment.
    ///
    /// \sa `geometry::segmentIntersect()`.
    /// 
    Segment2xIntersection intersect(const Segment2x& other) {
        return segmentIntersect(a(), b(), other.a(), other.b());
    }

    /// Adds in-place the points of `other` to the points of this `Segment2x`.
    ///
    constexpr Segment2x& operator+=(const Segment2x& other) {
        data_[0] += other[0];
        data_[1] += other[1];
        return *this;
    }

    /// Returns the addition of the two segments `s1` and `s2`.
    ///
    friend constexpr Segment2x operator+(const Segment2x& s1, const Segment2x& s2) {
        return Segment2x(s1) += s2;
    }

    /// Returns a copy of this `Segment2x` (unary plus operator).
    ///
    constexpr Segment2x operator+() const {
        return *this;
    }

    /// Substracts in-place the points of `other` from the points of this `Segment2x`.
    ///
    constexpr Segment2x& operator-=(const Segment2x& other) {
        data_[0] -= other[0];
        data_[1] -= other[1];
        return *this;
    }

    /// Returns the substraction of `s1` and `s2`.
    ///
    friend constexpr Segment2x operator-(const Segment2x& s1, const Segment2x& s2) {
        return Segment2x(s1) -= s2;
    }

    /// Returns the opposite of this `Segment2x` (unary minus operator).
    ///
    constexpr Segment2x operator-() const {
        return Segment2x(-data_[0], -data_[1]);
    }

    /// Multiplies in-place all the points in this `Segment2x` by
    /// the scalar `s`.
    ///
    constexpr Segment2x& operator*=(float s) {
        data_[0] *= s;
        data_[1] *= s;
        return *this;
    }

    /// Returns the multiplication of all the points in this `Segment2x` by
    /// the scalar `s`.
    ///
    constexpr Segment2x operator*(float s) const {
        return Segment2x(*this) *= s;
    }

    /// Returns the multiplication of the scalar `s` with the all the points
    /// in the segment `t`.
    ///
    friend constexpr Segment2x operator*(float s, const Segment2x& segment) {
        return segment * s;
    }

    /// Divides in-place the points of this `Segment2x` by the scalar `s`.
    ///
    constexpr Segment2x& operator/=(float s) {
        data_[0] /= s;
        data_[1] /= s;
        return *this;
    }

    /// Returns the division of the points in this `Segment2x` by the scalar `s`.
    ///
    constexpr Segment2x operator/(float s) const {
        return Segment2x(*this) /= s;
    }

    /// Returns whether `s1` and `s2` are equal.
    ///
    friend constexpr bool operator==(const Segment2x& s1, const Segment2x& s2) {
        return s1.data_[0] == s2.data_[0]
            && s1.data_[1] == s2.data_[1];
    }

    /// Returns whether `s1` and `s2` are different.
    ///
    friend constexpr bool operator!=(const Segment2x& s1, const Segment2x& s2) {
        return s1.data_[0] != s2.data_[0]
            || s1.data_[1] != s2.data_[1];
    }

    /// Compares `s1` and `s2` using lexicographic order on (ax, ay, bx, by),
    /// that is, lexicographic order on (a, b) where points are themselves
    /// compared using lexicographic order on (x, y).
    ///
    /// This order is often useful for plane-sweep algorithms.
    /// 
    friend constexpr bool operator<(const Segment2x& s1, const Segment2x& s2) {
        return ( (s1.data_[0] < s2.data_[0]) ||
               (!(s2.data_[0] < s1.data_[0]) &&
               ( (s1.data_[1] < s2.data_[1]))));
    }

    /// Compares `s1` and `s2` using lexicographic order on (ax, ay, bx, by).
    ///
    friend constexpr bool operator<=(const Segment2x& s1, const Segment2x& s2) {
        return !(s2 < s1);
    }

    /// Compares `s1` and `s2` using lexicographic order on (ax, ay, bx, by).
    ///
    friend constexpr bool operator>(const Segment2x& s1, const Segment2x& s2) {
        return s2 < s1;
    }

    /// Compares `s1` and `s2` using lexicographic order on (ax, ay, bx, by).
    ///
    friend constexpr bool operator>=(const Segment2x& s1, const Segment2x& s2) {
        return !(s1 < s2);
    }

private:
    Vec2x data_[2];
};

/// Alias for `vgc::core::Array<vgc::geometry::Segment2x>`.
///
using Segment2xArray = core::Array<Segment2x>;

/// Overloads `setZero(T& x)`.
///
/// \sa `vgc::core::zero<T>()`
///
inline void setZero(Segment2x& s) {
    s = Segment2x();
}

/// Writes the given `Segment2x` to the output stream.
///
template<typename OStream>
void write(OStream& out, const Segment2x& s) {
    write(out, '(', s[0], ", ", s[1], ')');
}

/// Reads a `Segment2x` from the input stream, and stores it in the given output
/// parameter `s`. Leading whitespaces are allowed. Raises `ParseError` if the
/// stream does not start with a `Segment2x`. Raises `RangeError` if one of its
/// coordinates is outside the representable range of a float.
///
template<typename IStream>
void readTo(Segment2x& s, IStream& in) {
    skipWhitespacesAndExpectedCharacter(in, '(');
    readTo(s[0], in);
    skipWhitespacesAndExpectedCharacter(in, ',');
    readTo(s[1], in);
    skipWhitespacesAndExpectedCharacter(in, ')');
}

} // namespace vgc::geometry

// see https://fmt.dev/latest/api.html#formatting-user-defined-types
template<>
struct fmt::formatter<vgc::geometry::Segment2x> {
    constexpr auto parse(format_parse_context& ctx) {
        auto it = ctx.begin(), end = ctx.end();
        if (it != end && *it != '}') {
            throw format_error("invalid format");
        }
        return it;
    }
    template<typename FormatContext>
    auto format(const vgc::geometry::Segment2x& s, FormatContext& ctx) {
        return format_to(ctx.out(), "({}, {})", s[0], s[1]);
    }
};

#endif // VGC_GEOMETRY_SEGMENT2X_H
