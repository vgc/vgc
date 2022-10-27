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

#ifndef VGC_CORE_COLOR_H
#define VGC_CORE_COLOR_H

#include <array>

#include <vgc/core/api.h>
#include <vgc/core/arithmetic.h>
#include <vgc/core/array.h>
#include <vgc/core/format.h>
#include <vgc/core/parse.h>

namespace vgc::core {

/// \class vgc::core::Color
/// \brief A class to represent a given color and opacity.
///
/// For now, the color is stored as RBGA with single-precision floats in [0, 1].
/// In the future, we could choose to store HSL
///
class VGC_CORE_API Color {
public:
    /// Creates an uninitialized Color.
    ///
    Color(core::NoInit) {
    }

    /// Creates a Color initialized to black.
    ///
    Color()
        : data_{0.0f, 0.0f, 0.0f, 1.0f} {
    }

    /// Creates a Color from the given HSL values.
    ///
    /// ```cpp
    /// core::Color c = core::Color::hsl(270, 0.6, 0.7);
    /// ```
    ///
    /// The hue should be given in degrees and implicitly wraps around (e.g.,
    /// adding or substrating any multiple of 360 doesn't change the returned
    /// color), and the saturation and lightness should be given in [0, 1] and
    /// are implicitly clamped to this range.
    ///
    static Color hsl(float h, float s, float l);

    /// Creates a Color initialized with the given r, g, b in [0, 1]. The alpha
    /// channel is set to 1.0.
    ///
    Color(float r, float g, float b)
        : data_{r, g, b, 1.0f} {
    }

    /// Creates a Color initialized with the given r, g, b, a in [0, 1].
    ///
    Color(float r, float g, float b, float a)
        : data_{r, g, b, a} {
    }

    /// Creates a Color from the given hexadecimal string.
    ///
    /// ```cpp
    /// core::Color c = core::Color::fromHex("#f375a3");
    /// core::Color c = core::Color::fromHex("#d4c"); // same as "#dd44cc"
    /// ```
    ///
    static Color fromHex(std::string_view hex);

    /// Accesses the i-th channel of the Color.
    ///
    const float& operator[](int i) const {
        return data_[i];
    }

    /// Mutates the i-th channel of the Color.
    ///
    float& operator[](int i) {
        return data_[i];
    }

    /// Accesses the red channel of the Color.
    ///
    float r() const {
        return data_[0];
    }

    /// Accesses the green channel of the Color.
    ///
    float g() const {
        return data_[1];
    }

    /// Accesses the blue channel of the Color.
    ///
    float b() const {
        return data_[2];
    }

    /// Accesses the alpha channel of the Color.
    ///
    float a() const {
        return data_[3];
    }

    /// Mutates the red channel of the Color.
    ///
    void setR(float r) {
        data_[0] = r;
    }

    /// Mutates the green channel of the Color.
    ///
    void setG(float g) {
        data_[1] = g;
    }

    /// Mutates the blue channel of the Color.
    ///
    void setB(float b) {
        data_[2] = b;
    }

    /// Mutates the alpha channel of the Color.
    ///
    void setA(float a) {
        data_[3] = a;
    }

    /// Returns the current color converted to an HSL representation.
    ///
    std::array<float, 3> toHsl() const;

    /// Returns the current color as an hexadecimal string.
    ///
    std::string toHex() const;

    /// Rounds the current color to the nearest RGB value representable as an
    /// 8bit 0-255 value.
    ///
    Color& round8b();

    /// Returns a color rounded to the nearest RGB value representable as an
    /// 8-bit value in the range 0-255.
    ///
    Color rounded8b() {
        return Color(*this).round8b();
    }

    /// Adds in-place the \p other Color to this Color.
    /// This is a component-wise addition of all channels, including the alpha
    /// channel.
    ///
    Color& operator+=(const Color& other) {
        data_[0] += other[0];
        data_[1] += other[1];
        data_[2] += other[2];
        data_[3] += other[3];
        return *this;
    }

