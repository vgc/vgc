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
// Instead, edit tools/vec4x.h then run tools/generate.py.

// clang-format off

#ifndef VGC_GEOMETRY_VEC4F_H
#define VGC_GEOMETRY_VEC4F_H

#include <cmath>

#include <vgc/core/array.h>
#include <vgc/geometry/api.h>
#include <vgc/geometry/stride.h>
#include <vgc/geometry/vec.h>

namespace vgc::geometry {

/// \class vgc::geometry::Vec4f
/// \brief 2D vector using single-precision floating points.
///
/// A Vec4f typically represents either a 4D point (= position), a 4D vector (=
/// difference of positions), a 3D vector in homogenous coordinates, or any
/// cases where you need to store 4 contiguous floats.
///
/// The memory size of a `Vec4f` is exactly `4 * sizeof(float)`. This will never
/// change in any future version, as this allows to conveniently use this class
/// for data transfer to the GPU (via OpenGL, Metal, etc.).
///
// VGC_GEOMETRY_API <- Omitted on purpose.
//                     If needed, manually export individual functions.
class Vec4f {
public:
    using ScalarType = float;
    static constexpr Int dimension = 4;

    VGC_WARNING_PUSH
    VGC_WARNING_MSVC_DISABLE(26495) // member variable uninitialized
    /// Creates an uninitialized `Vec4f`.
    ///
    Vec4f(core::NoInit) noexcept {}
    VGC_WARNING_POP

    /// Creates a `Vec4f` initialized to (0, 0, 0, 0).
    ///
    constexpr Vec4f() noexcept
        : data_{0, 0, 0, 0} {
    }


    /// Creates a `Vec4f` initialized with the given `x`, `y`, `z`, `w` coordinates.
    ///
    constexpr Vec4f(float x, float y, float z, float w) noexcept
        : data_{x, y, z, w} {
    }

    /// Creates a `Vec4f` from another `Vec<2, T>` object by performing a
    /// `static_cast` on each of its coordinates.
    ///
    /// ```cpp
    /// vgc::geometrt::Vec4d vd(1, 2);
    /// vgc::geometrt::Vec4f vf(vd); // cast from double to float
    /// ```
    ///
    template<typename TVec4,
        VGC_REQUIRES(
            isVec<TVec4>
         && TVec4::dimension == 4
         && !std::is_same_v<TVec4, Vec4f>)>
    explicit constexpr Vec4f(const TVec4& other) noexcept
        : data_{static_cast<float>(other[0]),
                static_cast<float>(other[1]),
                static_cast<float>(other[2]),
                static_cast<float>(other[3])} {
    }

    /// Returns a pointer to the underlying array of components.
    ///
    const float* data() const {
        return data_;
    }

    /// Returns a pointer to the underlying array of components.
    ///
    float* data() {
        return data_;
    }

    /// Accesses the `i`-th coordinate of this `Vec4f`.
    ///
    constexpr const float& operator[](Int i) const {
        return data_[i];
    }

    /// Mutates the `i`-th coordinate of this `Vec4f`.
    ///
    constexpr float& operator[](Int i) {
        return data_[i];
    }

    /// Accesses the first coordinate of this `Vec4f`.
    ///
    constexpr float x() const {
        return data_[0];
    }

    /// Accesses the second coordinate of this `Vec4f`.
    ///
    constexpr float y() const {
        return data_[1];
    }

    /// Accesses the third coordinate of this `Vec4f`.
    ///
    constexpr float z() const {
        return data_[2];
    }

    /// Accesses the fourth coordinate of this `Vec4f`.
    ///
    constexpr float w() const {
        return data_[3];
    }

    /// Mutates the first coordinate of this `Vec4f`.
    ///
    constexpr void setX(float x) {
        data_[0] = x;
    }

    /// Mutates the second coordinate of this `Vec4f`.
    ///
    constexpr void setY(float y) {
        data_[1] = y;
    }

    /// Mutates the third coordinate of this `Vec4f`.
    ///
    constexpr void setZ(float z) {
        data_[2] = z;
    }

    /// Mutates the fourth coordinate of this `Vec4f`.
    ///
    constexpr void setW(float w) {
        data_[3] = w;
    }

