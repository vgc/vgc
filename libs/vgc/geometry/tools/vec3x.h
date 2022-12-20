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

#ifndef VGC_GEOMETRY_VEC3X_H
#define VGC_GEOMETRY_VEC3X_H

#include <cmath>

#include <vgc/core/array.h>
#include <vgc/geometry/api.h>
#include <vgc/geometry/stride.h>
#include <vgc/geometry/vec.h>

namespace vgc::geometry {

/// \class vgc::geometry::Vec3x
/// \brief 3D vector using %scalar-type-description%.
///
/// A Vec3x represents either a 3D point (= position), a 3D vector (=
/// difference of positions), a 3D size (= positive position), or a 3D normal
/// (= unit vector). Unlike other libraries, we do not use separate types for
/// all these use cases.
///
/// The memory size of a `Vec3x` is exactly `3 * sizeof(float)`. This will never
/// change in any future version, as this allows to conveniently use this class
/// for data transfer to the GPU (via OpenGL, Metal, etc.).
///
// VGC_GEOMETRY_API <- Omitted on purpose.
//                     If needed, manually export individual functions.
class Vec3x {
public:
    using ScalarType = float;
    static constexpr Int dimension = 3;

    VGC_WARNING_PUSH
    VGC_WARNING_MSVC_DISABLE(26495) // member variable uninitialized
    /// Creates an uninitialized `Vec3x`.
    ///
    Vec3x(core::NoInit) noexcept {
    }
    VGC_WARNING_POP

    /// Creates a `Vec3x` initialized to (0, 0, 0).
    ///
    constexpr Vec3x() noexcept
        : data_{0, 0, 0} {
    }


    /// Creates a `Vec3x` initialized with the given `x`, `y`, and `z` coordinates.
    ///
    constexpr Vec3x(float x, float y, float z) noexcept
        : data_{x, y, z} {
    }

    /// Creates a `Vec3x` from another `Vec<3, T>` object by performing a
    /// `static_cast` on each of its coordinates.
    ///
    template<typename TVec3,
        VGC_REQUIRES(
            isVec<TVec3>
         && TVec3::dimension == 3
         && !std::is_same_v<TVec3, Vec3x>)>
    explicit constexpr Vec3x(const TVec3& other) noexcept
        : data_{static_cast<float>(other[0]),
                static_cast<float>(other[1]),
                static_cast<float>(other[2])} {
    }

    /// Accesses the `i`-th coordinate of this `Vec3x`.
    ///
    constexpr const float& operator[](Int i) const {
        return data_[i];
    }

    /// Mutates the `i`-th coordinate of this `Vec3x`.
    ///
    constexpr float& operator[](Int i) {
        return data_[i];
    }

    /// Accesses the first coordinate of this `Vec3x`.
    ///
    constexpr float x() const {
        return data_[0];
    }

    /// Accesses the second coordinate of this `Vec3x`.
    ///
    constexpr float y() const {
        return data_[1];
    }

    /// Accesses the third coordinate of this `Vec3x`.
    ///
    constexpr float z() const {
        return data_[2];
    }

    /// Mutates the first coordinate of this `Vec3x`.
    ///
    constexpr void setX(float x) {
        data_[0] = x;
    }

    /// Mutates the second coordinate of this `Vec3x`.
    ///
    constexpr void setY(float y) {
        data_[1] = y;
    }

    /// Mutates the third coordinate of this `Vec3x`.
    ///
    constexpr void setZ(float z) {
        data_[2] = z;
    }

    /// Adds in-place `other` to this `Vec3x`.
    ///
    constexpr Vec3x& operator+=(const Vec3x& other) {
        data_[0] += other[0];
        data_[1] += other[1];
        data_[2] += other[2];
        return *this;
    }

    /// Returns the addition of the two vectors `v1` and `v2`.
    ///
    friend constexpr Vec3x operator+(const Vec3x& v1, const Vec3x& v2) {
        return Vec3x(v1) += v2;
    }

    /// Returns a copy of this `Vec3x` (unary plus operator).
    ///
    constexpr Vec3x operator+() const {
        return *this;
    }

