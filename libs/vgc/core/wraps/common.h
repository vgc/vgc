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

#ifndef VGC_CORE_WRAPS_COMMON_H
#define VGC_CORE_WRAPS_COMMON_H

#include <pybind11/pybind11.h>

#include <pybind11/functional.h>
#include <pybind11/operators.h>
#include <pybind11/stl.h> // wrap std::vector, set, optional, variant, etc.

namespace py = pybind11;
using namespace py::literals;

#include <vgc/core/object.h>

// Make pybind11 aware of our ObjPtr<T> holder. This macro must appear at
// global namespace. We use "true" since it is an intrusive smart pointer that
// can be safely constructed from a raw pointer. We also specialize
// holder_helper so that pybind11 uses getAlive() instead of get() to access
// the underlying pointer, which throws if the underlying pointer is null, or
// if the object isn't alive.
//
PYBIND11_DECLARE_HOLDER_TYPE(T, vgc::core::ObjPtr<T>, true)

namespace vgc::core::wraps {

/// Returns the fully-qualified name of the given scope, which is excepted
/// to be a handle to either a class or a module.
///
/// Examples:
/// - `vgc.geometry`
/// - `vgc.geometry.SegmentIntersector2d`
/// - `vgc.geometry.SegmentIntersector2d.PointIntersection`
///
inline std::string getScopeFullName(py::handle scope) {

    if (py::hasattr(scope, "__module__")) {

        // scope is a class (i.e., T is a nested class)
        // Example:
        //   scope.__module__   == 'vgc.geometry'
        //   scope.__qualname__ == 'SegmentIntersector2d'
        //
        std::string moduleName = py::cast<std::string>(scope.attr("__module__"));
        std::string parentQualName = py::cast<std::string>(scope.attr("__qualname__"));
        return vgc::core::format("{}.{}", moduleName, parentQualName);
    }
    else {
        // scope is a module.
        // Example:
        //   scope.__name__ == 'vgc.geometry'
        //
        return py::cast<std::string>(scope.attr("__name__"));
    }
}

} // namespace vgc::core::wraps

#endif // VGC_CORE_WRAPS_COMMON_H
