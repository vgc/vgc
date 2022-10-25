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

#ifndef VGC_CORE_FORMAT_H
#define VGC_CORE_FORMAT_H

/// \file vgc/core/format.h
/// \brief Utilities to format strings and write to output streams.
///
/// This file defines various write() functions for writing to output streams,
/// defines the class StringWriter for wrapping an std::string into an output
/// stream, and also defines convenient toString(), format(), and formatTo()
/// functions for formatting strings.
///
/// The template parameter OStream can be any type implementing the following
/// functions, with the same semantics as std::ostream:
///
/// \code
/// OStream& put(char c);
/// OStream& write(const char* s, std::streamsize count);
/// explicit operator bool() const;
/// \endcode
///

#include <cstdio>  // fflush, fputc
#include <cstring> // strlen
#include <ios>     // streamsize
#include <limits>
#include <string>
#include <string_view>
#include <type_traits>
#include <unordered_map>
#include <vector>

// Include <fmt/format.h>, suppressing the following two warnings:
//
//   warning C4275: non dll-interface class 'std::runtime_error' used as base
//   for dll-interface class 'fmt::v6::format_error' / 'fmt::v6::system_error'
//
//   warning C4459: declaration of 'uint' hides global declaration
//   QtCore/qglobal.h: note: see declaration of 'uint'
//   fmt/format.h: note: see reference to function template instantiation
//                 'fmt::v8::detail::dragonbox::'... being compiled
//
#include <vgc/core/compiler.h>
#if defined(VGC_CORE_COMPILER_MSVC)
#    pragma warning(push)
#    pragma warning(disable : 4275)
#    pragma warning(disable : 4459)
#    pragma warning(disable : 4189)
#endif
#include <fmt/format.h>
#if defined(VGC_CORE_COMPILER_MSVC)
#    pragma warning(pop)
#endif

#include <vgc/core/api.h>
#include <vgc/core/preprocessor.h>
#include <vgc/core/templateutil.h>