    /// Adds in-place `other` to this `Vec4f`.
    ///
    constexpr Vec4f& operator+=(const Vec4f& other) {
        data_[0] += other[0];
        data_[1] += other[1];
        data_[2] += other[2];
        data_[3] += other[3];
        return *this;
    }

    /// Returns the addition of the two vectors `v1` and `v2`.
    ///
    friend constexpr Vec4f operator+(const Vec4f& v1, const Vec4f& v2) {
        return Vec4f(v1) += v2;
    }

    /// Returns a copy of this `Vec4f` (unary plus operator).
    ///
    constexpr Vec4f operator+() const {
        return *this;
    }

    /// Substracts in-place `other` from this `Vec4f`.
    ///
    constexpr Vec4f& operator-=(const Vec4f& other) {
        data_[0] -= other[0];
        data_[1] -= other[1];
        data_[2] -= other[2];
        data_[3] -= other[3];
        return *this;
    }

    /// Returns the substraction of `v1` and `v2`.
    ///
    friend constexpr Vec4f operator-(const Vec4f& v1, const Vec4f& v2) {
        return Vec4f(v1) -= v2;
    }

    /// Returns the opposite of this `Vec4f` (unary minus operator).
    ///
    constexpr Vec4f operator-() const {
        return Vec4f(-data_[0], -data_[1], -data_[2], -data_[3]);
    }

    /// Multiplies in-place this `Vec4f` by the scalar `s`.
    ///
    constexpr Vec4f& operator*=(float s) {
        data_[0] *= s;
        data_[1] *= s;
        data_[2] *= s;
        data_[3] *= s;
        return *this;
    }

    /// Returns the multiplication of this `Vec4f` by the scalar `s`.
    ///
    constexpr Vec4f operator*(float s) const {
        return Vec4f(*this) *= s;
    }

    /// Returns the multiplication of the scalar `s` with the vector `v`.
    ///
    friend constexpr Vec4f operator*(float s, const Vec4f& v) {
        return v * s;
    }

    /// Divides in-place this `Vec4f` by the scalar `s`.
    ///
    constexpr Vec4f& operator/=(float s) {
        data_[0] /= s;
        data_[1] /= s;
        data_[2] /= s;
        data_[3] /= s;
        return *this;
    }

    /// Returns the division of this `Vec4f` by the scalar `s`.
    ///
    constexpr Vec4f operator/(float s) const {
        return Vec4f(*this) /= s;
    }

    /// Returns whether `v1` and `v2` are equal.
    ///
    friend constexpr bool operator==(const Vec4f& v1, const Vec4f& v2) {
        return v1.data_[0] == v2.data_[0]
            && v1.data_[1] == v2.data_[1]
            && v1.data_[2] == v2.data_[2]
            && v1.data_[3] == v2.data_[3];
    }

    /// Returns whether `v1` and `v2` are different.
    ///
    friend constexpr bool operator!=(const Vec4f& v1, const Vec4f& v2) {
        return v1.data_[0] != v2.data_[0]
            || v1.data_[1] != v2.data_[1]
            || v1.data_[2] != v2.data_[2]
            || v1.data_[3] != v2.data_[3];
    }

    /// Compares `v1` and `v2` using the lexicographic
    /// order.
    ///
    friend constexpr bool operator<(const Vec4f& v1, const Vec4f& v2) {
        return (v1.data_[0] < v2.data_[0]) || (
                !(v2.data_[0] < v1.data_[0]) && (
                   (v1.data_[1] < v2.data_[1]) || (
                    !(v2.data_[1] < v1.data_[1]) && (
                       (v1.data_[2] < v2.data_[2]) || (
                        !(v2.data_[2] < v1.data_[2]) && (
                            v1.data_[3] < v2.data_[3]
                         )
                       )
                     )
                   )
                 )
               );
    }

    /// Compares `v1` and `v2` using the lexicographic
    /// order.
    ///
    friend constexpr bool operator<=(const Vec4f& v1, const Vec4f& v2) {
        return !(v2 < v1);
    }

