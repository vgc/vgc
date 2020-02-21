// Copyright 2020 The VGC Developers
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

#include <vgc/core/api.h>
#include <vgc/core/arithmetic.h>
#include <vgc/core/format.h>
#include <vgc/core/parse.h>

namespace vgc {
namespace core {

/// \class vgc::core::Color
/// \brief Color + alpha represented as RBGA with double-precision floats.
///
class VGC_CORE_API Color
{
public:
    /// Creates an uninitialized Color.
    ///
    Color() {}

    /// Creates a Color initialized with the given r, g, b in [0, 1]. The alpha
    /// channel is set to 1.0.
    ///
    Color(double r, double g,  double b) : data_{r, g, b, 1.0} {}

    /// Creates a Color initialized with the given r, g, b, a in [0, 1].
    ///
    Color(double r, double g,  double b, double a) : data_{r, g, b, a} {}

    /// Accesses the i-th channel of the Color.
    ///
    const double& operator[](int i) const { return data_[i]; }

    /// Mutates the i-th channel of the Color.
    ///
    double& operator[](int i) { return data_[i]; }

    /// Accesses the red channel of the Color.
    ///
    double r() const { return data_[0]; }

    /// Accesses the green channel of the Color.
    ///
    double g() const { return data_[1]; }

    /// Accesses the blue channel of the Color.
    ///
    double b() const { return data_[2]; }

    /// Accesses the alpha channel of the Color.
    ///
    double a() const { return data_[3]; }

    /// Mutates the red channel of the Color.
    ///
    void setR(double r) { data_[0] = r; }

    /// Mutates the green channel of the Color.
    ///
    void setG(double g) { data_[1] = g; }

    /// Mutates the blue channel of the Color.
    ///
    void setB(double b) { data_[2] = b; }

    /// Mutates the alpha channel of the Color.
    ///
    void setA(double a) { data_[3] = a; }

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
    Color& operator*=(double s) {
        data_[0] *= s;
        data_[1] *= s;
        data_[2] *= s;
        data_[3] *= s;
        return *this;
    }

    /// Returns the multiplication of this Color by the given scalar \p s.
    /// This multiplies all channels, including the alpha channel.
    ///
    Color operator*(double s) const {
        return Color(*this) *= s;
    }

    /// Returns the multiplication of the scalar \p s with the Color \p c.
    /// This multiplies all channels, including the alpha channel.
    ///
    friend Color operator*(double s, const Color& c) {
        return c * s;
    }

    /// Divides in-place this Color by the given scalar \p s.
    /// This divides all channels, including the alpha channel.
    ///
    Color& operator/=(double s) {
        data_[0] /= s;
        data_[1] /= s;
        data_[2] /= s;
        data_[3] /= s;
        return *this;
    }

    /// Returns the division of this Color by the given scalar \p s.
    /// This divides all channels, including the alpha channel.
    ///
    Color operator/(double s) const {
        return Color(*this) /= s;
    }

    /// Returns whether the two given Color \p c1 and \p c2 are equal.
    ///
    friend bool operator==(const Color& c1, const Color& c2) {
        return c1.data_[0] == c2.data_[0] &&
               c1.data_[1] == c2.data_[1] &&
               c1.data_[2] == c2.data_[2] &&
               c1.data_[3] == c2.data_[3];
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
        return ( (c1.data_[0] < c2.data_[0]) ||
               (!(c2.data_[0] < c1.data_[0]) &&
               ( (c1.data_[1] < c2.data_[1]) ||
               (!(c2.data_[1] < c1.data_[1]) &&
               ( (c1.data_[2] < c2.data_[2]) ||
               (!(c2.data_[2] < c1.data_[2]) &&
               ( (c1.data_[3] < c2.data_[3]))))))));
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

private:
    double data_[4];
};

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
void write(OStream& out, const Color& c)
{
    bool writeAlpha = (c.a() != 1.0);
    write(out, writeAlpha ? "rgba(" : "rgb(");
    write(out, double01ToUint8(c.r()), ", ",
               double01ToUint8(c.g()), ", ",
               double01ToUint8(c.b()));
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
template <typename IStream>
void readTo(Color& c, IStream& in)
{
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
    c[0] = uint8ToDouble01(read<Int>(in));
    skipWhitespaceCharacters(in);
    skipExpectedCharacter(in, ',');
    c[1] = uint8ToDouble01(read<Int>(in));
    skipWhitespaceCharacters(in);
    skipExpectedCharacter(in, ',');
    c[2] = uint8ToDouble01(read<Int>(in));
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

} // namespace core
} // namespace vgc

#endif // VGC_CORE_COLOR_H
