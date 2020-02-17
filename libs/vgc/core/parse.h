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

#ifndef VGC_CORE_PARSE_H
#define VGC_CORE_PARSE_H

/// \file vgc/core/parse.h
/// \brief Utilities to parse strings and read input streams.
///
/// This file defines various read() functions for reading input streams,
/// defines the class StringReader for wrapping an std::string into an input
/// stream, and also defines convenient parse() functions for parsing strings.
///
/// The template parameter IStream can be any type implementing the following
/// functions, with the same semantics as std::istream:
///
/// \code
/// IStream& get(char& c);
/// IStream& unget();
/// explicit operator bool() const;
/// \endcode
///

#include <limits>
#include <string>
#include <type_traits>

#include <vgc/core/api.h>
#include <vgc/core/exceptions.h>
#include <vgc/core/format.h>
#include <vgc/core/int.h>

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
inline double digitToDoubleNoRangeCheck(char c)
{
    const double t[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
    return t[c - '0'];
}

/// Returns the int represented by the given digit character \p c, assuming
/// that \p c is indeed a digit (that is, isDigit(c) must return true).
/// Otherwise, the behaviour is undefined.
///
VGC_CORE_API
inline int digitToIntNoRangeCheck(char c)
{
    return static_cast<int>(c - '0');
}

/// Returns the double represented by the given digit character \p c. If \p c
/// is not a digit (that is, isDigit(c) returns false), then ParseError is
/// raised.
///
VGC_CORE_API
inline double digitToDouble(char c)
{
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
inline int digitToInt(char c)
{
    if (isDigit(c)) {
        return digitToIntNoRangeCheck(c);
    }
    else {
        throw ParseError(
            std::string("Unexpected '") + c + "'. Expected a digit [0-9].");
    }
}

/// Extracts all leading whitespace characters from the input stream.
///
template<typename IStream>
void skipWhitespaceCharacters(IStream& in)
{
    bool foundNonWhitespaceCharacter = false;
    char c = -1; // dummy-initialization in order to silence warning
    while (!foundNonWhitespaceCharacter && in.get(c)) {
        if (!isWhitespace(c)) {
            foundNonWhitespaceCharacter = true;
        }
    }
    if (foundNonWhitespaceCharacter) {
        in.unget();
    }
}

/// Extracts the next character from the input stream, and raises ParseError if
/// this character is not the given character, or if the stream ends.
///
template<typename IStream>
void skipExpectedCharacter(IStream& in, char c)
{
    char d = readCharacter(in);
    if (d != c)  {
        throw ParseError(
            std::string("Unexpected '") + d + "'. Expected '" + c + "'.");
    }
}

/// Extracts the next character from the input stream, excepting that there is
/// none. Raises ParseError if the stream actually didn't end.
///
template<typename IStream>
void skipExpectedEof(IStream& in)
{
    char c = -1; // dummy-initialization in order to silence warning
    if (in.get(c)) {
        throw ParseError(
            std::string("Unexpected character '") + c +
            "'. Expected end of stream.");
    }
}

/// Extracts characters from the input stream one by one until a non-whitespace
/// character is extracted, and returns this non-whitespace character. Raises
/// ParseError if the stream ends before a non-whitespace character is found.
///
template<typename IStream>
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

/// Extracts the next character from the input stream. Raises
/// ParseError if the stream ends.
///
template<typename IStream>
char readCharacter(IStream& in)
{
    char c = -1; // dummy-initialization in order to silence warning
    if (!in.get(c)) {
        throw ParseError(
            "Unexpected end of stream. Expected a character.");
    }
    return c;
}

/// Extracts and returns the next character from the input stream. Raises
/// ParseError if this character does not belong to \p allowedCharacters or if
/// the stream ends.
///
template<typename IStream, std::size_t N>
char readExpectedCharacter(IStream& in, const char (&allowedCharacters)[N])
{
    char c = readCharacter(in);
    bool allowed = false;
    for (int i = 0; i < N; ++i) {
        allowed = allowed || (c == allowedCharacters[i]);
    }
    if (!allowed)  {
        std::string allowedCharactersString;
        if (N > 0) {
            allowedCharactersString += "'";
            allowedCharactersString += allowedCharacters[0];
            allowedCharactersString += "'";
        }
        for (int i = 1; i < N; ++i) {
            allowedCharactersString += ", '";
            allowedCharactersString += allowedCharacters[i];
            allowedCharactersString += "'";
        }
        throw ParseError(
            std::string("Unexpected '") + c + "'. Expected one of the "
            "following characters: " + allowedCharactersString + "'.");
    }
    return c;
}

/// Reads a base-10 text representation of an unsigned integer from the input
/// stream \p in. Leading whitespaces are allowed. No leading positive or
/// negative sign is allowed. Raises ParseError if the stream does not contain
/// an unsigned integer. Raises RangeError if the stream contains an unsigned
/// integer outside of the representable range of the output type.
///
/// Note that the output type can also be a signed integer type, but a
/// ParseError is raised if a positive or negative signed is found.
///
template<typename IntType, typename IStream>
typename std::enable_if<std::is_integral<IntType>::value, IntType>::type
readUnsignedInteger(IStream& in)
{
    constexpr IntType n = vgc::internal::type_max<IntType>::value;
    constexpr IntType m = n / 10;
    constexpr IntType k = n % 10;
    char c = readNonWhitespaceCharacter(in);
    IntType i = 0;
    bool hasDigits = false;
    while (isDigit(c)) {
        hasDigits = true;
        IntType j = static_cast<IntType>(c - '0');
        if (i < m || (i == m && j <= k)) {
            i = 10*i + j;
        }
        else {
            throw RangeError(core::format(
                "The integer {}{} is outside the range of {}.",
                i, c, int_typename<IntType>()));
        }
        if (!in.get(c)) { // end of stream
            return i;
        }
    }
    if (!hasDigits) {
        throw ParseError(core::format(
            "Unexpected '{}' before any digit of the integer was read. "
            "Expected a digit [0-9].", c));
    }
    in.unget(); // last read character wasn't a digit
    return i;
}

namespace internal {

// Implementation of readSignedInteger. We use this indirection to make
// documentation easier: there is only one function in the public API, taking
// any type of integer. Only in the implementation we have two functions using
// SFINAE for dispatching.
//
template<typename SIntType, typename IStream>
typename std::enable_if<
    std::is_integral<SIntType>::value &&
    std::is_signed<SIntType>::value, SIntType>::type
readSignedIntegerImpl(IStream& in)
{
    char c = readNonWhitespaceCharacter(in);
    bool isPositive = true;
    if (c == '-' || c == '+') {
        isPositive = (c == '+');
    }
    else {
        in.unget();
    }
    if (isPositive) {
        return readUnsignedInteger<SIntType>(in);
    }
    else {
        // We parse as a UIntType to ensure it handles IntMin correctly
        using UIntType = typename std::make_unsigned<SIntType>::type;
        constexpr SIntType imin = vgc::internal::type_min<SIntType>::value;
        constexpr UIntType umin = static_cast<UIntType>(imin);
        UIntType u = readUnsignedInteger<UIntType>(in);
        if (u < umin) {
            return (0 - static_cast<SIntType>(u));
        }
        else if (u == umin) {
            return imin;
        }
        else {
            throw RangeError(core::format(
                "The integer -{} is outside the range of {}.",
                u, int_typename<SIntType>()));
        }
    }
}

template<typename UIntType, typename IStream>
typename std::enable_if<
    std::is_integral<UIntType>::value &&
    std::is_unsigned<UIntType>::value, UIntType>::type
readSignedIntegerImpl(IStream& in)
{
    char c = readNonWhitespaceCharacter(in);
    bool isPositive = true;
    if (c == '-' || c == '+') {
        isPositive = (c == '+');
    }
    else {
        in.unget();
    }
    UIntType u = readUnsignedInteger<UIntType>(in);
    if (isPositive || u == 0) {
        return u;
    }
    else {
        throw RangeError(core::format(
            "The integer -{} is outside the range of {}.",
            u, int_typename<UIntType>()));
    }
}

} // namespace internal

/// Reads a base-10 text representation of a signed integer from the input
/// stream \p in. Leading whitespaces are allowed. A leading positive or
/// negative sign is allowed. Raises ParseError if the stream does not contain
/// a signed integer. Raises RangeError if the stream contains a signed integer
/// outside of the representable range of the output type.
///
/// Note that the output type can also be an unsigned integer type. In this
/// case, if a negative integer is read, then a RangeError is raised. No error
/// is raised for "-0".
///
template<typename IntType, typename IStream>
typename std::enable_if<std::is_integral<IntType>::value, IntType>::type
readSignedInteger(IStream& in)
{
    return internal::readSignedIntegerImpl<IntType>(in);
}

namespace internal {

// Computes (-1)^s * a * 10^b, where a must be a double representing an integer
// with n digits. This latter argument is used to guard against underflow and
// overflow.
double computeDouble(bool isPositive, double a, int b, int n);

// Computes (-1)^s * a
inline double computeDouble(bool isPositive, double a) { return isPositive ? a : -a; }

} // namespace internal

/// Reads a base-10 text representation of a number from the input stream \p
/// in, and converts it approximately to a double, with a guaranteed precision
/// of 15 significant digits. This is an optimization to make the conversion
/// from base-10 to base-2 faster when accuracy above 15 significant digits is
/// not required. If in doubt, use readDouble() instead.
///
/// Leading whitespaces are allowed. After leading whistespaces are skipped,
/// the text representation must match the following pattern:
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
/// including) the first non-matching character.
///
/// If the text representation matches the pattern, then the longest matching
/// sequence is considered. The stream is read up to (but excluding) the first
/// non-matching character. If the absolute value of the number is greater or
/// equal to 1.0e+308, then RangeError is raised. If the absolute value of the
/// number is smaller than 10^-307, then the returned value is 0.
///
/// Unlike many built-in C++ utilities performing similar tasks (e.g., sscanf,
/// strtod, atof, istream::operator>>), this function does not depend on
/// locale, that is, the decimal point is always assumed to be ".". Instead,
/// this function is similar to std::from_chars (introduced in C++17), but
/// unlike std::from_chars, this function throws an exception on error, and
/// takes as input a templated input stream.
///
template<typename IStream>
double readDoubleApprox(IStream& in)
{
    // TODO?
    // - Also define a non-throwing version, taking an error
    // code/string/struct as output argument.

    // Overview of the algorithm, with the example input "   -0012.3456e+2"
    //
    // 1. Skip whitespaces
    // 2. Read plus/minus sign
    // 3. Compute double a = 123456 via the following sequence of operations:
    //      a = 1
    //      a = (10*x) + 2
    //      a = (10*x) + 3
    //      a = (10*x) + 4
    //      a = (10*x) + 5
    //      a = (10*x) + 6
    //
    //    We ignore all digits after reading 17 significant digits.
    //
    //    We remember dotPosition = 4 and numDigits = 6
    //
    //    Here are other examples of resulting (a, dotPosition, numDigits) based on input:
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
    //    - Up to reading the 15th digit, the value hold by 'a' is exact.
    //    - Once we read the 16th digit, there is a 95% chance that the value
    //    hold by 'a' is exact. In the remaining 5% of cases, the value is off by 1.
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
    // 5. Compute x = a * 10^exponent
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
            return internal::computeDouble(isPositive, 0.0);
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
            return internal::computeDouble(isPositive, a, -dotPosition, numDigits);
        }
    }

    // Read decimal point
    if (c == '.') {
        if (!in.get(c)) {
            if (numDigits > 0) {
                // End of stream; a non-zero integer was read, e.g., "042."
                return internal::computeDouble(isPositive, a, -dotPosition, numDigits);
            }
            else if (hasLeadingZeros)  {
                // End of stream; 0 or -0 was read, e.g.,  "00."
                return internal::computeDouble(isPositive, 0.0);
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
                return internal::computeDouble(isPositive, 0.0);
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
            return internal::computeDouble(isPositive, a, -dotPosition, numDigits);
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
                    return internal::computeDouble(isPositive, a, exponent - dotPosition, numDigits);
                }
                else {
                    // End of stream; 0 or -0 was read, e.g., "00.e050"
                    return internal::computeDouble(isPositive, 0.0);
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
        return internal::computeDouble(isPositive, a, exponent - dotPosition, numDigits);
    }
    else {
        // 0 or -0 was read, e.g., "00.e050"
        return internal::computeDouble(isPositive, 0.0);
    }
}

/// Approximately converts a base-10 text representation of a number into a
/// double stored in \p x, with a guaranteed precision of 15 significant
/// digits. See readDoubleApprox() for details.
///
VGC_CORE_API
double toDoubleApprox(const std::string& s);

/// Reads the next character from the input stream, and store it in the output
/// parameter. Raises ParseError if no character can be read from the stream
/// (e.g., we've already reached the end).
///
template<typename IStream>
void readTo(char& c, IStream& in)
{
    c = readCharacter(in);
}

/// Reads a base-10 text representation of a signed integer from the input
/// stream \p in, and stores it in the given output parameter. Leading
/// whitespaces are allowed. A positive or negative sign is allowed. Raises
/// ParseError if the stream does not contain an integer. Raises RangeError if
/// the stream contains an integer but without the allowed range of IntType.
///
template<typename SIntType, typename IStream>
typename std::enable_if<
    std::is_integral<SIntType>::value &&
    std::is_signed<SIntType>::value>::type
readTo(SIntType& i, IStream& in)
{
    i = readSignedInteger<SIntType>(in);
}

/// Reads a base-10 text representation of an unsigned integer from the input
/// stream \p in, and stores it in the given output parameter. Leading
/// whitespaces are allowed. No positive or negative signs are allowed. Raises
/// ParseError if the stream does not contain an integer. Raises RangeError if
/// the stream contains an integer but without the allowed range of UIntType.
///
template<typename UIntType, typename IStream>
typename std::enable_if<
    std::is_integral<UIntType>::value &&
    std::is_unsigned<UIntType>::value>::type
readTo(UIntType& i, IStream& in)
{
    i = readUnsignedInteger<UIntType>(in);
}

/// Reads a base-10 text representation of a float from the input stream \p
/// in, and stores it in the given output parameter. Raises ParseError if the
/// stream does not contain a float. See readDoubleApprox() for details on
/// accepted input.
///
template<typename IStream>
void readTo(float& x, IStream& in)
{
    x = static_cast<float>(readDoubleApprox(in));
}

/// Reads a base-10 text representation of a double from the input stream \p
/// in, and stores it in the given output parameter. Raises ParseError if the
/// stream does not contain a double. See readDoubleApprox() for details on
/// accepted input.
///
template<typename IStream>
void readTo(double& x, IStream& in)
{
    x = readDoubleApprox(in);
}

/// Reads and returns a value of the given type T from the input stream \p in.
/// This function may raise ParseError or RangeError, please refer to the
/// documentation of readTo(T& x, IStream& in) for the given type T for more
/// details.
///
/// ```cpp
/// double x = read<double>(in);
/// ```
///
template<typename T, typename IStream>
T read(IStream& in)
{
    T x;
    readTo(x, in);
    return x;
}

} // namespace core
} // namespace vgc

#endif // VGC_CORE_PARSE_H