    /// Substracts in-place `other` from this `Vec3x`.
    ///
    constexpr Vec3x& operator-=(const Vec3x& other) {
        data_[0] -= other[0];
        data_[1] -= other[1];
        data_[2] -= other[2];
        return *this;
    }

    /// Returns the substraction of `v1` and `v2`.
    ///
    friend constexpr Vec3x operator-(const Vec3x& v1, const Vec3x& v2) {
        return Vec3x(v1) -= v2;
    }

    /// Returns the opposite of this `Vec3x` (unary minus operator).
    ///
    constexpr Vec3x operator-() const {
        return Vec3x(-data_[0], -data_[1], -data_[2]);
    }

    /// Multiplies in-place this `Vec3x` by the scalar `s`.
    ///
    constexpr Vec3x& operator*=(float s) {
        data_[0] *= s;
        data_[1] *= s;
        data_[2] *= s;
        return *this;
    }

    /// Returns the multiplication of this `Vec3x` by the scalar `s`.
    ///
    constexpr Vec3x operator*(float s) const {
        return Vec3x(*this) *= s;
    }

    /// Returns the multiplication of the scalar `s` with the vector `v`.
    ///
    friend constexpr Vec3x operator*(float s, const Vec3x& v) {
        return v * s;
    }

    /// Divides in-place this `Vec3x` by the scalar `s`.
    ///
    constexpr Vec3x& operator/=(float s) {
        data_[0] /= s;
        data_[1] /= s;
        data_[2] /= s;
        return *this;
    }

    /// Returns the division of this `Vec3x` by the scalar `s`.
    ///
    constexpr Vec3x operator/(float s) const {
        return Vec3x(*this) /= s;
    }

    /// Returns whether `v1` and `v2` are equal.
    ///
    friend constexpr bool operator==(const Vec3x& v1, const Vec3x& v2) {
        return v1.data_[0] == v2.data_[0]
            && v1.data_[1] == v2.data_[1]
            && v1.data_[2] == v2.data_[2];
    }

    /// Returns whether `v1` and `v2` are different.
    ///
    friend constexpr bool operator!=(const Vec3x& v1, const Vec3x& v2) {
        return v1.data_[0] != v2.data_[0]
            || v1.data_[1] != v2.data_[1]
            || v1.data_[2] != v2.data_[2];
    }

    /// Compares `v1` and `v2` using the lexicographic
    /// order.
    ///
    friend constexpr bool operator<(const Vec3x& v1, const Vec3x& v2) {
        return (v1.data_[0] < v2.data_[0]) || (
                !(v2.data_[0] < v1.data_[0]) && (
                   (v1.data_[1] < v2.data_[1]) || (
                    !(v2.data_[1] < v1.data_[1]) && (
                        v1.data_[2] < v2.data_[2]
                     )
                   )
                 )
               );
    }

    /// Compares `v1` and `v2` using the lexicographic
    /// order.
    ///
    friend constexpr bool operator<=(const Vec3x& v1, const Vec3x& v2) {
        return !(v2 < v1);
    }

    /// Compares the `v1` and `v2` using the lexicographic
    /// order.
    ///
    friend constexpr bool operator>(const Vec3x& v1, const Vec3x& v2) {
        return v2 < v1;
    }

    /// Compares `v1` and `v2` using the lexicographic
    /// order.
    ///
    friend constexpr bool operator>=(const Vec3x& v1, const Vec3x& v2) {
        return !(v1 < v2);
    }

    /// Returns the Euclidean length of this `Vec3x`.
    ///
    float length() const {
        return std::sqrt(squaredLength());
    }

    /// Returns the square of the Euclidean length of this `Vec3x`.
    ///
    /// This function is faster than `length()`, therefore it is a good idea to
    /// use it whenever you don't need the actual length. For example, if you
    /// need to know which vector has greater length, you can use
    /// `v1.squaredLength() < v2.squaredLength()`.
    ///
    constexpr float squaredLength() const {
        return data_[0] * data_[0]
             + data_[1] * data_[1]
             + data_[2] * data_[2];
    }

