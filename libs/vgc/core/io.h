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
#include <vgc/core/stringutil.h>

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
char readNonWhitespaceCharacter(IStream& in)
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

/// Returns the double represented by the given digit character \p c, assuming
/// that \p c is indeed a digit (that is, isDigit(c) must return true).
/// Otherwise, the behaviour is undefined.
///
double digitToDoubleNoRangeCheck(char c) {
    const double t[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
    return t[c - '0'];
}

/// Returns the int represented by the given digit character \p c, assuming
/// that \p c is indeed a digit (that is, isDigit(c) must return true).
/// Otherwise, the behaviour is undefined.
///
int digitToIntNoRangeCheck(char c) {
    return static_cast<int>(c - '0');
}

/// Returns the double represented by the given digit character \p c. If \p c
/// is not a digit (that is, isDigit(c) returns false), then ParseError is
/// raised.
///
double digitToDouble(char c) {
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
int digitToInt(char c) {
    if (isDigit(c)) {
        return digitToIntNoRangeCheck(c);
    }
    else {
        throw ParseError(
            std::string("Unexpected '") + c + "'. Expected a digit [0-9].");
    }
}

namespace impl_
{

// Computes (-1)^s * a * 10^b, where a must be a double representing an integer
// with n digits. This latter argument is used to guard against underflow and
// overflow.
double makeDouble(bool isPositive, double a, int b, int n)
{
    if (b+n-1 > 307) {
        throw RangeError(
            std::string("The number ") + (isPositive ? "" : "-") + toString(a) +
            "e" + toString(b) + " is too big to be represented as a double.");
    }

    if (b+n-1 < -307) {
        return isPositive ? 0.0 : -0.0;
    }

    if (b < -250) {
        // Avoid subnormal numbers by keeping a large margin.
        a *= std::pow(10.0, -20);
        b += 20;
    }

    return isPositive ? a * std::pow(10.0, b) : -a * std::pow(10.0, b);

    // TODO use precomputed powers of tens for better performance and higher accuracy.
}

// Computes (-1)^s * a
double makeDouble(bool isPositive, double a)
{
    return isPositive ? a : -a;
}

} // namespace impl_

/// Reads a base-10 text representation of a number from the input stream \p
/// in, and converts it approximately to a double stored in \p x, with a
/// guaranteed precision of 15 significant digits. This is an optimization to
/// make the conversion from base-10 to base-2 faster when accuracy above 15
/// significant digits is not required. If in doubt, use readDouble() instead.
///
/// Leading whitespaces are allowed. the text representation must match the
/// following pattern:
///
/// \verbatim
/// [+-]? ( [0-9]+ | [0-9]+ '.' [0-9]* | [0-9]* '.' [0-9]+ ) ([eE][+-]?[0-9]+)?
/// \endverbatim
///
/// Examples of valid input:
///
/// \verbatim
/// 0               =  0.0
/// 1               =  1.0
/// 42              =  42.0
/// +42             =  42.0
/// -42             = -42.0
/// 4.2             =  4.2
/// 4.2e+1          =  42.0
/// 4.2e1           =  42.0
/// 4.2E+1          =  42.0
/// 0.42e+2         =  42.0
/// 420e-1          =  42.0
/// 004.200e+01     =  42.0
/// 0.0             =  0.0
/// .0              =  0.0
/// 0.              =  0.0
/// 1.0             =  1.0
/// 0.1             =  0.1
/// .1              =  0.1
/// -.1             = -0.1
/// 1.              =  1.0
/// \endverbatim
///
/// Examples of invalid input:
///
/// \verbatim
/// 0x123456
/// + 1.0
/// NaN
/// inf
/// .
/// \endverbatim
///
/// In other words, both decimal and scientific expressions are allowed, but
/// floating-point hex notations are not. Special values such as "NaN" or "inf"
/// are not allowed. A leading plus or minus sign is allowed. No space is
/// allowed between the sign and the first digit. Leading or trailing zeros are
/// allowed. It is allowed not to have any digit before or after the decimal
/// point, as long as there is least one digit in the significand. The exponent
/// symbol is optional, but when present, the exponent should contain at least
/// one digit.
///
/// If the text representation does not match the required pattern, then
/// ParseError is raised. In such cases, the stream is read up to (and
/// including) the first non-matching character, and the value of \p x is
/// unchanged.
///
/// If the text representation matches the pattern, then the longest matching
/// sequence is considered. The stream is read up to (but excluding) the first
/// non-matching character. If the absolute value of the number is greater or
/// equal to 10^308, then RangeError is raised. If the absolute value of the
/// number is smaller than 10^-307, then the returned value is 0.
///
/// Unlike most built-in C++ utilities performing similar tasks (e.g., sscanf,
/// strtod, atof, istream::operator>>, etc), this function does not depend on
/// locale, that is, the decimal point is always assumed to be ".". The spirit
/// and rationale for this function is similar to std::from_chars (introduced
/// in C++17), with similar rationale (see P0067R5). However, unlike
/// std::from_chars, this function throws an exception on error, and uses a
/// stream-based interface.
///
/// The input stream \p in can be of any type implementing the following
/// methods (same behaviour as std::istream):
///
/// \code
/// IStream& get(char& c);
/// IStream& unget();
/// explicit operator bool() const;
/// \endcode
///
template <typename IStream>
void readDoubleApprox(IStream& in, double& x)
{
    // Overview of the algorithm, with the example input "   -0012.3456e+2"
    //
    // 1. Skip whitespaces
    // 2. Read minus sign
    // 3. Compute x = 123456 via the following sequence of operations:
    //      x = 1
    //      x = (10*x) + 2
    //      x = (10*x) + 3
    //      x = (10*x) + 4
    //      x = (10*x) + 5
    //      x = (10*x) + 6
    //
    //    We ignore all digits after reading 17 significant digits.
    //
    //    We remember dotPosition = 4 and numDigits = 6
    //    Here are other examples of resulting (x, dotPosition, numDigits) based on input:
    //           "42"    -> (42,   0, 2)
    //           "42.0"  -> (420,  1, 3)   equivalent to (42, 0, 2) but we couldn't know in advance during parsing
    //           "42.01" -> (4201, 2, 4)
    //           "420"   -> (420,  0, 3)
    //           "4201"  -> (4201, 0, 4)
    //           "1.2345678901234567"    -> (12345678901234567, 16, 17)
    //           "0.12345678901234567"   -> (12345678901234567, 17, 17)
    //           "0.012345678901234567"  -> (12345678901234567, 18, 17)
    //           "0.0123456789012345678" -> (12345678901234567, 18, 17)   i.e., we ignore the last digit
    //           "12345678901234567"     -> (12345678901234567,  0, 17)
    //           "123456789012345678"    -> (12345678901234567, -1, 17)   i.e., we ignore the last digit
    //           "123456789012345670"    -> (12345678901234567, -1, 17)
    //           "1234567890123456700"   -> (12345678901234567, -2, 17)
    //
    //    Note: These integers are exactly representable by a double:
    //      - all integers        up to   9007199254740992 (<= this number has 16 digits)
    //      - all multiples of 2  up to  18014398509481984 (<= this number has 17 digits)
    //      - all multiples of 4  up to  36028797018963968 (<= this number has 17 digits)
    //      - all multiples of 8  up to  72057594037927936 (<= this number has 17 digits)
    //      - all multiples of 16 up to 144115188075855872 (<= this number has 18 digits)
    //
    //    So here is what happens:
    //    - Up to reading the 15th digit, the value hold by x is exact.
    //    - Once we read the 16th digit, there is a 95% chance that the value
    //    hold by x is exact. In the remaining 5% of cases, the value is off by 1.
    //    - Once we read the 17th digit, the multiplication by 10 causes the
    //    "off by 1" to become "off by 10", which in turns may become "off by
    //    18" after rounding. Rounding after the final addition brings the
    //    worst case to "off by 26".
    //
    //    We could alternatively do the computation exactly using a 64-bit
    //    integer, and then convert to a double, which would bring higher
    //    accuracy (worst case: off by 8) in the case of 17 digits. However,
    //    the accuracy is still the same if there are 16 digits or less, which
    //    is typical in our use case, so we prefer to stay using double and
    //    avoid the int64->double conversion at the end. This choice is
    //    especially efficient when the number of digits is small.
    //
    // 4. Read the exponent "+2".
    //    Set exponent = exponent - dotPosition.
    //    Ensure that both "exponent" and "exponent + numDigits" is within [-306..308]
    //    (Question: could we also consider [-325..-307] by allowing subnormals?)
    //
    // 5. Get 10^exp as a double from a precomputed table
    //
    // 6. Compute the multiplication between x and 10^exp
    //
    // Note: for accurate base-10 to base-2, see:
    //   - How to Read Floating Point Numbers Accurately, William D Clinger: http://citeseerx.ist.psu.edu/viewdoc/download?doi=10.1.1.45.4152&rep=rep1&type=pdf
    //   - How Rust does it: https://github.com/rust-lang/rust/pull/27307
    //   - How gcc (used to) do it: https://www.exploringbinary.com/how-gcc-converts-decimal-literals-to-floating-point/
    //   - Interesting blog post: http://www.ryanjuckett.com/programming/printing-floating-point-numbers/
    //   - dtoa.c by David Gay: http://www.netlib.org/fp/dtoa.c
    //      -> This is the implementation used by Python (modified, see: https://github.com/python/cpython/blob/master/Python/dtoa.c)
    //   - double-conversion: https://github.com/google/double-conversion
    //      -> This is the implementation used by the V8 JavaScript engine of Google Chrome

    // Skip leading whitespaces; get the first non-whitespace character
    char c = readNonWhitespaceCharacter(in);

    // Read sign
    bool isPositive = true;
    if (c == '-' || c == '+') {
        isPositive = (c == '+');
        if (!in.get(c)) {
            throw ParseError(
                "Unexpected end of stream while attempting to read the first "
                "character following the sign of a number. Expected a "
                "digit [0-9] or '.'.");
        }
    }

    // Read leading zeros
    bool hasLeadingZeros = false;
    while (c == '0') {
        hasLeadingZeros = true;
        if (!in.get(c)) {
            // End of stream; 0 or -0 was read, e.g., "00"
            x = impl_::makeDouble(isPositive, 0.0);
            return;
        }
    }

    // Read integer part
    int numDigits = 0;
    int dotPosition = 0;
    double a = 0;
    while (isDigit(c)) {
        if (numDigits < 17) {
            a *= 10;
            a += digitToDoubleNoRangeCheck(c);
            numDigits += 1;
        }
        else {
            dotPosition -= 1;
        }
        if (!in.get(c)) {
            // End of stream; a non-zero integer was read, e.g., "042"
            x = impl_::makeDouble(isPositive, a, -dotPosition, numDigits);
            return;
        }
    }

    // Read decimal point
    if (c == '.') {
        if (!in.get(c)) {
            if (numDigits > 0) {
                // End of stream; a non-zero integer was read, e.g., "042."
                x = impl_::makeDouble(isPositive, a, -dotPosition, numDigits);
                return;
            }
            else if (hasLeadingZeros)  {
                // End of stream; 0 or -0 was read, e.g.,  "00."
                x = impl_::makeDouble(isPositive, 0.0);
                return;
            }
            else {
                 // End of stream; we've only read "."
                throw ParseError(
                    "Unexpected end of stream while attempting to read the first "
                    "character following the decimal point of a number that has "
                    "no digits before its decimal point. Expected a digit [0-9].");
            }
        }
    }

    // Read leading zeros in fractional part (e.g., we've read so far "00." or ".")
    if (numDigits == 0) {
        while (c == '0') {
            hasLeadingZeros = true;
            dotPosition += 1;
            if (!in.get(c)) {
                // End of stream; 0 or -0 was read, e.g., "00.00" or ".00"
                x = impl_::makeDouble(isPositive, 0.0);
                return;
            }
        }
    }

    // Read fractional part (e.g., we've read so far "042." or "00.00")
    while (isDigit(c)) {
        if (numDigits < 17) {
            a *= 10;
            a += digitToDoubleNoRangeCheck(c);
            numDigits += 1;
            dotPosition += 1;
        }
        else {
            // nothing
        }
        if (!in.get(c)) {
            // End of stream; a non-zero integer was read, e.g., "042.0140"
            x = impl_::makeDouble(isPositive, a, -dotPosition, numDigits);
            return;
        }
    }

    // Check that the integer+fractional part has at least one digit
    if (numDigits == 0 && !hasLeadingZeros) {
        throw ParseError(
            std::string("Unexpected '") + c + "' in a number before any digit "
            "was read. Expected a digit [0-9], a sign [+-], or a decimal "
            "point '.'.");
    }

    // Read exponent part
    int exponent = 0;
    if (c == 'e' || c == 'E') {
        if (!in.get(c)) {
            throw ParseError(
                "Unexpected end of stream while attempting to read the first "
                "character following the exponent symbol of a number. "
                "Expected a digit [0-9], or a sign [+-].");
        }
        bool isExponentPositive = true;
        if (c == '-' || c == '+') {
            isExponentPositive = (c == '+');
            if (!in.get(c)) {
                throw ParseError(
                    "Unexpected end of stream while attempting to read the first "
                    "character following the sign of the exponent part "
                    "of a number. Expected a digit [0-9].");
            }
        }
        bool hasExponentDigits = false;
        while (isDigit(c)) {
            hasExponentDigits = true;
            if (numDigits > 0) {
                //^ Don't bother computing exponent if we already know that the
                //  output will be 0
                if (exponent - dotPosition + numDigits - 1 <= 307 ||
                    exponent - dotPosition + numDigits - 1 >= -307) {
                    //^ Don't bother computing further if we already know that
                    //  the output will overflow or underflow. But don't throw
                    //  just yet: we still want to advance the stream until the
                    //  end of the number.
                    exponent *= 10;
                    exponent += isExponentPositive ? digitToIntNoRangeCheck(c) : -digitToIntNoRangeCheck(c);
                }
            }
            if (!in.get(c)) {
                if (numDigits > 0) {
                    // End of stream; a non-zero number was read, e.g., "042.0140e050" or "042.0140e0"
                    x = impl_::makeDouble(isPositive, a, exponent - dotPosition, numDigits);
                    return;
                }
                else {
                    // End of stream; 0 or -0 was read, e.g., "00.e050"
                    x = impl_::makeDouble(isPositive, 0.0);
                    return;
                }
            }
        }
        if (!hasExponentDigits) {
            throw ParseError(
                std::string("Unexpected '") + c + " in the exponent part of "
                "a number before any digit of the exponent part was read. "
                "Expected a digit [0-9], or a sign [+-].");
        }
    }

    // Un-extract the last character read, which is not part of the number.
    in.unget();

    // Compute the result
    if (numDigits > 0) {
        // A non-zero number was read, e.g., "042.0140e050" or "042.0140e0"
        x = impl_::makeDouble(isPositive, a, exponent - dotPosition, numDigits);
    }
    else {
        // 0 or -0 was read, e.g., "00.e050"
        x = impl_::makeDouble(isPositive, 0.0);
    }
}

/// \overload read(IStream& in, double& x)
///
double toDoubleApprox(const std::string& s)
{
    double res;
    std::stringstream in(s);
    readDoubleApprox(in, res);
    return res;

    // TODO: Use a custom StringViewStream class instead of std::stringstream
    // to avoid copying the data.

    // TODO: implement versions taking as input a custom StringView class (or
    // C++17 string_view)

    // TODO: Allow trailing whitespace but disallow trailing garbage.
    // Currently, our toDoubleApprox("1.0garbage") returns 1.0, while Python's
    // float("1.0garbage") raises ValueError.

    // TODO: Write the accurate version "toDouble()" using built-in C++
    // functions or Gay's dtoa.c (Python's modified version is probably even
    // better.). We should still do our own parsing (for precise control of
    // raised exceptions and allowed pattern), but then if valid, pass the
    // string (potentially cleaned up) to the third-party function for the
    // actual math.
}

} // namespace core
} // namespace vgc

#endif // VGC_CORE_IO_H
