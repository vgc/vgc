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

#ifndef VGC_CORE_WRAPS_OBJECT_H
#define VGC_CORE_WRAPS_OBJECT_H

#include <vgc/core/wraps/common.h>
#include <vgc/core/wraps/signal.h>
#include <vgc/core/object.h>

namespace vgc::core::wraps {

// Define a suitable iterator object to be used for iterating in Python.
// Indeed, we can't use ObjListIterator<T> as is, because unlike C++ iterators,
// Python iterators need to be self-aware of when to stop.
//
template<typename T>
struct PyObjListIterator {
    ObjListIterator<T> current;
    ObjListIterator<T> end;
    PyObjListIterator(const ObjListView<T>& list) :
        current(list.begin()), end(list.end()) {}
};

// Wrap FooListIterator and FooListView for a given Foo object subclass.
//
template<typename T>
void wrapObjectCommon(py::module& m, const std::string& className)
{
    //std::string listName = valueTypeName + "List"; // TODO?
    std::string listIteratorName = className + "ListIterator";
    std::string listViewName = className + "ListView";

    using ListIterator = PyObjListIterator<T>;
    using ListView = ObjListView<T>;

    py::class_<ListIterator>(m, listIteratorName.c_str())
        .def("__iter__", [](ListIterator& self) -> ListIterator& { return self; })
        .def("__next__", [](ListIterator& self) -> T* {
            if (self.current == self.end) throw py::stop_iteration();
            return *(self.current++); },
            py::return_value_policy::reference_internal)
    ;

    py::class_<ListView>(m, listViewName.c_str())
        .def("__iter__", [](ListView& self) { return ListIterator(self); }, py::return_value_policy::reference_internal)
        .def("__len__", &ListView::length)
    ;
}

#define OBJCLASS_WRAP_PYCLASS_METHOD(name) \
    template<typename... Args> \
    ObjClass& name(Args&&... args) { \
        PyClass::name(std::forward<Args>(args)...); \
        return *this; \
    }

template <typename type_, typename... options>
class ObjClass : py::class_<type_, core::ObjPtr<type_>, options...> {
public:
    using Holder = core::ObjPtr<type_>;
    using PyClass = py::class_<type_, Holder, options...>;
    using PyClass::PyClass;
    
    OBJCLASS_WRAP_PYCLASS_METHOD(def)
    OBJCLASS_WRAP_PYCLASS_METHOD(def_static)
    OBJCLASS_WRAP_PYCLASS_METHOD(def_cast)
    OBJCLASS_WRAP_PYCLASS_METHOD(def_buffer)
    OBJCLASS_WRAP_PYCLASS_METHOD(def_readwrite)
    OBJCLASS_WRAP_PYCLASS_METHOD(def_readonly)
    OBJCLASS_WRAP_PYCLASS_METHOD(def_readwrite_static)
    OBJCLASS_WRAP_PYCLASS_METHOD(def_readonly_static)
    OBJCLASS_WRAP_PYCLASS_METHOD(def_property_readonly)
    OBJCLASS_WRAP_PYCLASS_METHOD(def_property_readonly_static)
    OBJCLASS_WRAP_PYCLASS_METHOD(def_property)
    OBJCLASS_WRAP_PYCLASS_METHOD(def_property_static)

    template<typename R, typename... SignalArgs, typename... Extra>
    ObjClass& def_signal(const char* name, R(type_::* mfn)(SignalArgs...) const, const Extra&... extra) {
        static_assert(vgc::core::internal::isSignal<decltype(mfn)>);
        py::cpp_function fget(
            [mfn](const type_& c) -> AbstractCppSignalRef* {
                return new CppSignalRef<SignalArgs...>(const_cast<type_*>(&c), R::getInfo().name);
            }, py::is_method(*this));
        PyClass::def_property_readonly(name, fget, py::return_value_policy::take_ownership, extra...);
        return *this;
    }

    template<typename... SlotArgs, typename... Extra>
    ObjClass& def_slot(const char* name, void (type_::* mfn)(SlotArgs...), const Extra&... extra) {
        // XXX check is slot if possible
        py::cpp_function fget(
            [mfn](const type_& c) -> AbstractCppSlotRef* {
                return new CppSlotRefImpl<type_, SlotArgs...>(const_cast<type_*>(&c), mfn);
            }, py::is_method(*this));
        PyClass::def_property_readonly(name, fget, py::return_value_policy::take_ownership, extra...);
        return *this;
    }
};

#undef OBJCLASS_WRAP_PYCLASS_METHOD

} // namespace vgc::core::wraps


// Wraps all the  exception base class.
//
// ```cpp
// VGC_CORE_WRAP_BASE_EXCEPTION(core, LogicError);
// ```
//
#define VGC_CORE_WRAP_OBJECT_RELATED_CLASSES(libname, ErrorType) \
    py::register_exception<vgc::libname::ErrorType>(m, #ErrorType)

// Wraps an exception class deriving from another exception class. If the
// parent exception is from the same module, simply pass it `m`, otherwise, you
// must import beforehand the module in whic the parent Exception is defined.
//
// ```cpp
// VGC_CORE_WRAP_EXCEPTION(core, IndexError, m, LogicError);
//
// py::module core = py::module::import("vgc.core");
// VGC_CORE_WRAP_EXCEPTION(dom, LogicError, core, LogicError);
// ```
//
#define VGC_CORE_WRAP_EXCEPTION(libname, ErrorType, parentmodule, ParentErrorType) \
    py::register_exception<vgc::libname::ErrorType>(m, #ErrorType, parentmodule.attr(#ParentErrorType).ptr())

#endif // VGC_CORE_WRAPS_OBJECT_H