    /// Makes this `Vec3x` a unit vector by dividing it by its length.
    ///
    /// If provided, `isNormalizable` is set to either `true` or `false`
    /// depending on whether the vector was considered normalizable.
    ///
    /// The vector is considered non-normalizable whenever its length is less
    /// or equal than the given `epsilon`. An appropriate epsilon is
    /// context-dependent, and therefore zero is used as default, which means
    /// that the vector is considered non-normalizable if and only if it is
    /// exactly equal to the null vector `Vec3x()`.
    ///
    /// If the vector is considered non-normalizable, then it is set to
    /// `(1.0f, 0.0f, 0.0f)`.
    ///
    /// \sa `length()`.
    ///
    Vec3x& normalize(bool* isNormalizable = nullptr, float epsilon = 0.0f);

    /// Returns a normalized copy of this Vec3x.
    ///
    Vec3x normalized(bool* isNormalizable = nullptr, float epsilon = 0.0f) const;

    /// Returns the dot product between this Vec3x `a` and the given Vec3x `b`.
    ///
    /// ```cpp
    /// float d = a.dot(b);
    /// ```
    ///
    /// Note that this is also equal to `a.length() * b.length() * cos(a.angle(b))`.
    ///
    /// \sa cross(), angle()
    ///
    constexpr float dot(const Vec3x& b) const {
        const Vec3x& a = *this;
        return a[0]*b[0] + a[1]*b[1] + a[2]*b[2];
    }

    /// Returns the cross product between this Vec3x `a` and the given Vec3x `b`.
    ///
    /// ```cpp
    /// float d = a.cross(b);
    /// ```
    ///
    constexpr Vec3x cross(const Vec3x& b) const {
        const Vec3x& a = *this;
        return Vec3x(
            a[1]*b[2] - a[2]*b[1],
            a[2]*b[0] - a[0]*b[2],
            a[0]*b[1] - a[1]*b[0]);
    }

    /// Returns the angle, in radians and in the interval [0, π], between this
    /// Vec3x `a` and the given Vec3x `b`.
    ///
    /// This value is computed using the following formula:
    ///
    /// ```cpp
    /// float angle = atan2(a.cross(b).length(), a.dot(b));
    /// ```
    ///
    /// It returns an undefined value if either `a` or `b` is zero-length.
    ///
    /// \sa cross(), dot(), length()
    ///
    float angle(const Vec3x& b) const {
        const Vec3x& a = *this;
        return std::atan2(a.cross(b).length(), a.dot(b));
    }

    /// Returns whether this Vec3x `a` and the given Vec3x `b` are almost equal
    /// within some relative tolerance. See `Vec2x::isClose()` for details.
    ///
    bool isClose(const Vec3x& b, float relTol = 1e-5f, float absTol = 0.0f) const {
        const Vec3x& a = *this;
        float diff2 = a.infdiff_(b).squaredLength();
        if (diff2 == std::numeric_limits<float>::infinity()) {
            return false; // opposite infinities or finite/infinite mismatch
        }
        else {
            float relTol2 = relTol * relTol;
            float absTol2 = absTol * absTol;
            return diff2 <= relTol2 * b.squaredLength()
                || diff2 <= relTol2 * a.squaredLength()
                || diff2 <= absTol2;
        }
    }

    /// Returns whether all coordinates in this Vec3x `a` are almost equal to
    /// their corresponding coordinate in the given Vec3x `b`, within some
    /// relative tolerance. See `Vec2x::allClose()` for details.
    ///
    bool allClose(const Vec3x& b, float relTol = 1e-5f, float absTol = 0.0f) const {
        const Vec3x& a = *this;
        return core::isClose(a[0], b[0], relTol, absTol)
            && core::isClose(a[1], b[1], relTol, absTol)
            && core::isClose(a[2], b[2], relTol, absTol);
    }

    /// Returns whether the euclidean distance between this Vec3x `a` and the
    /// given Vec3x `b` is smaller or equal than the given absolute tolerance.
    /// See `Vec2x::isNear()` for details.
    ///
    bool isNear(const Vec3x& b, float absTol) const {
        const Vec3x& a = *this;
        float diff2 = a.infdiff_(b).squaredLength();
        if (diff2 == std::numeric_limits<float>::infinity()) {
            return false; // opposite infinities or finite/infinite mismatch
        }
        else {
            float absTol2 = absTol * absTol;
            return diff2 <= absTol2;
        }
    }

