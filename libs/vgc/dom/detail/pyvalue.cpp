// Copyright 2024 The VGC Developers
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

#include <vgc/dom/detail/pyvalue.h>

namespace vgc::dom::detail {

namespace {

// Python type identifier
using PyTypeId = core::StringId;

// Type of a map from a Python type identifier to a PyValueFactory
using PyValueFactoryMap = std::unordered_map<PyTypeId, PyValueFactory>;

// Returns the global factory map
PyValueFactoryMap& pyValueFactoryMap() {
    static PyValueFactoryMap map;
    return map;
}

// Get the identifier that identifies a given Python's object type.
//
// For now, we use the string representation of `obj.__class__` as identifier,
// which looks like "<class 'int'>" or "<class 'vgc.geometry.Vec2d'>".
//
// XXX: This currently requires dynamic allocations:
// - First to convert the __class__ attribute to an std::string
// - Then to convert it to a StringId
//
// Is there a more performant alternative?
//
PyTypeId getPyTypeId(py::handle h) {
    std::string s = py::str(h.attr("__class__"));
    return PyTypeId(s);
}

// "int" => "<class 'int'>"
PyTypeId getPyTypeId(std::string_view pyTypeName) {
    std::string pyClass = "<class '";
    pyClass += pyTypeName;
    pyClass += "'>";
    return PyTypeId(pyClass);
}

} // namespace

void registerPyValue(std::string_view pyTypeName, PyValueFactory factory) {
    detail::PyTypeId id = detail::getPyTypeId(pyTypeName);
    pyValueFactoryMap()[id] = factory;
}

Value createValue(py::handle h) {
    detail::PyTypeId id = detail::getPyTypeId(h);
    const auto& map = detail::pyValueFactoryMap();
    auto it = map.find(id);
    if (it != map.end()) {
        const auto& factory = it->second;
        return factory(h);
    }
    else {
        // If there is no registered C++ type corresponding to
        // the Python type of h, then fallback to holding the Python object directly.
        return Value(detail::AnyPyValue(h));
    }
}

} // namespace vgc::dom::detail
