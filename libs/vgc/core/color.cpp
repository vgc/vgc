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

#include <vgc/core/color.h>

namespace vgc::core {

namespace {

template<typename Float>
std::array<Float, 3> rgbFromHsl(Float h, Float s, Float l) {
    // Wrap h to [0, 360] range, and clamp s, l to [0, 1]
    h = std::fmod(h, static_cast<Float>(360));
    if (h < 0) {
        h += 360;
    }
    s = core::clamp(s, 0, 1);
    l = core::clamp(l, 0, 1);

    // HSL to RGB
    Float c = (1 - std::abs(2 * l - 1)) * s;
    Float hp = h / 60;
    Float x = c * (1 - std::abs(std::fmod(hp, static_cast<Float>(2)) - 1));
    int hi = core::ifloor<int>(hp + 1); // in theory, we should use iceil instead
    Float r1, g1, b1;
    if (hi == 1) {
        r1 = c;
        g1 = x;
        b1 = 0;
    }
    else if (hi == 2) {
        r1 = x;
        g1 = c;
        b1 = 0;
    }
    else if (hi == 3) {
        r1 = 0;
        g1 = c;
        b1 = x;
    }
    else if (hi == 4) {
        r1 = 0;
        g1 = x;
        b1 = c;
    }
    else if (hi == 5) {
        r1 = x;
        g1 = 0;
        b1 = c;
    }
    else if (hi == 6) {
        r1 = c;
        g1 = 0;
        b1 = x;
    }
    else {
        r1 = 0;
        g1 = 0;
        b1 = 0;
    }
    Float m = l - c / 2;
    return {r1 + m, g1 + m, b1 + m};
}

enum class Channel : UInt8 {
    Red,
    Green,
    Blue
};

template<typename Float>
std::array<Float, 3> hslFromRgb(Float r, Float g, Float b) {
    r = core::clamp(r, 0, 1);
    g = core::clamp(g, 0, 1);
    b = core::clamp(b, 0, 1);
    Float min;
    Float max;
    Channel maxChannel;
    if (r < g) {
        if (g < b) { // r < g < b
            min = r;
            max = b;
            maxChannel = Channel::Blue;
        }
        else if (r < b) { // r < b < g
            min = r;
            max = g;
            maxChannel = Channel::Green;
        }
        else { // b < r < g
            min = b;
            max = g;
            maxChannel = Channel::Green;
        }
    }
    else {           // g < r
        if (r < b) { // g < r < b
            min = g;
            max = b;
            maxChannel = Channel::Blue;
        }
        else if (g < b) { // g < b < r
            min = g;
            max = r;
            maxChannel = Channel::Red;
        }
        else { // b < g < r
            min = b;
            max = r;
            maxChannel = Channel::Red;
        }
    }
    Float c = max - min;
    Float h = 0;
    if (c > 0) {
        switch (maxChannel) {
        case Channel::Red:
            h = (g - b) / c;
            if (h < 0) {
                h += 6;
            }
            break;
        case Channel::Green:
            h = (b - r) / c + 2;
            break;
        case Channel::Blue:
            h = (r - g) / c + 4;
            break;
        }
        h *= 60;
    }
    Float l = (min + max) / 2;
    Float s = 0;
    if (0 < l && l < 1) {
        s = c / (1 - std::abs(2 * l - 1));
    }
    return {h, s, l};
}

template<typename Float>
Float round8b_(Float x) {
    x = std::round(x * 255) / 255;
    return core::clamp(x, 0, 1);
}

UInt8 hexByteToUint8(char d) {
    constexpr UInt8 c0 = static_cast<UInt8>('0');
    constexpr UInt8 c9 = static_cast<UInt8>('9');
    constexpr UInt8 ca = static_cast<UInt8>('a');
    constexpr UInt8 cf = static_cast<UInt8>('f');
    constexpr UInt8 cA = static_cast<UInt8>('A');
    constexpr UInt8 cF = static_cast<UInt8>('F');
    constexpr UInt8 i10 = static_cast<UInt8>(10);
    UInt8 c = static_cast<UInt8>(d);

    if (c >= c0 && c <= c9) {
        return c - c0;
    }
    else if (c >= ca && c <= cf) {
        return i10 + c - ca;
    }
    else if (c >= cA && c <= cF) {
        return i10 + c - cA;
    }
    else {
        throw core::ParseError(core::format("Invalid hexadecimal digit: '{}'.", d));
    }
}

} // namespace

Color Color::hsl(double h, double s, double l) {
    auto [r, g, b] = rgbFromHsl(h, s, l);
    return Color(r, g, b);
}

Color Color::fromHex(std::string_view hex) {
    size_t s = hex.size();
    if (!(s == 4 || s == 7) || hex[0] != '#') {
        throw core::ParseError(core::format("Invalid hexadecimal color: \"{}\".", hex));
    }
    UInt8 r0, g0, b0;
    UInt8 r1, g1, b1;
    if (s == 4) {
        r0 = hexByteToUint8(hex[1]);
        r1 = r0;
        g0 = hexByteToUint8(hex[2]);
        g1 = g0;
        b0 = hexByteToUint8(hex[3]);
        b1 = b0;
    }
    else { // s == 7
        r0 = hexByteToUint8(hex[1]);
        r1 = hexByteToUint8(hex[2]);
        g0 = hexByteToUint8(hex[3]);
        g1 = hexByteToUint8(hex[4]);
        b0 = hexByteToUint8(hex[5]);
        b1 = hexByteToUint8(hex[6]);
    }
    double r = core::uint8ToDouble01(16 * r0 + r1);
    double g = core::uint8ToDouble01(16 * g0 + g1);
    double b = core::uint8ToDouble01(16 * b0 + b1);
    return Color(r, g, b);
}

std::array<double, 3> Color::toHsl() const {
    return hslFromRgb(r(), g(), b());
}

std::string Color::toHex() const {
    UInt8 r8 = core::double01ToUint8(r());
    UInt8 g8 = core::double01ToUint8(g());
    UInt8 b8 = core::double01ToUint8(b());
    std::string res;
    res.reserve(7);
    res.append("#");
    res.append(core::toHexPair(r8));
    res.append(core::toHexPair(g8));
    res.append(core::toHexPair(b8));
    return res;
}

Color& Color::round8b() {
    data_[0] = round8b_(data_[0]);
    data_[1] = round8b_(data_[1]);
    data_[2] = round8b_(data_[2]);
    data_[3] = round8b_(data_[3]);
    return *this;
}

Colorf Colorf::hsl(float h, float s, float l) {
    auto [r, g, b] = rgbFromHsl(h, s, l);
    return Colorf(r, g, b);
}

std::array<float, 3> Colorf::toHsl() const {
    return hslFromRgb(r(), g(), b());
}

Colorf& Colorf::round8b() {
    data_[0] = round8b_(data_[0]);
    data_[1] = round8b_(data_[1]);
    data_[2] = round8b_(data_[2]);
    data_[3] = round8b_(data_[3]);
    return *this;
}

} // namespace vgc::core