    /// Returns whether all coordinates in this Vec3x `a` are within some
    /// absolute tolerance of their corresponding coordinate in the given Vec3x
    /// `b`. See `Vec2x::allNear()` for details.
    ///
    bool allNear(const Vec3x& b, float absTol) const {
        const Vec3x& a = *this;
        return core::isNear(a[0], b[0], absTol)
            && core::isNear(a[1], b[1], absTol)
            && core::isNear(a[2], b[2], absTol);
    }

private:
    float data_[3];

    Vec3x infdiff_(const Vec3x& b) const {
        const Vec3x& a = *this;
        return Vec3x(
            core::detail::infdiff(a[0], b[0]),
            core::detail::infdiff(a[1], b[1]),
            core::detail::infdiff(a[2], b[2]));
    }
};

// See Vec2x for detailed comments
inline Vec3x& Vec3x::normalize(bool* isNormalizable, float epsilon_) {
    float l2 = squaredLength();
    if (l2 <= epsilon_*epsilon_) {
        if (isNormalizable) {
            *isNormalizable = false;
        }
        *this = Vec3x(1.0f, 0.0f, 0.0f);
    }
    else {
        if (isNormalizable) {
            *isNormalizable = true;
        }
        float l = std::sqrt(l2);
        *this /= l;
    }
    return *this;
}

inline Vec3x Vec3x::normalized(bool* isNormalizable, float epsilon_) const {
    return Vec3x(*this).normalize(isNormalizable, epsilon_);
}

/// Alias for `vgc::core::Array<vgc::geometry::Vec3x>`.
///
using Vec3xArray = core::Array<Vec3x>;

/// Alias for `vgc::core::SharedConstArray<vgc::geometry::Vec3x>`.
///
using SharedConstVec3xArray = core::SharedConstArray<Vec3x>;

/// Allows to iterate over a range of `Vec3x` stored in a memory buffer of
/// floats, where consecutive `Vec3x` elements are separated by a given stride.
///
using Vec3xSpan = StrideSpan<float, Vec3x>;

/// Const version of `Vec3xSpan`.
///
using Vec3xConstSpan = StrideSpan<const float, const Vec3x>;

/// Overloads `setZero(T& x)`.
///
/// \sa `vgc::core::zero<T>()`.
///
inline void setZero(Vec3x& v) {
    v[0] = 0.0f;
    v[1] = 0.0f;
    v[2] = 0.0f;
}

/// Writes the given `Vec3x` to the output stream.
///
template<typename OStream>
void write(OStream& out, const Vec3x& v) {
    write(out, '(', v[0], ", ", v[1], ',', v[2], ')');
}

/// Reads a `Vec3x` from the input stream, and stores it in the given output
/// parameter `v`. Leading whitespaces are allowed. Raises `ParseError` if the
/// stream does not start with a `Vec3x`. Raises `RangeError` if one of its
/// coordinates is outside the representable range of a float.
///
template <typename IStream>
void readTo(Vec3x& v, IStream& in) {
    skipWhitespaceCharacters(in);
    skipExpectedCharacter(in, '(');
    readTo(v[0], in);
    skipWhitespaceCharacters(in);
    skipExpectedCharacter(in, ',');
    readTo(v[1], in);
    skipWhitespaceCharacters(in);
    skipExpectedCharacter(in, ',');
    readTo(v[2], in);
    skipWhitespaceCharacters(in);
    skipExpectedCharacter(in, ')');
}

} // namespace vgc::geometry

// see https://fmt.dev/latest/api.html#formatting-user-defined-types
template <>
struct fmt::formatter<vgc::geometry::Vec3x> {
    constexpr auto parse(format_parse_context& ctx) {
        auto it = ctx.begin(), end = ctx.end();
        if (it != end && *it != '}') {
            throw format_error("invalid format");
        }
        return it;
    }
    template <typename FormatContext>
    auto format(const vgc::geometry::Vec3x& v, FormatContext& ctx) {
        return format_to(ctx.out(),"({}, {}, {})", v[0], v[1], v[2]);
    }
};

#endif // VGC_GEOMETRY_VEC3X_H
