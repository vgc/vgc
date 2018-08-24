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

#ifndef VGC_CORE_IO_H
#define VGC_CORE_IO_H

#include <cmath>
#include <sstream>
#include <string>
#include <vgc/core/api.h>
#include <vgc/core/exceptions.h>

namespace vgc {
namespace core {

/// Returns as a std::string the content of the file given by its \p filePath.
///
VGC_CORE_API
std::string readFile(const std::string& filePath);

/// Returns whether the given char is a whitespace character, that is, ' ',
/// '\n', '\r', or '\t'.
///
bool isWhitespace(char c)
{
    return c == ' ' || c == '\n' || c == '\r' || c == '\t';
}

/// Returns whether the given char is a digit character, that is, '0'-'9'.
///
bool isDigit(char c)
{
   return ('0' <= c && c <= '9' );
}

/// Extracts characters from the input stream one by one until a non-whitespace
/// character is extracted, and returns this non-whitespace character. Raises
/// ParseError if the stream ends before a non-whitespace character is found.
///
template <typename IStream>
char getNonWhitespaceCharacter(IStream& in)
{
    bool found = false;
    char c = -1; // dummy-initialization in order to silence warning
    while (!found && in.get(c)) {
        if (!isWhitespace(c)) {
            found = true;
        }
    }
    if (!found) {
        throw ParseError(
            "Unexpected end of stream while searching for a non-whitespace "
            "character. Expected either a whitespace character (to be "
            " skipped), or a non-whitespace character (to be returned).");
    }
    return c;
}

namespace impl_
{

// Returns the double represented by the given digit character.
// Assumes that isDigit(c) returns true.
double toDouble(char c) {
    const double t[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
    return t[c - '0'];
}

} // namespace impl_

/// Reads a number from the input stream \p in and stores it as a double in the
/// given output argument \p x. Raises ParseError if the input stream cannot be
/// interpreted as a number.
///
/// The only accepted numbers are those following the JSON number format, with
/// the extension that leading zeros are accepted:
///
/// number ::= ("-")? [0-9]+ ("." [0-9]+)? ([eE][+-]?[0-9]+)?
///
/// Note that unlike std::atof and std::istream::operator>>, this function is
/// locale-independent: the decimal point is always assumed to be ".".
///
/// Leading whitespaces are ignored.
///
template <typename IStream>
void read(IStream& in, double& x)
{
    char c = getNonWhitespaceCharacter(in);

    bool isPositive = true;
    if (c == '-') {
        isPositive = false;
        if (!in.get(c)) {
            throw ParseError(
                "Unexpected end of stream while attempting to read the "
                "character following the minus sign of a number. Expected a "
                "digit (0-9).");
        }
    }

    // Read integer part
    if (!isDigit(c)) {
        throw ParseError(
            std::string("Unexpected '") + c + "' while reading the first "
            "character of number (after potential minus sign). Expected "
            "a digit (0-9).");
    }
    x = impl_::toDouble(c);
    bool nonDigitCharacterFound = false;
    while (!nonDigitCharacterFound && in.get(c)) {
        if (isDigit(c)) {
            x *= 10;
            x += impl_::toDouble(c);
        }
        else {
            nonDigitCharacterFound = true;
        }
    }
    if (!nonDigitCharacterFound) {
        // This is the end of the stream, a valid number was read
        x *= isPositive ? 1.0 : -1.0;
        return;
    }

    // Read fractional part
    if (c == '.') {
        // Note: incrementally multiplying by 0.1 as we do here generates
        // floating point rounding errors. In particular, the expression (0.01
        // == 0.1 * 0.1) evaluates to false, thus readDouble("0.01") != 0.01.
        //
        // A better approach would be to store 0.1, 0.01, etc. in a table, and
        // only multiply once by the relevant multiplier: 12.453 = 12453 *
        // 0.001.
        //
        // An even better approach might be to implement:
        //   How to Read Floating Point Numbers Accurately, William D Clinger
        //   http://citeseerx.ist.psu.edu/viewdoc/download?doi=10.1.1.45.4152&rep=rep1&type=pdf
        //
        // Relevant:
        //   - How Rust does it: https://github.com/rust-lang/rust/pull/27307
        //   - How gcc (used to) do it: https://www.exploringbinary.com/how-gcc-converts-decimal-literals-to-floating-point/
        //
        // More simply, I might just want to use:
        //
        //     https://en.cppreference.com/w/cpp/string/byte/strtof
        //
        // if performance seems okay. The rationale to roll up our own
        // implementation was that in our use case, extreme accuracy is not
        // required,  we can restrict ourselves to JSON syntax,
        // and we don't care about the locale. This makes room for better
        // performance, and I expect that decimal to double conversion is
        // what takes most of the time to open a file. Benchmark will follow.
        //
        double multiplier = 0.1;
        if (!in.get(c)) {
            throw ParseError(
                "Unexpected end of stream while attempting to read the "
                "character following the decimal point of a number. Expected "
                "a digit (0-9).");
        }
        if (!isDigit(c)) {
            throw ParseError(
                std::string("Unexpected '") + c + "' while reading the "
                "character following the decimal point of a number. Expected "
                "a digit (0-9).");
        }
        x += impl_::toDouble(c) * multiplier;
        bool nonDigitCharacterFound = false;
        while (!nonDigitCharacterFound && in.get(c)) {
            if (isDigit(c)) {
                multiplier *= 0.1;
                x += impl_::toDouble(c) * multiplier;
            }
            else {
                nonDigitCharacterFound = true;
            }
        }
        if (!nonDigitCharacterFound) {
            // This is the end of the stream, a valid number was read
            x *= isPositive ? 1.0 : -1.0;
            return;
        }
    }

    // Read exponent part
    if (c == 'e' || c == 'E') {
        if (!in.get(c)) {
            throw ParseError(
                "Unexpected end of stream while attempting to read the "
                "character following the exponent symbol of a number. "
                "Expected a digit (0-9), or '+', or '-'.");
        }
        bool isExponentPositive = true;
        if (c == '-' || c == '+') {
            if (c == '-') {
                isExponentPositive = false;
            }
            if (!in.get(c)) {
                throw ParseError(
                    "Unexpected end of stream while attempting to read the "
                    "character following the sign after the exponent symbol "
                    "of a number. Expected a digit (0-9).");
            }
        }
        if (!isDigit(c)) {
            throw ParseError(
                std::string("Unexpected '") + c + "' while reading the "
                "character following the exponent symbol (and possibly a +/- "
                "sign) of a number. Expected a digit (0-9).");
        }
        double exponent = impl_::toDouble(c);
        bool nonDigitCharacterFound = false;
        while (!nonDigitCharacterFound && in.get(c)) {
            if (isDigit(c)) {
                exponent *= 10;
                exponent += impl_::toDouble(c);
            }
            else {
                nonDigitCharacterFound = true;
            }
        }
        exponent *= isExponentPositive ? 1.0 : -1.0;
        x *= std::pow(10, exponent);
        if (!nonDigitCharacterFound) {
            // This is the end of the stream, a valid number was read
            x *= isPositive ? 1.0 : -1.0;
            return;
        }
    }

    // Set sign bit
    x *= isPositive ? 1.0 : -1.0;

    // Un-extract the last character read, which is not part of the number.
    in.unget();
}

/// \overload read(IStream& in, double& x)
///
double readDouble(const std::string& s)
{
    double res;
    std::stringstream in(s);
    read(in, res);
    return res;

    // XXX TODO: Use a custom StringViewStream class instead of
    // std::stringstream to avoid copying the data.

    // TODO: implement versions taking as input a custom StringView class (or
    // C++17 string_view)
}

} // namespace core
} // namespace vgc

#endif // VGC_CORE_IO_H
