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

#ifndef VGC_CORE_STRINGUTIL_H
#define VGC_CORE_STRINGUTIL_H

#include <string>
#include <type_traits>
#include <vector>
#include <vgc/core/api.h>

namespace vgc {
namespace core {

/// Converts the given character to a string.
///
/// Example:
/// \code
/// char c = 65;
/// std::cout << toString(c); // writes out "A" (ASCII code for 'A' is 65)
/// std::cout << c;           // writes out "A"
/// \endcode
///
VGC_CORE_API
std::string toString(char x);

/// Converts the given 8-bit signed integer to a string.
///
/// Example:
/// \code
/// signed char c = 65;
/// std::cout << toString(c); // writes out "65"
/// std::cout << c;           // writes out "A" (ASCII code for 'A' is 65)
/// \endcode
///
VGC_CORE_API
std::string toString(signed char x);

/// Converts the given 8-bit unsigned integer to a string.
///
/// Example:
/// \code
/// unsigned char c = 65;
/// std::cout << toString(c); // writes out "65"
/// std::cout << c;           // writes out "A" (ASCII code for 'A' is 65)
/// \endcode
///
VGC_CORE_API
std::string toString(unsigned char x);

/// Converts the given integer to a string.
///
/// Example:
/// \code
/// Int x = 65;
/// std::cout << toString(x); // writes out "65"
/// \endcode
///
template <typename T>
typename std::enable_if<std::is_integral<T>::value, std::string>::type
toString(T x) {
    return std::to_string(x);
}

/// Converts the given double to a string.
///
VGC_CORE_API
std::string toString(double x);

/// Converts the given address to a string.
///
VGC_CORE_API
std::string toString(const void* x);

/// Converts the given address to a string.
///
template <typename T>
std::string toString(const T* x) {
    return toString(static_cast<const void*>(x));
}

/// Converts the given vector to a string.
///
template <typename T>
std::string toString(const std::vector<T>& v) {
    std::string res = "[";
    std::string sep = "";
    for (const T& x : v) {
        res += sep + toString(x);
        sep = ", ";
    }
    res += "]";
    return res;
}

/// Approximately converts a base-10 text representation of a number into a
/// double stored in \p x, with a guaranteed precision of 15 significant
/// digits. See readDoubleApprox() for details.
///
VGC_CORE_API
double toDoubleApprox(const std::string& s);

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
std::string secondsToString(double t, TimeUnit unit = TimeUnit::Seconds, int decimals = 0);

} // namespace core
} // namespace vgc

#endif // VGC_CORE_STRINGUTIL_H
