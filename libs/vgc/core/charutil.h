// Copyright 2018 The VGC Developers
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

#ifndef VGC_CORE_CHARUTIL_H
#define VGC_CORE_CHARUTIL_H

#include <string>

#include <vgc/core/api.h>
#include <vgc/core/exceptions.h>

namespace vgc {
namespace core {

/// Returns whether the given char is a whitespace character, that is, ' ',
/// '\n', '\r', or '\t'.
///
VGC_CORE_API
inline bool isWhitespace(char c)
{
    return c == ' ' || c == '\n' || c == '\r' || c == '\t';
}

/// Returns whether the given char is a digit character, that is, '0'-'9'.
///
VGC_CORE_API
inline bool isDigit(char c)
{
   return ('0' <= c && c <= '9' );
}

/// Returns the double represented by the given digit character \p c, assuming
/// that \p c is indeed a digit (that is, isDigit(c) must return true).
/// Otherwise, the behaviour is undefined.
///
VGC_CORE_API
inline double digitToDoubleNoRangeCheck(char c) {
    const double t[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
    return t[c - '0'];
}

/// Returns the int represented by the given digit character \p c, assuming
/// that \p c is indeed a digit (that is, isDigit(c) must return true).
/// Otherwise, the behaviour is undefined.
///
VGC_CORE_API
inline int digitToIntNoRangeCheck(char c) {
    return static_cast<int>(c - '0');
}

/// Returns the double represented by the given digit character \p c. If \p c
/// is not a digit (that is, isDigit(c) returns false), then ParseError is
/// raised.
///
VGC_CORE_API
inline double digitToDouble(char c) {
    if (isDigit(c)) {
        return digitToDoubleNoRangeCheck(c);
    }
    else {
        throw ParseError(
            std::string("Unexpected '") + c + "'. Expected a digit [0-9].");
    }
}

/// Returns the int represented by the given digit character \p c. If \p c
/// is not a digit (that is, isDigit(c) returns false), then ParseError is
/// raised.
///
VGC_CORE_API
inline int digitToInt(char c) {
    if (isDigit(c)) {
        return digitToIntNoRangeCheck(c);
    }
    else {
        throw ParseError(
            std::string("Unexpected '") + c + "'. Expected a digit [0-9].");
    }
}

} // namespace core
} // namespace vgc

#endif // VGC_CORE_CHARUTIL_H
