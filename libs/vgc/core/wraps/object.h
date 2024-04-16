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

#include <vgc/core/wraps/class.h>
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

template<typename TObj, typename... Options>
class ObjClass;

/// Specialize this to define the visible superclass in python.
///
template<typename T>
struct ObjClassSuperClass {
    using type = typename T::SuperClass;
};

namespace detail {

template<typename TObjClass>
struct ObjClassDeclarator_;

template<typename TObj, typename... Options>
struct ObjClassDeclarator_<ObjClass<TObj, Options...>> {
    using ObjClassType = ObjClass<TObj, Options...>;
    using type = ClassDeclarator<
        ObjClassType,
        TObj,
        ObjPtr<TObj>,
        typename ObjClassSuperClass<TObj>::type,
        Options...>;
};

template<typename... Options>
struct ObjClassDeclarator_<ObjClass<Object, Options...>> {
    using ObjClassType = ObjClass<Object, Options...>;
    using type = ClassDeclarator<ObjClassType, Object, ObjectPtr, Options...>;
};

template<typename TObjClass>
using ObjClassDeclarator = typename ObjClassDeclarator_<TObjClass>::type;

} // namespace detail

template<typename TObj, typename... Options>
class ObjClass : public detail::ObjClassDeclarator<ObjClass<TObj, Options...>> {

    using Base = detail::ObjClassDeclarator<ObjClass<TObj, Options...>>;

public:
    template<typename... Extra>
    ObjClass(py::handle scope, const char* name, const Extra&... extra)
        : Base(scope, name, py::dynamic_attr(), extra...) {
    }

    static_assert(isObject<TObj>);

    template<typename R = void, typename... Args, typename... Extra>
    ObjClass& def_create(const Extra&... extra) {
        if constexpr (std::is_same_v<R, void>) {
            Base::def(py::init(&TObj::create), extra...);
        }
        else {
            Base::def(py::init(static_cast<R (*)(Args...)>(&TObj::create)), extra...);
        }
        return *this;
    }

    // XXX prevent signatures with references to python immutables (int..)
    template<
        typename TSignal,
        typename... Extra,
        VGC_REQUIRES(core::isSignalGetter<TSignal>)>
    ObjClass& def_signal(const char* name, TSignal signal, const Extra&... extra) {
        static_assert(
            std::is_invocable_v<TSignal, const TObj*>,
            "Signal must be accessible in the class being pybound.");
        defSignal(name, signal, extra...);
        return *this;
    }

    // XXX prevent signatures with references to python immutables (int..)
    template<
        typename TSlot, //
        typename... Extra,
        VGC_REQUIRES(core::isSlotGetter<TSlot>)>
    ObjClass& def_slot(const char* name, TSlot slot, const Extra&... extra) {
        static_assert(
            std::is_invocable_v<TSlot, TObj*>,
            "Slot must be accessible in the class being pybound.");
        defSlot(name, slot, extra...);
        return *this;
    }

protected:
    template<typename TSignalRef, typename... Extra>
    void
    defSignal(const char* name, TSignalRef (TObj::*mfn)() const, const Extra&... extra) {
        std::string sname(name);
        py::cpp_function fget(
            [=](py::object self) -> PyCppSignalRef* {
                TObj* this_ = self.cast<TObj*>();
                PyCppSignalRef* sref =
                    new PyCppSignalRefImpl<TSignalRef>((this_->*mfn)());
                py::object pysref =
                    py::cast(sref, py::return_value_policy::take_ownership);
                self.attr("__dict__")[sname.c_str()] = pysref; // caching
                return sref; // pybind will find the object in registered_instances
            },
            py::keep_alive<0, 1>());
        Base::def_property_readonly(name, fget, extra...);
    }

    template<typename TSlotRef, typename... Extra>
    void defSlot(const char* name, TSlotRef (TObj::*mfn)(), const Extra&... extra) {
        std::string sname(name);
        py::cpp_function fget(
            [=](py::object self) -> PyCppSlotRef* {
                TObj* this_ = self.cast<TObj*>();
                PyCppSlotRef* sref =
                    new PyCppSlotRefImpl<typename TSlotRef::SlotMethod>((this_->*mfn)());
                py::object pysref =
                    py::cast(sref, py::return_value_policy::take_ownership);
                self.attr("__dict__")[sname.c_str()] = pysref; // caching
                return sref; // pybind will find the object in registered_instances
            },
            py::keep_alive<0, 1>());
        Base::def_property_readonly(name, fget, extra...);
    }
};

} // namespace vgc::core::wraps

#endif // VGC_CORE_WRAPS_OBJECT_H
