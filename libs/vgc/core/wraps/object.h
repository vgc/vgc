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

#include <memory>
#include <type_traits>

#include <vgc/core/object.h>
#include <vgc/core/wraps/common.h>
#include <vgc/core/wraps/signal.h>

namespace vgc::core::wraps {

// Define a suitable iterator object to be used for iterating in Python.
// Indeed, we can't use ObjListIterator<T> as is, because unlike C++ iterators,
// Python iterators need to be self-aware of when to stop.
//
template<typename T>
struct PyObjListIterator {
    ObjListIterator<T> current;
    ObjListIterator<T> end;
    PyObjListIterator(const ObjListView<T>& list)
        : current(list.begin())
        , end(list.end()) {
    }
};

// Wrap FooListIterator and FooListView for a given Foo object subclass.
//
template<typename T>
void wrapObjectCommon(py::module& m, const std::string& className) {
    //std::string listName = valueTypeName + "List"; // TODO?
    std::string listIteratorName = className + "ListIterator";
    std::string listViewName = className + "ListView";

    using ListIterator = PyObjListIterator<T>;
    using ListView = ObjListView<T>;

    py::class_<ListIterator>(m, listIteratorName.c_str())
        .def("__iter__", [](ListIterator& self) -> ListIterator& { return self; })
        .def(
            "__next__",
            [](ListIterator& self) -> T* {
                if (self.current == self.end)
                    throw py::stop_iteration();
                return *(self.current++);
            },
            py::return_value_policy::reference_internal);

    py::class_<ListView>(m, listViewName.c_str())
        .def(
            "__iter__",
            [](ListView& self) { return ListIterator(self); },
            py::return_value_policy::reference_internal)
        .def("__len__", &ListView::length);
}

#define OBJCLASS_WRAP_PYCLASS_METHOD(name)                                               \
    template<typename... Args>                                                           \
    ObjClass& name(Args&&... args) {                                                     \
        PyClass::name(std::forward<Args>(args)...);                                      \
        return *this;                                                                    \
    }

template<typename ObjT, typename... Options>
class ObjClass
    : py::class_<ObjT, core::ObjPtr<ObjT>, typename ObjT::SuperClass, Options...> {
public:
    using Holder = core::ObjPtr<ObjT>;
    using Parent = typename ObjT::SuperClass;
    using PyClass = py::class_<ObjT, Holder, Parent, Options...>;

    template<typename... Extra>
    ObjClass(py::handle scope, const char* name, const Extra&... extra)
        : PyClass(scope, name, py::dynamic_attr(), extra...) {
    }

    static_assert(isObject<ObjT>);

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

    template<typename R = void, typename... Args>
    ObjClass& def_create() {
        if constexpr (std::is_same_v<R, void>) {
            def(py::init(&ObjT::create));
        }
        else {
            def(py::init(static_cast<R (*)(Args...)>(&ObjT::create)));
        }
        return *this;
    }

    // XXX prevent signatures with references to python immutables (int..)
    template<
        typename SignalT,
        typename... Extra,
        VGC_REQUIRES(core::detail::isSignal<SignalT>)>
    ObjClass& def_signal(const char* name, SignalT signal, const Extra&... extra) {
        static_assert(
            std::is_invocable_v<SignalT, const ObjT*>,
            "Signal must be accessible in the class being pybound.");
        defSignal(name, signal, extra...);
        return *this;
    }

    // XXX prevent signatures with references to python immutables (int..)
    template<typename SlotT, typename... Extra, VGC_REQUIRES(core::detail::isSlot<SlotT>)>
    ObjClass& def_slot(const char* name, SlotT slot, const Extra&... extra) {
        static_assert(
            std::is_invocable_v<SlotT, ObjT*>,
            "Slot must be accessible in the class being pybound.");
        defSlot(name, slot, extra...);
        return *this;
    }

protected:
    template<typename SignalRefT, typename... Extra>
    void
    defSignal(const char* name, SignalRefT (ObjT::*mfn)() const, const Extra&... extra) {
        std::string sname(name);
        py::cpp_function fget(
            [=](py::object self) -> PyCppSignalRef* {
                ObjT* this_ = self.cast<ObjT*>();
                PyCppSignalRef* sref =
                    new PyCppSignalRefImpl<SignalRefT>((this_->*mfn)());
                py::object pysref =
                    py::cast(sref, py::return_value_policy::take_ownership);
                self.attr("__dict__")[sname.c_str()] = pysref; // caching
                return sref; // pybind will find the object in registered_instances
            },
            py::keep_alive<0, 1>());
        PyClass::def_property_readonly(name, fget, extra...);
    }

    template<typename SlotRefT, typename... Extra>
    void defSlot(const char* name, SlotRefT (ObjT::*mfn)(), const Extra&... extra) {
        std::string sname(name);
        py::cpp_function fget(
            [=](py::object self) -> PyCppSlotRef* {
                ObjT* this_ = self.cast<ObjT*>();
                PyCppSlotRef* sref =
                    new PyCppSlotRefImpl<typename SlotRefT::SlotMethod>((this_->*mfn)());
                py::object pysref =
                    py::cast(sref, py::return_value_policy::take_ownership);
                self.attr("__dict__")[sname.c_str()] = pysref; // caching
                return sref; // pybind will find the object in registered_instances
            },
            py::keep_alive<0, 1>());
        PyClass::def_property_readonly(name, fget, extra...);
    }
};

#undef OBJCLASS_WRAP_PYCLASS_METHOD

} // namespace vgc::core::wraps

#endif // VGC_CORE_WRAPS_OBJECT_H