namespace vgc::core {

/// Flushes the given `std::FILE`. If no argument is provided, flushes `stdout`.
///
/// Returns zero on success. Otherwise EOF is returned and the error indicator
/// of the file stream is set.
///
/// This is a convenient wrapper around `std::fflush`, see:
///
/// https://en.cppreference.com/w/cpp/io/c/fflush
///
inline int flush(std::FILE* stream = stdout) {
    return std::fflush(stream);
}

/// Prints formatted data to the given `std::FILE`.
///
/// ```cpp
/// vgc::geometry::Vec2d v(12, 42);
/// vgc::core::print(stderr, "position = {}", v);
/// ```
///
/// This function uses the {fmt} library under the hood. For more info on the
/// format syntax, see https://fmt.dev/latest/syntax.html
///
template<typename S, typename... Args>
inline void print(std::FILE* f, const S& formatString, Args&&... args) {
    fmt::vprint(f, formatString, fmt::make_args_checked<Args...>(formatString, args...));
}

/// Prints formatted data to `stdout`.
///
/// ```cpp
/// vgc::geometry::Vec2d v(12, 42);
/// vgc::core::print("position = {}", v);
/// ```
///
/// This function uses the {fmt} library under the hood. For more info on the
/// format syntax, see https://fmt.dev/latest/syntax.html
///
template<typename S, typename... Args>
inline void print(const S& formatString, Args&&... args) {
    fmt::vprint(formatString, fmt::make_args_checked<Args...>(formatString, args...));
}

/// Prints formatted data to the given file `f`, adds a newline, and flushes.
///
/// \sa print()
///
template<typename S, typename... Args>
inline void println(std::FILE* f, const S& formatString, Args&&... args) {
    fmt::vprint(f, formatString, fmt::make_args_checked<Args...>(formatString, args...));
    std::fputc('\n', f);
    vgc::core::flush(f);
}

/// Prints formatted data to `stdout`, adds a newline, and flushes.
///
/// \sa print()
///
template<typename S, typename... Args>
inline void println(const S& formatString, Args&&... args) {
    fmt::vprint(formatString, fmt::make_args_checked<Args...>(formatString, args...));
    std::fputc('\n', stdout);
    vgc::core::flush(stdout);
}

/// Formats arguments and returns the result as a string.
///
/// ```cpp
/// vgc::geometry::Vec2d v(12, 42);
/// std::string s = vgc::core::format("position = {}", v);
/// ```
///
/// This function uses the {fmt} library under the hood. For more info on the
/// format syntax, https://fmt.dev/latest/syntax.html
///
/// \sa formatTo()
///

template<typename... T>
VGC_FORCE_INLINE std::string format(fmt::format_string<T...> fmtString, T&&... args) {
    return fmt::vformat(fmtString, fmt::make_format_args(args...));
}

/// Formats arguments and appends the result to the given output iterator.
///
/// ```cpp
/// vgc::geometry::Vec2d v(12, 42);
/// std::string out = "The position is: "
/// vgc::core::formatTo(std::back_inserter(out), "{}", v);
/// ```
///
/// \sa format()
///
template<
    typename OutputIt,
    typename... T,
    VGC_REQUIRES(fmt::detail::is_output_iterator<OutputIt, char>::value)>
VGC_FORCE_INLINE OutputIt
formatTo(OutputIt out, fmt::format_string<T...> fmtString, T&&... args) {
    return fmt::vformat_to(out, fmtString, fmt::make_format_args(args...));
}

/// Writes the given `char` to the given output stream.
///
/// ```cpp
/// char c1 = 'a';
/// char c2 = 65; // = ASCII code for 'A'
/// vgc::core::write(out, c1); // write "a"
/// vgc::core::write(out, c2); // write "A"
/// ```
///
/// Note that throughout the VGC API, we use the type `char` to mean "one byte
/// of a UTF-8 encoded string", and the template parameter `OStream` to mean "a
/// UTF-8 encoded output stream". Therefore, calling this function writes the
/// given byte as is, without further encoding.
///
/// ```cpp
/// // Write the UTF-8 encoded character "é" (U+00E9)
/// char c1 = 0xC3;
/// char c2 = 0xA9;
/// vgc::core::write(out, c1);
/// vgc::core::write(out, c2);
/// ```
///
/// Note that unlike when using std::ostream::operator<<, the types `signed
/// char` and `unsigned char` aren't writen as is, but instead converted to
/// their decimal representation. See `write(OStream& out, IntType x)` for more
/// info.
///
template<typename OStream>
void write(OStream& out, char x) {
    out.put(x);
}

/// Writes the given null-terminated C-style string to the given output stream.
/// The behavior is undefined if the given C-style string isn't
/// null-terminated.
///
/// ```cpp
/// vgc::core::write(out, "Hello World!");
/// ```
///
template<typename OStream>
void write(OStream& out, const char* s) {
    size_t n = std::strlen(s);
    out.write(s, static_cast<std::streamsize>(n));
}

/// Writes the given char range to the given output stream.
///
/// ```cpp
/// vgc::core::write(out, s.begin(), s.size());
/// ```
///
template<typename OStream>
void write(OStream& out, const char* begin, std::streamsize ssize) {
    out.write(begin, ssize);
}

/// Writes the given string to the given output stream.
///
/// ```cpp
/// std::string s = "Hello World!";
/// vgc::core::write(out, s);
/// ```
///
template<typename OStream>
void write(OStream& out, const std::string& s) {
    size_t n = s.size();
    out.write(s.data(), static_cast<std::streamsize>(n));
}

/// Writes the decimal representation of the given integer to the given output
/// stream.
///
/// ```cpp
/// vgc::Int x = 65;
/// vgc::core::write(out, x); // write "65"
/// ```
///
/// Note that unlike when using std::ostream::operator<<, the types `signed
/// char`, `unsigned char`, `Int8`, and `UInt8` are also converted to their
/// decimal representation:
///
/// ```cpp
/// signed char c = 'A';
/// unsigned char d = 'A';
/// vgc::Int8 i = 65;
/// vgc::UInt8 j = 65;
/// std::cout << c;           // write "A"
/// std::cout << d;           // write "A"
/// std::cout << i;           // write "A"
/// std::cout << j;           // write "A"
/// vgc::core::write(out, c); // write "65"
/// vgc::core::write(out, d); // write "65"
/// vgc::core::write(out, i); // write "65"
/// vgc::core::write(out, j); // write "65"
/// ```
///
/// Ideally, we would prefer to write the `signed char` and `unsigned char` as
/// "A", and the `Int8` and `UInt8` as "65". Unfortunately, this isn't possible
/// due to the annoying fact that `int8_t` (resp. `uint8_t`) is typically a
/// typedef for `signed char` (resp. `unsigned char`), rather than a separate
/// type.
///
/// For this reason, in VGC, we never directly use the types `signed char` or
/// `unsigned char`. Instead, we always use `Int8` or `UInt8`, and mean them to
/// actually represent an 8-bit integer, not an ASCII character.
///
template<typename OStream, typename IntType, VGC_REQUIRES(std::is_integral_v<IntType>)>
void write(OStream& out, IntType x) {
    fmt::format_int f(x);
    out.write(f.data(), f.size());
}

/// Writes the decimal representation of the given floating-point number to the
/// given output stream. It never uses the scientific notation. It rounds to
/// the 12th digit after the decimal point, and rounds to 6 significant digits
/// for floats, and 15 significant digits for doubles.
///
/// ```cpp
/// vgc::core::write(out, 42.0);             // write "42"
/// vgc::core::write(out, -42.0);            // write "-42"
/// vgc::core::write(out, 1988.42);          // write "1988.42"
/// vgc::core::write(out, 0.0);              // write "0"
/// vgc::core::write(out, -0.0);             // write "0"
/// vgc::core::write(out, 0.000000000004);   // write "0.000000000004"
/// vgc::core::write(out, 0.0000000000004);  // write "0"
/// vgc::core::write(out, 0.0000000000006);  // write "0.000000000001"
/// vgc::core::write(out, -0.0000000000004); // write "0"
/// vgc::core::write(out, -0.0000000000006); // write "-0.000000000001"
/// vgc::core::write(out, 41.9999999999999); // write "42"
/// vgc::core::write(out, 1.0 / 0.0);        // write "inf"
/// vgc::core::write(out, -1.0 / 0.0);       // write "-inf"
/// vgc::core::write(out, 0.0 / 0.0);        // write "nan"
/// vgc::core::write(out, -0.0 / 0.0);       // write "nan"
/// ```
///
/// Note that all the examples above give the same result for floats and
/// doubles, due to their number of significant digits (after the input is
/// rounded to the 12th digit after the decimal point) being less or equal than
/// 6.
///
/// Below are examples where this function prints something different depending
/// on whether the input is a float or a double, due to the number of
/// significant digits being more than the precision of a float or double.
///
/// ```cpp
/// vgc::core::write(out, 0.01234567890123456);  // write "0.012345678901"
/// vgc::core::write(out, 0.01234567890123456f); // write "0.0123457"
/// vgc::core::write(out, 1234567890123456.0);   // write "1234567890120000"
/// vgc::core::write(out, 1234567890123456.0f);  // write "1234570000000000"
/// ```
///
template<
    typename OStream,
    typename FloatType,
    VGC_REQUIRES(std::is_floating_point_v<FloatType>)>
void write(OStream& out, FloatType x) {
    // Shortcut for zero:
    //   0.0000000000004 -> "0"
    //   0.0000000000006 -> "0.000000000001"
    constexpr FloatType eps = static_cast<FloatType>(5e-13);
    if (-eps < x && x < eps) {
        out.put('0');
        return;
    }

    // Convert to string, rounding to 12th digit after the decimal point.
    // Examples:
    //   1988.42  -> " 1988.420000000000"
    //   1988.42f -> " 1988.420043945312"
    // We use a leading whitespace to make room for carrying:
    //   9999999.f -> " 9999999.000000000000" (after calling fmt::format_to)
    //             -> "10000000             " (after our post-processing)
    fmt::memory_buffer b;
    formatTo(std::back_inserter(b), " {:.12f}", x);
    auto begin = b.begin() + 1;
    auto end = b.end();

    // Handle "nan", "inf", "-nan", and "-inf".
    // Note: we always convert "-nan" to "nan".
    if (b[1] == 'n' || b[1] == 'i' || (b[1] == '-' && (b[2] == 'n' || b[2] == 'i'))) {
        if (b[3] == 'a') {
            // Convert "-nan" to "nan"
            ++begin;
        }
        out.write(begin, end - begin);
        return;
    }

    // Read up to first significant digit.
    auto p = begin;
    int numSigDigits = 0;
    bool hasDecimalPoint = false;
    if (p != end && *p == '-') {
        // skip negative sign
        ++p;
    }
    while (numSigDigits == 0 && p != end) {
        if (*p == '.') {
            hasDecimalPoint = true;
        }
        else if (*p != '0') {
            ++numSigDigits;
        }
        else {
            // keep reading leading zeroes: 0.000...
        }
        ++p;
    }

    // Read up to (max + 1) number of significant digit.
    constexpr int maxSigDigits = std::numeric_limits<FloatType>::digits10;
    while (numSigDigits <= maxSigDigits && p != end) {
        if (*p == '.') {
            hasDecimalPoint = true;
        }
        else {
            ++numSigDigits;
        }
        ++p;
    }

    // Round up if necessary. Note that we round half away from zero:
    //   1000005.f  ->  "1000010"
    //   -1000005.f -> "-1000010"
    if (numSigDigits == maxSigDigits + 1) {
        --p;
        bool roundUp = (*p > '4');
        auto q = p;
        while (roundUp) {
            --p;
            if (*p == '.') {
                // skip decimal point
            }
            else if (*p == '9') {
                // carry over
                *p = '0';
            }
            else {
                // stop carrying over
                roundUp = false;
                if (*p == ' ') {
                    // handle leading space
                    *p = '1';
                    begin = p;
                }
                else if (*p == '-') {
                    // handle negative sign
                    *p = '1';
                    *--p = '-';
                    begin = p;
                }
                else {
                    // handle normal case
                    *p += 1;
                }
            }
        }
        p = q;
    }

    // Change all digits after max significant digits to '0'.
    while (p != end) {
        if (*p == '.') {
            hasDecimalPoint = true;
        }
        else {
            *p = '0';
        }
        ++p;
    }

    // Remove trailing zeros and trailing decimal point, if any.
    if (hasDecimalPoint) {
        --p;
        while (*p == '0') {
            // remove trailing zeros
            --p;
        }
        if (*p == '.') {
            // remove trailing decimal point
            --p;
        }
        ++p;
    }

    // Convert "-0" to "0". This is unlikely due the "shortcut for zero" at the
    // beginning of this function, but proving that it can never happen is
    // hard, so we take conservative measures.
    //
    if (begin[0] == '-' && begin[1] == '0' && p - begin == 2) {
        ++begin;
    }

    // Write out result.
    out.write(begin, p - begin);
}

/// Writes two or more formatted values, one after the other, to the output stream.
///
/// ```cpp
/// int x = 42;
/// double y = 1.5;
/// vgc::core::write(out, '(', x, ", ", y, ')');  // write "(42, 1.5)"
/// ```
///
template<typename OStream, typename T1, typename T2, typename... Args>
void write(OStream& out, T1 x1, T2 x2, Args... args) {
    write(out, x1);
    write(out, x2, args...);
}

/// \class vgc::core::StringWriter
/// \brief An output stream which appends characters to an existing string.
///
/// A StringWriter is a thin wrapper around a given string that allows you to
/// append formatted values to the string.
///
/// ```cpp
/// std::string s;
/// vgc::core::StringWriter sw(s);
/// sw << "The answer is " << 42; // append "The answer is 42" to s
/// ```
///
/// Note that the StringWriter holds a non-owning mutable reference to its
/// underlying string. This means that it is important that whoever creates a
/// StringWriter ensures that its underlying string outlives the StringWriter
/// itself, otherwise the behavior is undefined. For this reason, StringWriters
/// should typically be used in a very short, local scope. In particular,
/// StringWriters are non-copyable, and should typically not be stored as
/// member variables.
///
/// StringWriters are extremely lightweight and fast. It is usually at least
/// twice as fast as using std::ostringstream, due to better use of cache,
/// optimized number to string conversions, avoidance of the final string copy,
/// and not having any of the heavy machinery brought by std::ios and
/// std::streambuf (virtual function calls, locale, sentry, etc.).
///
/// ```cpp
/// // This is much slower
/// std::ostringstream oss;
/// oss << "The answer is " << 42;
/// std::string s = oss.str();
/// ```
///
/// Using a StringWriter is also usually a bit faster than using the `+` or `+=`
/// string operators directly, since it avoids allocation of temporary
/// strings. However, the difference is often not very significant due to
/// the so-called "small string optimization".
///
/// ```cpp
/// // This is a little slower
/// std::string s = "The answer is " + vgc::core::toString(42);
/// ```
///
class StringWriter {
public:
    using string_type = std::string;
    using size_type = string_type::size_type;