// Note 1: In the future, we'd like to have the class "Color" be more flexible.
// Notably, there should be control on:
//
//   1. How is the color stored in memory (C++ representation)
//   2. How is it converted to a string (XML representation)
//
// Examples of orthogonal choices for #1 include rgb/hls, int/float, 8/16/32/64
// bits per channel, premultiplied or not, etc.
//
// Examples of orthogonal choices for #2 include all the above + whether to
// omit the alpha channel if fully-opaque, whether to use #rrggbbaa notation or
// rgb(r, g, b), or rgb8(r, g, b), etc.
//
// In addition to the per-Color formatting style, there could be a
// per-file/per-session/per-user preferred format (the one used by default),
// but allowing a per-color formatting style is important, since within the
// same file the user might prefer to use hex notation for some colors, and
// functional notation for others. It's especially useful to preserve existing
// format when opening a file initially written by hand. New colors should have
// color.stringFormat() == Color::FileDefault, where the per-file default
// format for colors can be inferred when reading the file. For new files, or
// files where the FileDefault can't be inferred (or only partially inferred),
// then "FileDefault" should be determined via user preference.

// Note 2: Currently, for simplicity and genericity, the Color class is
// actually quite heavy: 4x64bits (and later: + metadata). This is okay for
// single colors, but would make a large vector<Color> very inefficient. This
// is why the type ColorArray shouldn't just be a vector<Color>, but instead a
// vector<char> + metadata, so that the metadata is shared, and only the
// required number of bytes is used. In fact, even the Color class might be a
// vector<char>, that is, use dynamic allocation to save memory. It isn't clear
// at the moment whether it's better to have a heavy Color class, or a Color
// class doing dynamic allocation to save memory.

// Note 3: ideally, we think it makes more sense if the alpha channel was
// formatted as a 8bit integer by default (like the other color channels are),
// instead of a floating point between [0, 1]. However, we decided to stick to
// the existing SVG and CSS standards:
//
//   https://developer.mozilla.org/en-US/docs/Web/CSS/CSS_Types
//
// For now, we always print as rgb(int8, int8, int8, float), but in the future
// we'd like to have the following the bring back sanity and flexibility in
// a backward-compatible manner:
//
//     - rgb(int8, int8, int8 [, double])
//     - rgb8(int8, int8, int8 [, int8])
//     - rgb16(int16, int16, int16 [, int16])
//     - rgb32(int32, int32, int32 [, int32])
//     - rgb64(int64, int64, int64 [, int64])
//     - rgb32f(int32, int32, int32 [, int32])
//     - rgb64f(int64, int64, int64 [, int64])
//
// Color arrays may be stored as:
//
//    - rbg[(r, g, b), ...]
//    - rbg8[(r, g, b), ...]
//    - ...
//
// Sometimes, we may want to have per-item alpha values, and sometimes a shared
// alpha values, we haven't decided yet the best way to allow both, although
// a separate "opacity" attribute is probably enough in most use cases.

// Note 4: when the color is read as rgb(int8, int8, int8, double), the alpha
// channel should actually simply be stored as a 8-bit integer, especially in
// a large ColorArray. This is because if each color channel is 8-bit anyway,
// there's probably no need for more precision than 8-bit for the alpha channel
// either, since the result of alpha-compositing would clamp to 8-bit anyway.
