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

using NamedColorMap = std::unordered_map<std::string_view, Color>;

NamedColorMap generateNamedColorMap() {
    return {
        {"aliceblue", Color::fromRgb8(240, 248, 255)},
        {"antiquewhite", Color::fromRgb8(250, 235, 215)},
        {"aqua", Color::fromRgb8(0, 255, 255)},
        {"aquamarine", Color::fromRgb8(127, 255, 212)},
        {"azure", Color::fromRgb8(240, 255, 255)},
        {"beige", Color::fromRgb8(245, 245, 220)},
        {"bisque", Color::fromRgb8(255, 228, 196)},
        {"black", Color::fromRgb8(0, 0, 0)},
        {"blanchedalmond", Color::fromRgb8(255, 235, 205)},
        {"blue", Color::fromRgb8(0, 0, 255)},
        {"blueviolet", Color::fromRgb8(138, 43, 226)},
        {"brown", Color::fromRgb8(165, 42, 42)},
        {"burlywood", Color::fromRgb8(222, 184, 135)},
        {"cadetblue", Color::fromRgb8(95, 158, 160)},
        {"chartreuse", Color::fromRgb8(127, 255, 0)},
        {"chocolate", Color::fromRgb8(210, 105, 30)},
        {"coral", Color::fromRgb8(255, 127, 80)},
        {"cornflowerblue", Color::fromRgb8(100, 149, 237)},
        {"cornsilk", Color::fromRgb8(255, 248, 220)},
        {"crimson", Color::fromRgb8(220, 20, 60)},
        {"cyan", Color::fromRgb8(0, 255, 255)},
        {"darkblue", Color::fromRgb8(0, 0, 139)},
        {"darkcyan", Color::fromRgb8(0, 139, 139)},
        {"darkgoldenrod", Color::fromRgb8(184, 134, 11)},
        {"darkgray", Color::fromRgb8(169, 169, 169)},
        {"darkgreen", Color::fromRgb8(0, 100, 0)},
        {"darkgrey", Color::fromRgb8(169, 169, 169)},
        {"darkkhaki", Color::fromRgb8(189, 183, 107)},
        {"darkmagenta", Color::fromRgb8(139, 0, 139)},
        {"darkolivegreen", Color::fromRgb8(85, 107, 47)},
        {"darkorange", Color::fromRgb8(255, 140, 0)},
        {"darkorchid", Color::fromRgb8(153, 50, 204)},
        {"darkred", Color::fromRgb8(139, 0, 0)},
        {"darksalmon", Color::fromRgb8(233, 150, 122)},
        {"darkseagreen", Color::fromRgb8(143, 188, 143)},
        {"darkslateblue", Color::fromRgb8(72, 61, 139)},
        {"darkslategray", Color::fromRgb8(47, 79, 79)},
        {"darkslategrey", Color::fromRgb8(47, 79, 79)},
        {"darkturquoise", Color::fromRgb8(0, 206, 209)},
        {"darkviolet", Color::fromRgb8(148, 0, 211)},
        {"deeppink", Color::fromRgb8(255, 20, 147)},
        {"deepskyblue", Color::fromRgb8(0, 191, 255)},
        {"dimgray", Color::fromRgb8(105, 105, 105)},
        {"dimgrey", Color::fromRgb8(105, 105, 105)},
        {"dodgerblue", Color::fromRgb8(30, 144, 255)},
        {"firebrick", Color::fromRgb8(178, 34, 34)},
        {"floralwhite", Color::fromRgb8(255, 250, 240)},
        {"forestgreen", Color::fromRgb8(34, 139, 34)},
        {"fuchsia", Color::fromRgb8(255, 0, 255)},
        {"gainsboro", Color::fromRgb8(220, 220, 220)},
        {"ghostwhite", Color::fromRgb8(248, 248, 255)},
        {"gold", Color::fromRgb8(255, 215, 0)},
        {"goldenrod", Color::fromRgb8(218, 165, 32)},
        {"gray", Color::fromRgb8(128, 128, 128)},
        {"grey", Color::fromRgb8(128, 128, 128)},
        {"green", Color::fromRgb8(0, 128, 0)},
        {"greenyellow", Color::fromRgb8(173, 255, 47)},
        {"honeydew", Color::fromRgb8(240, 255, 240)},
        {"hotpink", Color::fromRgb8(255, 105, 180)},
        {"indianred", Color::fromRgb8(205, 92, 92)},
        {"indigo", Color::fromRgb8(75, 0, 130)},
        {"ivory", Color::fromRgb8(255, 255, 240)},
        {"khaki", Color::fromRgb8(240, 230, 140)},
        {"lavender", Color::fromRgb8(230, 230, 250)},
        {"lavenderblush", Color::fromRgb8(255, 240, 245)},
        {"lawngreen", Color::fromRgb8(124, 252, 0)},
        {"lemonchiffon", Color::fromRgb8(255, 250, 205)},
        {"lightblue", Color::fromRgb8(173, 216, 230)},
        {"lightcoral", Color::fromRgb8(240, 128, 128)},
        {"lightcyan", Color::fromRgb8(224, 255, 255)},
        {"lightgoldenrodyellow", Color::fromRgb8(250, 250, 210)},
        {"lightgray", Color::fromRgb8(211, 211, 211)},
        {"lightgreen", Color::fromRgb8(144, 238, 144)},
        {"lightgrey", Color::fromRgb8(211, 211, 211)},
        {"lightpink", Color::fromRgb8(255, 182, 193)},
        {"lightsalmon", Color::fromRgb8(255, 160, 122)},
        {"lightseagreen", Color::fromRgb8(32, 178, 170)},
        {"lightskyblue", Color::fromRgb8(135, 206, 250)},
        {"lightslategray", Color::fromRgb8(119, 136, 153)},
        {"lightslategrey", Color::fromRgb8(119, 136, 153)},
        {"lightsteelblue", Color::fromRgb8(176, 196, 222)},
        {"lightyellow", Color::fromRgb8(255, 255, 224)},
        {"lime", Color::fromRgb8(0, 255, 0)},
        {"limegreen", Color::fromRgb8(50, 205, 50)},
        {"linen", Color::fromRgb8(250, 240, 230)},
        {"magenta", Color::fromRgb8(255, 0, 255)},
        {"maroon", Color::fromRgb8(128, 0, 0)},
        {"mediumaquamarine", Color::fromRgb8(102, 205, 170)},
        {"mediumblue", Color::fromRgb8(0, 0, 205)},
        {"mediumorchid", Color::fromRgb8(186, 85, 211)},
        {"mediumpurple", Color::fromRgb8(147, 112, 219)},
        {"mediumseagreen", Color::fromRgb8(60, 179, 113)},
        {"mediumslateblue", Color::fromRgb8(123, 104, 238)},
        {"mediumspringgreen", Color::fromRgb8(0, 250, 154)},
        {"mediumturquoise", Color::fromRgb8(72, 209, 204)},
        {"mediumvioletred", Color::fromRgb8(199, 21, 133)},
        {"midnightblue", Color::fromRgb8(25, 25, 112)},
        {"mintcream", Color::fromRgb8(245, 255, 250)},
        {"mistyrose", Color::fromRgb8(255, 228, 225)},
        {"moccasin", Color::fromRgb8(255, 228, 181)},
        {"navajowhite", Color::fromRgb8(255, 222, 173)},
        {"navy", Color::fromRgb8(0, 0, 128)},
        {"oldlace", Color::fromRgb8(253, 245, 230)},
        {"olive", Color::fromRgb8(128, 128, 0)},
        {"olivedrab", Color::fromRgb8(107, 142, 35)},
        {"orange", Color::fromRgb8(255, 165, 0)},
        {"orangered", Color::fromRgb8(255, 69, 0)},
        {"orchid", Color::fromRgb8(218, 112, 214)},
        {"palegoldenrod", Color::fromRgb8(238, 232, 170)},
        {"palegreen", Color::fromRgb8(152, 251, 152)},
        {"paleturquoise", Color::fromRgb8(175, 238, 238)},
        {"palevioletred", Color::fromRgb8(219, 112, 147)},
        {"papayawhip", Color::fromRgb8(255, 239, 213)},
        {"peachpuff", Color::fromRgb8(255, 218, 185)},
        {"peru", Color::fromRgb8(205, 133, 63)},
        {"pink", Color::fromRgb8(255, 192, 203)},
        {"plum", Color::fromRgb8(221, 160, 221)},
        {"powderblue", Color::fromRgb8(176, 224, 230)},
        {"purple", Color::fromRgb8(128, 0, 128)},
        {"red", Color::fromRgb8(255, 0, 0)},
        {"rosybrown", Color::fromRgb8(188, 143, 143)},
        {"royalblue", Color::fromRgb8(65, 105, 225)},
        {"saddlebrown", Color::fromRgb8(139, 69, 19)},
        {"salmon", Color::fromRgb8(250, 128, 114)},
        {"sandybrown", Color::fromRgb8(244, 164, 96)},
        {"seagreen", Color::fromRgb8(46, 139, 87)},
        {"seashell", Color::fromRgb8(255, 245, 238)},
        {"sienna", Color::fromRgb8(160, 82, 45)},
        {"silver", Color::fromRgb8(192, 192, 192)},
        {"skyblue", Color::fromRgb8(135, 206, 235)},
        {"slateblue", Color::fromRgb8(106, 90, 205)},
        {"slategray", Color::fromRgb8(112, 128, 144)},
        {"slategrey", Color::fromRgb8(112, 128, 144)},
        {"snow", Color::fromRgb8(255, 250, 250)},
        {"springgreen", Color::fromRgb8(0, 255, 127)},
        {"steelblue", Color::fromRgb8(70, 130, 180)},
        {"tan", Color::fromRgb8(210, 180, 140)},
        {"teal", Color::fromRgb8(0, 128, 128)},
        {"thistle", Color::fromRgb8(216, 191, 216)},
        {"tomato", Color::fromRgb8(255, 99, 71)},
        {"turquoise", Color::fromRgb8(64, 224, 208)},
        {"violet", Color::fromRgb8(238, 130, 238)},
        {"wheat", Color::fromRgb8(245, 222, 179)},
        {"white", Color::fromRgb8(255, 255, 255)},
        {"whitesmoke", Color::fromRgb8(245, 245, 245)},
        {"yellow", Color::fromRgb8(255, 255, 0)},
        {"yellowgreen", Color::fromRgb8(154, 205, 50)}};
}

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