    /// Constructs a StringWriter operating on the given string.
    /// The string must outlive this StringWriter.
    ///
    StringWriter(std::string& s)
        : s_(s) {
    }

    /// Appends a character to the underlying string.
    ///
    StringWriter& put(char c) {
        s_.push_back(c);
        return *this;
    }

    /// Appends multiple characters to the underlying string.
    /// The behavior is undefined if count < 0.
    ///
    StringWriter& write(const char* s, std::streamsize count) {
        s_.append(s, static_cast<size_type>(count));
        return *this;
    }

    /// Returns whether the stream has no errors. This always returns true,
    /// since a StringWriter can never be in error: it is always possible to
    /// add more characters to a string, unless we run out of memory which will
    /// cause other errors anyway.
    ///
    explicit operator bool() {
        return true;
    }

private:
    std::string& s_;
};

/// Appends the given value to the underlying string of the given StringWriter.
///
/// ```cpp
/// std::string s;
/// vgc::core::StringWriter sw(s);
/// sw << 42; // append "42" to s
/// ```
///
template<typename T>
inline StringWriter& operator<<(StringWriter& sw, const T& x) {
    write(sw, x);
    return sw;
}

/// Returns a string representation of the given value.
///
/// ```cpp
/// char c = 'A';
/// vgc::Int i = 42;
/// double x = 1.5;
/// std::cout << toString(c); // writes out "A"
/// std::cout << toString(i); // writes out "42"
/// std::cout << toString(x); // writes out "1.5"
/// ```
///
/// Note that calling `s = toString(x)` is equivalent to the following:
///
/// ```cpp
/// using vgc::core::write;
/// std::string s;
/// StringWriter sw(s);
/// write(out, x)
/// ```
///
/// Therefore, for more details on how the given value is formatted, please
/// refer to the documentation of write(OStream& out, T& x) corresponding to
/// the given type T of the given value.
///
template<typename T>
std::string toString(const T& x) {
    std::string s;
    StringWriter out(s);
    write(out, x);
    return s;
}

/// Casts the given pointer to a `const void*`, which ensures
/// that it is printed as an address string.
///
/// \sa toAddressString(const T* x)
///
template<typename T>
const void* asAddress(const T* x) {
    return static_cast<const void*>(x);
}

/// Converts the given pointer to its address string.
///
/// Note that we use a specific function name for this (rather than overloading
/// toString()) for better type safety, and forcing the caller to disambiguate
/// between printing the address of the pointer, or its content. In other
/// words, like in {fmt}, automatic formatting of non-void pointers is
/// intentionally disabled.
///
/// More info:
/// https://github.com/fmtlib/fmt/issues/1248
/// https://accu.org/index.php/journals/1539
///
template<typename T>
std::string toAddressString(const T* x) {
    return core::format("{}", asAddress(x));
}

/// \enum vgc::core::TimeUnit
/// \brief Enumeration of all possible time units
///
enum class TimeUnit {
    Seconds,
    Milliseconds,
    Microseconds,
    Nanoseconds
};

/// Converts a floating-point number representing a duration in seconds
/// into a human-readable string in the given TimeUnit with the given
/// number of decimal points.
///
VGC_CORE_API
std::string
secondsToString(double t, TimeUnit unit = TimeUnit::Seconds, int decimals = 0);

/// Converts an integer in the range [0..255] to a pair of characters in [0-9a-z].
///
/// ```cpp
/// std::string_view hex = toHexPair(42); // => "2a"
/// ```
///
VGC_CORE_API
std::string_view toHexPair(unsigned char x);

} // namespace vgc::core

#endif // VGC_CORE_FORMAT_H
