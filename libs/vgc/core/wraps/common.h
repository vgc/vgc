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

#ifndef VGC_CORE_WRAPS_COMMON_H
#define VGC_CORE_WRAPS_COMMON_H

#include <pybind11/pybind11.h>
namespace py = pybind11;

namespace vgc {
namespace core {

/// Return value policy to use for methods returning raw pointers to
/// vgc::core::Object instances. These pointers are non-owning and we desire
/// that Python references to such objects do NOT extend the lifetime of the
/// object.
///
// XXX At this point, we haven't yet determined whether we should use
// py::return_value_policy::reference_internal, or
// py::return_value_policy::reference, or something else to achieve our goals.
// Below are examples of the behaviour we would like, if at all possible.
//
// Scenario 1:
//
// >>> element = someElement()
// >>> child = element.firstChild
// >>> element.removeChild(child) # or child deleted from the GUI, C++ side
// >>> child.doSomething()
// ExpiredObjectError: the given element does not exist anymore
//
// Scenario 2:
//
// >>> root = Document().createChildElement("vgc")
// >>> print(root)
// <vgc.dom.Vgc object at 0x7f45ba8d4ab0>
// >>> print(root.parent)
// <vgc.dom.Document object at 0x7f45ba8d4ab0>
//
// Current behaviour:
//
// Python 3.6.5 (default, Apr  1 2018, 05:46:30)
// [GCC 7.3.0] on linux
// Type "help", "copyright", "credits" or "license" for more information.
// >>> from vgc.dom import *
// >>> doc = Document()
// Object 0x11cfe50 constructed
// >>> foo = doc.appendChild(Element("foo"))
// Object 0x1153080 constructed
// >>> foo.parent
// <vgc.dom.Document object at 0x7fa3a1d311b8>
// >>> bar = Document().appendChild(Element("bar"))
// Object 0x11dd160 constructed
// Object 0x11dd2c0 constructed
// Object 0x11dd160 destructed
// >>> bar.parent
// <vgc.core.Object object at 0x7fa3a1d3d068>
// >>> exit()
// Object 0x11cfe50 destructed
// Object 0x1153080 destructed
// Object 0x11dd2c0 destructed
//
const auto object_ptr_policy = py::return_value_policy::reference_internal;

} // namespace core
} // namespace vgc

#endif // VGC_CORE_WRAPS_COMMON_H