    /// Compares the `v1` and `v2` using the lexicographic
    /// order.
    ///
    friend constexpr bool operator>(const Vec4f& v1, const Vec4f& v2) {
        return v2 < v1;
    }

    /// Compares `v1` and `v2` using the lexicographic
    /// order.
    ///
    friend constexpr bool operator>=(const Vec4f& v1, const Vec4f& v2) {
        return !(v1 < v2);
    }

    /// Returns the Euclidean length of this `Vec4f`.
    ///
    float length() const {
        return std::sqrt(squaredLength());
    }

    /// Returns the square of the Euclidean length of this `Vec4f`.
    ///
    /// This function is faster than `length()`, therefore it is a good idea to
    /// use it whenever you don't need the actual length. For example, if you
    /// need to know which vector has greater length, you can use
    /// `v1.squaredLength() < v2.squaredLength()`.
    ///
    constexpr float squaredLength() const {
        return data_[0] * data_[0]
             + data_[1] * data_[1]
             + data_[2] * data_[2]
             + data_[3] * data_[3];
    }

    /// Makes this `Vec4f` a unit vector by dividing it by its length.
    ///
    /// If provided, `isNormalizable` is set to either `true` or `false`
    /// depending on whether the vector was considered normalizable.
    ///
    /// The vector is considered non-normalizable whenever its length is less
    /// or equal than the given `epsilon`. An appropriate epsilon is
    /// context-dependent, and therefore zero is used as default, which means
    /// that the vector is considered non-normalizable if and only if it is
    /// exactly equal to the null vector `Vec4f()`.
    ///
    /// If the vector is considered non-normalizable, then it is set to
    /// `(1.0f, 0.0f, 0.0f, 0.0f)`.
    ///
    /// \sa `length()`.
    ///
    Vec4f& normalize(bool* isNormalizable = nullptr, float epsilon = 0.0f);

    /// Returns a normalized copy of this Vec4f.
    ///
    Vec4f normalized(bool* isNormalizable = nullptr, float epsilon = 0.0f) const;

    /// Returns the dot product between this Vec4f `a` and the given Vec4f `b`.
    ///
    /// ```cpp
    /// float d = a.dot(b);
    /// ```
    ///
    constexpr float dot(const Vec4f& b) const {
        const Vec4f& a = *this;
        return a[0]*b[0] + a[1]*b[1] + a[2]*b[2] + a[3]*b[3];
    }

    /// Returns the angle, in radians and in the interval [0, π], between this
    /// Vec4f `a` and the given Vec4f `b`.
    ///
    /// This value is computed using the following formula:
    ///
    /// ```cpp
    /// float angle = acos(a.dot(b) / (a.length() * b.length()));
    /// ```
    ///
    /// It returns an undefined value if either `a` or `b` is zero-length.
    ///
    float angle(const Vec4f& b) const {
        const Vec4f& a = *this;
        return std::acos(a.dot(b) / (a.length() * b.length()));
    }