    /// Returns the addition of the Color \p c1 and the Color \p c2.
    /// This is a component-wise addition of all channels, including the alpha
    /// channel.
    ///
    friend Color operator+(const Color& c1, const Color& c2) {
        return Color(c1) += c2;
    }

    /// Substracts in-place the \p other Color to this Color.
    /// This is a component-wise substraction of all channels, including the
    /// alpha channel.
    ///
    Color& operator-=(const Color& other) {
        data_[0] -= other[0];
        data_[1] -= other[1];
        data_[2] -= other[2];
        data_[3] -= other[3];
        return *this;
    }

    /// Returns the substraction of the Color \p c1 and the Color \p c2.
    /// This is a component-wise substraction of all channels, including the
    /// alpha channel.
    ///
    friend Color operator-(const Color& c1, const Color& c2) {
        return Color(c1) -= c2;
    }

    /// Multiplies in-place this Color by the given scalar \p s.
    /// This multiplies all channels, including the alpha channel.
    ///
    Color& operator*=(float s) {
        data_[0] *= s;
        data_[1] *= s;
        data_[2] *= s;
        data_[3] *= s;
        return *this;
    }

    /// Returns the multiplication of this Color by the given scalar \p s.
    /// This multiplies all channels, including the alpha channel.
    ///
    Color operator*(float s) const {
        return Color(*this) *= s;
    }

    /// Returns the multiplication of the scalar \p s with the Color \p c.
    /// This multiplies all channels, including the alpha channel.
    ///
    friend Color operator*(float s, const Color& c) {
        return c * s;
    }

    /// Divides in-place this Color by the given scalar \p s.
    /// This divides all channels, including the alpha channel.
    ///
    Color& operator/=(float s) {
        data_[0] /= s;
        data_[1] /= s;
        data_[2] /= s;
        data_[3] /= s;
        return *this;
    }

    /// Returns the division of this Color by the given scalar \p s.
    /// This divides all channels, including the alpha channel.
    ///
    Color operator/(float s) const {
        return Color(*this) /= s;
    }

    /// Returns whether the two given Color \p c1 and \p c2 are equal.
    ///
    friend bool operator==(const Color& c1, const Color& c2) {
        return c1.data_[0] == c2.data_[0]    //
               && c1.data_[1] == c2.data_[1] //
               && c1.data_[2] == c2.data_[2] //
               && c1.data_[3] == c2.data_[3];
    }

    /// Returns whether the two given Color \p c1 and \p c2 are different.
    ///
    friend bool operator!=(const Color& c1, const Color& c2) {
        return !(c1 == c2);
    }

    /// Compares the two Color \p c1 and \p c2 using the lexicographic
    /// order.
    ///
    friend bool operator<(const Color& c1, const Color& c2) {
        // clang-format off
        return ( (c1.data_[0] < c2.data_[0]) ||
               (!(c2.data_[0] < c1.data_[0]) &&
               ( (c1.data_[1] < c2.data_[1]) ||
               (!(c2.data_[1] < c1.data_[1]) &&
               ( (c1.data_[2] < c2.data_[2]) ||
               (!(c2.data_[2] < c1.data_[2]) &&
               ( (c1.data_[3] < c2.data_[3]))))))));
        // clang-format on
    }

    /// Compares the two Color \p c1 and \p c2 using the lexicographic
    /// order.
    ///
    friend bool operator<=(const Color& c1, const Color& c2) {
        return !(c2 < c1);
    }

    /// Compares the two Color \p c1 and \p c2 using the lexicographic
    /// order.
    ///
    friend bool operator>(const Color& c1, const Color& c2) {
        return c2 < c1;
    }

    /// Compares the two Color \p c1 and \p c2 using the lexicographic
    /// order.
    ///
    friend bool operator>=(const Color& c1, const Color& c2) {
        return !(c1 < c2);
    }

