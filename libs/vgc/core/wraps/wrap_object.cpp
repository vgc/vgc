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

#include <vgc/core/object.h>

#include <vgc/core/wraps/common.h>
#include <vgc/core/wraps/object.h>

void wrap_object(py::module& m) {

    using This = vgc::core::Object;

    auto getattribute =
        py::module::import("builtins").attr("object").attr("__getattribute__");

    vgc::core::wraps::wrapObjectCommon<This>(m, "Object");

    vgc::core::wraps::ObjClass<This>(m, "Object")
        .def("isAlive", &This::isAlive)
        .def("refCount", &This::refCount)
        .def(
            "__getattribute__",
            [getattribute](py::object self, py::str name) {
                This* obj = self.cast<This*>();
                if (obj->isAlive()) {
                    return getattribute(self, name);
                }
                else {
                    std::string name_(name);
                    if (name_ == "isAlive" || name_ == "refCount") {
                        return getattribute(self, name);
                    }
                    else {
                        throw vgc::core::NotAliveError(obj);
                    }
                }
            })
        .def("parentObject", &This::parentObject)
        .def("firstChildObject", &This::firstChildObject)
        .def("lastChildObject", &This::lastChildObject)
        .def("previousSiblingObject", &This::previousSiblingObject)
        .def("nextSiblingObject", &This::nextSiblingObject)
        .def("childObjects", &This::childObjects)
        .def("numChildObjects", &This::numChildObjects)
        .def("isDescendantObjectOf", &This::isDescendantObjectOf)
        .def("dumpObjectTree", &This::dumpObjectTree);

    // test objects

    using ConstructibleTestObject = vgc::core::detail::ConstructibleTestObject;
    vgc::core::wraps::ObjClass<ConstructibleTestObject>(m, "ConstructibleTestObject")
        .def_create();

    using SignalTestObject = vgc::core::detail::SignalTestObject;
    vgc::core::wraps::ObjClass<SignalTestObject>(m, "CppSignalTestObject")
        .def_create()
        .def_signal("signalNoArgs", &SignalTestObject::signalNoArgs)
        .def_signal("signalInt", &SignalTestObject::signalInt)
        .def_signal("signalIntRef", &SignalTestObject::signalIntRef)
        .def_signal("signalConstIntRef", &SignalTestObject::signalConstIntRef)
        .def_signal("signalIntFloat", &SignalTestObject::signalIntFloat)
        .def_signal("signalIntFloatBool", &SignalTestObject::signalIntFloatBool)
        .def_slot("slotNoArgs", &SignalTestObject::slotNoArgs)
        .def_slot("slotFloat", &SignalTestObject::slotFloat)
        .def_slot("slotUInt", &SignalTestObject::slotUInt)
        .def_slot("slotInt", &SignalTestObject::slotInt)
        .def_slot("slotConstIntRef", &SignalTestObject::slotConstIntRef)
        .def_slot("slotIncIntRef", &SignalTestObject::slotIncIntRef)
        .def_slot("slotIntFloat", &SignalTestObject::slotIntFloat)
        .def("staticFuncInt", &SignalTestObject::staticFuncInt)
        .def("reset", &SignalTestObject::reset)
        .def_readwrite_static("sfnIntCalled", &SignalTestObject::sfnIntCalled)
        .def_readwrite("slotNoargsCallCount", &SignalTestObject::slotNoargsCallCount)
        .def_readwrite("sumInt", &SignalTestObject::sumInt)
        .def_readwrite("sumFloat", &SignalTestObject::sumFloat);
}