    /// Returns whether this Vec4f `a` and the given Vec4f `b` are almost equal
    /// within some relative tolerance. See `Vec2f::isClose()` for details.
    ///
    bool isClose(const Vec4f& b, float relTol = 1e-5f, float absTol = 0.0f) const {
        const Vec4f& a = *this;
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

    /// Returns whether all coordinates in this Vec4f `a` are almost equal to
    /// their corresponding coordinate in the given Vec4f `b`, within some
    /// relative tolerance. See `Vec2f::allClose()` for details.
    ///
    bool allClose(const Vec4f& b, float relTol = 1e-5f, float absTol = 0.0f) const {
        const Vec4f& a = *this;
        return core::isClose(a[0], b[0], relTol, absTol)
            && core::isClose(a[1], b[1], relTol, absTol)
            && core::isClose(a[2], b[2], relTol, absTol)
            && core::isClose(a[3], b[3], relTol, absTol);
    }

    /// Returns whether the euclidean distance between this Vec4f `a` and the
    /// given Vec4f `b` is smaller or equal than the given absolute tolerance.
    /// See `Vec2f::isNear()` for details.
    ///
    bool isNear(const Vec4f& b, float absTol) const {
        const Vec4f& a = *this;
        float diff2 = a.infdiff_(b).squaredLength();
        if (diff2 == std::numeric_limits<float>::infinity()) {
            return false; // opposite infinities or finite/infinite mismatch
        }
        else {
            float absTol2 = absTol * absTol;
            return diff2 <= absTol2;
        }
    }

    /// Returns whether all coordinates in this Vec4f `a` are within some
    /// absolute tolerance of their corresponding coordinate in the given Vec4f
    /// `b`. See `Vec2f::allNear()` for details.
    ///
    bool allNear(const Vec4f& b, float absTol) const {
        const Vec4f& a = *this;
        return core::isNear(a[0], b[0], absTol)
            && core::isNear(a[1], b[1], absTol)
            && core::isNear(a[2], b[2], absTol)
            && core::isNear(a[3], b[3], absTol);
    }

private:
    float data_[4];

    Vec4f infdiff_(const Vec4f& b) const {
        const Vec4f& a = *this;
        return Vec4f(
            core::detail::infdiff(a[0], b[0]),
            core::detail::infdiff(a[1], b[1]),
            core::detail::infdiff(a[2], b[2]),
            core::detail::infdiff(a[3], b[3]));
    }
};

// See Vec2f for detailed comments
inline Vec4f& Vec4f::normalize(bool* isNormalizable, float epsilon_) {
    float l2 = squaredLength();
    if (l2 <= epsilon_*epsilon_) {
        if (isNormalizable) {
            *isNormalizable = false;
        }
        *this = Vec4f(1.0f, 0.0f, 0.0f, 0.0f);
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

inline Vec4f Vec4f::normalized(bool* isNormalizable, float epsilon_) const {
    return Vec4f(*this).normalize(isNormalizable, epsilon_);
}

/// Alias for `vgc::core::Array<vgc::geometry::Vec4f>`.
///
using Vec4fArray = core::Array<Vec4f>;

/// Alias for `vgc::core::SharedConstArray<vgc::geometry::Vec4f>`.
///
using SharedConstVec4fArray = core::SharedConstArray<Vec4f>;

/// Allows to iterate over a range of `Vec4f` stored in a memory buffer of
/// floats, where consecutive `Vec4f` elements are separated by a given stride.
///
using Vec4fSpan = StrideSpan<float, Vec4f>;

/// Const version of `Vec4fSpan`.
///
using Vec4fConstSpan = StrideSpan<const float, const Vec4f>;

/// Overloads `setZero(T& x)`.
///
/// \sa `vgc::core::zero<T>()`.
///
inline void setZero(Vec4f& v) {
    v[0] = 0.0f;
    v[1] = 0.0f;
    v[2] = 0.0f;
    v[3] = 0.0f;
}

/// Writes the given `Vec4f` to the output stream.
///
template<typename OStream>
void write(OStream& out, const Vec4f& v) {
    write(out, '(', v[0], ", ", v[1], ", ", v[2], ", ", v[3], ')');
}

/// Reads a `Vec4f` from the input stream, and stores it in the given output
/// parameter `v`. Leading whitespaces are allowed. Raises `ParseError` if the
/// stream does not start with a `Vec4f`. Raises `RangeError` if one of its
/// coordinates is outside the representable range of a float.
///
template <typename IStream>
void readTo(Vec4f& v, IStream& in) {
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
    skipExpectedCharacter(in, ',');
    readTo(v[3], in);
    skipWhitespaceCharacters(in);
    skipExpectedCharacter(in, ')');
}

} // namespace vgc::geometry

// see https://fmt.dev/latest/api.html#formatting-user-defined-types
template <>
struct fmt::formatter<vgc::geometry::Vec4f> {
    constexpr auto parse(format_parse_context& ctx) {
        auto it = ctx.begin(), end = ctx.end();
        if (it != end && *it != '}') {
            throw format_error("invalid format");
        }
        return it;
    }
    template <typename FormatContext>
    auto format(const vgc::geometry::Vec4f& v, FormatContext& ctx) {
        return format_to(ctx.out(),"({}, {}, {}, {})", v[0], v[1], v[2], v[3]);
    }
};

#endif // VGC_GEOMETRY_VEC4F_H