    /// Converts a floating point in [0, 1] to an integer in [0..255].
    ///
    static UInt8 mapTo255(float x) {
        float y = std::round(clamp(x, 0.0f, 1.0f) * 255.0f);
        return static_cast<UInt8>(y);
    }

    /// Converts an integer in [0..255] to a floating point in [0, 1].
    ///
    static float mapFrom255(Int x) {
        Int y = clamp(x, Int(0), Int(255));
        return static_cast<float>(y) / 255.0f;
    }

private:
    float data_[4];
};

/// Alias for `vgc::core::Array<vgc::core::Color>`.
///
using ColorArray = Array<Color>;

/// Writes the given Color to the output stream.
///
/// The written string is a valid CSS Color Module Level 3, see:
///
/// https://www.w3.org/TR/2018/PR-css-color-3-20180315/
///
/// ```cpp
/// Color red(1, 0, 0);
/// Color halfGreen(0, 1, 0, 0.5);
/// vgc::core::write(out, red);       // writes "rgb(255, 0, 0)"
/// vgc::core::write(out, halfGreen); // writes "rgba(0, 255, 0, 0.5)"
/// ```
///
template<typename OStream>
void write(OStream& out, const Color& c) {
    bool writeAlpha = (c.a() != 1.0);
    write(out, writeAlpha ? "rgba(" : "rgb(");
    write(
        out,
        Color::mapTo255(c.r()),
        ", ",
        Color::mapTo255(c.g()),
        ", ",
        Color::mapTo255(c.b()));
    if (writeAlpha) {
        write(out, ", ", c.a());
    }
    write(out, ')');
}

/// Reads a Color from the input stream, and stores it in the given output
/// parameter. Leading whitespaces are allowed. Raises ParseError if the stream
/// does not start with a Color. Raises RangeError if one of its coordinate is
/// outside the representable range of an int or double.
///
template<typename IStream>
void readTo(Color& c, IStream& in) {
    skipWhitespaceCharacters(in);
    skipExpectedCharacter(in, 'r');
    skipExpectedCharacter(in, 'g');
    skipExpectedCharacter(in, 'b');
    char d = readExpectedCharacter(in, {'a', '('});
    bool hasAlpha = false;
    if (d == 'a') {
        hasAlpha = true;
        skipExpectedCharacter(in, '(');
    }
    c[0] = Color::mapFrom255(read<Int>(in));
    skipWhitespaceCharacters(in);
    skipExpectedCharacter(in, ',');
    c[1] = Color::mapFrom255(read<Int>(in));
    skipWhitespaceCharacters(in);
    skipExpectedCharacter(in, ',');
    c[2] = Color::mapFrom255(read<Int>(in));
    skipWhitespaceCharacters(in);
    if (hasAlpha) {
        skipExpectedCharacter(in, ',');
        readTo(c[3], in);
        skipWhitespaceCharacters(in);
    }
    else {
        c[3] = 1.0;
    }
    skipExpectedCharacter(in, ')');
}

} // namespace vgc::core

template<>
struct fmt::formatter<vgc::core::Color> {
    constexpr auto parse(format_parse_context& ctx) {
        auto it = ctx.begin(), end = ctx.end();
        if (it != end && *it != '}')
            throw format_error("invalid format");
        return it;
    }
    template<typename FormatContext>
    auto format(const vgc::core::Color& c, FormatContext& ctx) {
        vgc::UInt8 r = vgc::core::Color::mapTo255(c.r());
        vgc::UInt8 g = vgc::core::Color::mapTo255(c.g());
        vgc::UInt8 b = vgc::core::Color::mapTo255(c.b());
        double a = c.a();
        if (a == 1.0) {
            return format_to(ctx.out(), "rgb({}, {}, {})", r, g, b);
        }
        else {
            return format_to(ctx.out(), "rgba({}, {}, {}, {})", r, g, b, a);
        }
    }
};

#endif // VGC_CORE_COLOR_H