Color Color::hsl(float h, float s, float l) {
    auto [r, g, b] = rgbFromHsl(h, s, l);
    return Color(r, g, b);
}

Color Color::hsla(float h, float s, float l, float a) {
    auto [r, g, b] = rgbFromHsl(h, s, l);
    return Color(r, g, b, a);
}

std::array<float, 3> Color::toHsl() const {
    return hslFromRgb(r(), g(), b());
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
    float r = mapFromUInt8(16 * r0 + r1);
    float g = mapFromUInt8(16 * g0 + g1);
    float b = mapFromUInt8(16 * b0 + b1);
    return Color(r, g, b);
}

Color Color::fromName(std::string_view name) {
    static NamedColorMap map = generateNamedColorMap();
    auto it = map.find(name);
    if (it != map.end()) {
        return it->second;
    }
    else {
        throw core::ParseError(core::format("Invalid color name: '{}'.", name));
    }
}

std::string Color::toHex() const {
    UInt8 r8 = mapToUInt8(r());
    UInt8 g8 = mapToUInt8(g());
    UInt8 b8 = mapToUInt8(b());
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

// Note 2: ideally, we think it makes more sense if the alpha channel was
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

// Note 3: when the color is read as rgb(int8, int8, int8, double), the alpha
// channel should actually simply be stored as a 8-bit integer, especially in
// a large ColorArray. This is because if each color channel is 8-bit anyway,
// there's probably no need for more precision than 8-bit for the alpha channel
// either, since the result of alpha-compositing would clamp to 8-bit anyway.
