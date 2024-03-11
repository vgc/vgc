// Copyright 2022 The VGC Developers
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

#ifndef VGC_GEOMETRY_WRAPS_VEC_H
#define VGC_GEOMETRY_WRAPS_VEC_H

#include <vgc/core/format.h>
#include <vgc/core/stringutil.h>
#include <vgc/core/templateutil.h>
#include <vgc/core/typeid.h>
#include <vgc/core/wraps/common.h>
#include <vgc/geometry/vec2d.h>
#include <vgc/geometry/vec2f.h>

namespace vgc::geometry::wraps {

namespace detail {

template<typename T>
std::string_view typeName() {
    return core::typeId<T>().name(); // may return "d" for double on some platform
}

template<>
inline std::string_view typeName<float>() {
    return "float";
}

template<>
inline std::string_view typeName<double>() {
    return "double";
}

// Same as py::cast<T>(input), but in case of failure it modifies the error
// type and message to make it more consistent with other failures when used in
// the context of a conversion from a Python type to a vgc::geometry C++ type:
//
// - Changes runtime_error to value_error.
// - Adds a period at the end.
// - Prints the full input.
//
template<typename T, typename PyType>
T cast(const PyType& input) {
    try {
        return py::cast<T>(input);
    }
    catch (...) {
        std::string repr = py::cast<std::string>(input.attr("__repr__")());
        char separator = core::contains(repr, '\n') ? '\n' : ' ';
        std::string message = core::format(
            "Unable to cast{}{}{}to C++ type '{}'.",
            separator,
            repr,
            separator,
            typeName<T>());
        throw py::value_error(message);
    }
}

// This function should be called in case of failure when converting a Python
// instance to a Vec or Mat.
//
// It allows to throw a `py::value_error` with an appropriate message, e.g.:
//
//    Unable to convert [[1, '2'], [4, 5]] to Mat2d: Unable to convert
//    [1, '2'] to Vec2d: Unable to cast '2' to C++ type 'double'.
//
// When implementing a conversion function, it is useful to catch all
// exceptions and rethrow using this function, which ensures for example that a
// `py::runtime_error` is more appropriately rethrown as a `py::value_error`.
//
template<typename TVecOrMat, typename PyType>
[[noreturn]] void
throwUnableToConvertVecOrMat(const PyType& input, const char* reason = nullptr) {

    const char* vecOrMat = vgc::geometry::isVec<TVecOrMat> ? "Vec" : "Mat";
    constexpr Int dimension = TVecOrMat::dimension;
    constexpr char typeSuffix = (sizeof(typename TVecOrMat::ScalarType) == 4) ? 'f' : 'd';

    std::string repr = py::cast<std::string>(input.attr("__repr__")());
    char separator = core::contains(repr, '\n') ? '\n' : ' ';

    const char* reasonSeparator = ": ";
    if (!reason) {
        reasonSeparator = ".";
        reason = "";
    }

    std::string message = core::format(
        "Unable to convert{}{}{}to {}{}{}{}{}",
        separator,
        repr,
        separator,
        vecOrMat,
        dimension,
        typeSuffix,
        reasonSeparator,
        reason);

    throw py::value_error(message);
}

template<typename TVec, typename PyType>
TVec vecFromSequence_(const PyType& s) {
    using T = typename TVec::ScalarType;
    constexpr Int dimension = TVec::dimension;

    if (s.size() != dimension) {
        throwUnableToConvertVecOrMat<TVec>(s, "Incompatible sizes.");
    }

    try {
        if constexpr (dimension == 2) {
            return TVec(cast<T>(s[0]), cast<T>(s[1]));
        }
        else if constexpr (dimension == 3) {
            return TVec(cast<T>(s[0]), cast<T>(s[1]), cast<T>(s[2]));
        }
        else if constexpr (dimension == 4) {
            return TVec(cast<T>(s[0]), cast<T>(s[1]), cast<T>(s[2]), cast<T>(s[3]));
        }
        else {
            static_assert("Vec type not supported.");
        }
    }
    catch (const std::exception& error) {
        vgc::geometry::wraps::detail::throwUnableToConvertVecOrMat<TVec>(s, error.what());
    }
    catch (...) {
        vgc::geometry::wraps::detail::throwUnableToConvertVecOrMat<TVec>(s);
    }
}

} // namespace detail

template<typename TVec>
TVec vecFromSequence(py::sequence sequence) {

    // A py::str is also a py::sequence, but here we really expect a sequence
    // of T, no a string. So if we have a string, we directly throw a useful
    // error message now, rather than the more cryptic "Incompatible sizes" or
    // "Cannot cast '(' to C++ type 'double'" message that would otherwise be
    // thrown later.
    //
    if (py::isinstance<py::str>(sequence)) {
        vgc::geometry::wraps::detail::throwUnableToConvertVecOrMat<TVec>(
            sequence, "Implicit conversion from string is not allowed in this context.");
    }
    return detail::vecFromSequence_<TVec>(sequence);
}

template<typename TVec>
TVec vecFromObject(py::handle obj) {
    py::sequence sequence;
    try {
        sequence = py::cast<py::sequence>(obj);
    }
    catch (...) {
        std::string reason =
            core::format("Expected a sequence of {} elements", TVec::dimension);
        vgc::geometry::wraps::detail::throwUnableToConvertVecOrMat<TVec>(
            obj, reason.c_str());
    }
    return vecFromSequence<TVec>(sequence);
}

template<typename TVec>
TVec vecFromTuple(py::tuple tuple) {
    return detail::vecFromSequence_<TVec>(tuple);
}

} // namespace vgc::geometry::wraps

#endif // VGC_GEOMETRY_WRAPS_VEC_H
