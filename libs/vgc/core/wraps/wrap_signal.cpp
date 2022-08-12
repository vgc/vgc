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

#include <pybind11/functional.h>

#include <vgc/core/object.h>
#include <vgc/core/wraps/common.h>
#include <vgc/core/wraps/object.h>
#include <vgc/core/wraps/signal.h>

namespace vgc::core::wraps {

Int getFunctionArity(const py::function& method) {
    auto inspect = py::module::import("inspect");
    return getFunctionArity(inspect, method);
}

Int getFunctionArity(py::handle inspect, const py::function& method) {
    // Check that it only has a fixed count of arguments.
    py::object sig = inspect.attr("signature")(method);
    py::object parameter = inspect.attr("Parameter");
    py::handle POSITIONAL_ONLY = parameter.attr("POSITIONAL_ONLY");
    py::handle POSITIONAL_OR_KEYWORD = parameter.attr("POSITIONAL_OR_KEYWORD");
    py::iterable parameters = sig.attr("parameters").attr("values")();
    Int arity = 0;
    for (auto it = py::iter(parameters); it != py::iterator::sentinel(); ++it) {
        py::handle kind = it->attr("kind");
        if (kind.not_equal(POSITIONAL_ONLY) && kind.not_equal(POSITIONAL_OR_KEYWORD)) {
            throw py::value_error("Arity only apply to methods with no var-positional "
                                  "argument, nor kw-only arguments.");
        }
        ++arity;
    }
    return arity;
}

// Used to decorate a python signal method.
// Does something similar to what is done in ObjClass::defSignal().
//
py::object signalDecoratorFn(const py::function& signalMethod) {
    auto builtins = py::module::import("builtins");
    auto inspect = py::module::import("inspect");

    // Check it is a function (not a method yet since not processed by metaclass).
    if (!inspect.attr("isfunction")(signalMethod)) {
        throw py::value_error("@signal only apply to method declarations.");
    }

    Int arity = getFunctionArity(inspect, signalMethod);
    if (arity == 0) {
        throw py::value_error(
            "Python signal method expected to at least have 'self' parameter.");
    }
    --arity;
    if (arity > VGC_CORE_MAX_SIGNAL_ARGS) {
        throw py::value_error("Signals and slots are limited to " VGC_PP_XSTR(VGC_CORE_MAX_SIGNAL_ARGS) " arguments.");
    }

    // Create a new unique ID for this signal.
    auto newId = core::detail::genFunctionId();

    // Create the property getter
    py::str signalName = signalMethod.attr("__name__");
    py::cpp_function fget(
        [=](py::object self) -> PyPySignalRef* {
            // XXX more explicit error when self is not a core::Object.
            Object* this_ = self.cast<Object*>();
            // Create emit function.
            //
            // XXX can use py::name, py::doc if py::dynamic_attr doesn't let us use functools.update_wrapper
            //
            py::cpp_function emitFn(
                // Captures this_ -> must die before object dies.
                [=](py::args args) -> void {
                    // XXX add check enough args
                    using SignalArgRefsTuple = std::tuple<const py::args&>;
                    core::detail::SignalHub::emitFwd<SignalArgRefsTuple>(
                        this_, newId, args);
                });
            PyPySignalRef* sref = new PyPySignalRef(this_, newId, arity, emitFn);
            py::object pysref = py::cast(sref, py::return_value_policy::take_ownership);
            self.attr("__dict__")[signalName] = pysref; // caching
            return sref; // pybind will find the object in registered_instances
        },
        py::keep_alive<0, 1>());

    // Create the property
    auto prop = builtins.attr("property")(fget);

    return prop;
}

// Used to decorate a python slot method.
// Does something similar to what is done in ObjClass::defSlot().
//
py::object slotDecoratorFn(py::function unboundSlotMethod) {
    auto builtins = py::module::import("builtins");
    auto inspect = py::module::import("inspect");

    // Check it is a function (not a method yet since not processed by metaclass).
    if (!inspect.attr("isfunction")(unboundSlotMethod)) {
        throw py::value_error("@slot only apply to method declarations.");
    }

    Int arity = getFunctionArity(inspect, unboundSlotMethod);
    if (arity == 0) {
        throw py::value_error("Slot method expected to at least have 'self' parameter.");
    }
    --arity;
    if (arity > VGC_CORE_MAX_SIGNAL_ARGS) {
        throw py::value_error("Signals and slots are limited to " VGC_PP_XSTR(VGC_CORE_MAX_SIGNAL_ARGS) " arguments.");
    }

    // Create a new unique ID for this slot.
    auto newId = core::detail::genFunctionId();

    // Create the property getter
    py::str slotName = unboundSlotMethod.attr("__name__");
    py::cpp_function fget(
        [=](py::object self) -> PyPySlotRef* {
            // XXX more explicit error when self is not a core::Object.
            Object* this_ = self.cast<Object*>();
            PyPySlotRef* sref = new PyPySlotRef(this_, newId, arity, unboundSlotMethod);
            py::object pysref = py::cast(sref, py::return_value_policy::take_ownership);
            self.attr("__dict__")[slotName] = pysref; // caching
            return sref; // pybind will find the object in registered_instances
        },
        py::keep_alive<0, 1>());

    // Create the property
    auto prop = builtins.attr("property")(fget);

    return prop;
}

void wrapSignalAndSlotRefs(py::module& m) {
    auto pyConnectionHandle = py::class_<ConnectionHandle>(m, "ConnectionHandle");

    // XXX provide a getter for id too ?
    //     could be interesting to expose signal/slot info/stats to python.
    auto pyAbstractSlotRef =
        py::class_<PyAbstractSlotRef>(m, "AbstractSlotRef")
            .def_property_readonly(
                "object", [](PyAbstractSlotRef* this_) { return this_->object(); });

    auto pySlotRef = py::class_<PyPySlotRef, PyAbstractSlotRef>(m, "PySlotRef")
                         .def(
                             /* calling slot calls its method. */
                             "__call__",
                             [](py::object self, py::args args) {
                                 PyPySlotRef* this_ = self.cast<PyPySlotRef*>();
                                 this_->unboundPySlotFn()(this_->object(), *args);
                             });

    auto pySignalRef =
        py::class_<PyPySignalRef, PyAbstractSlotRef>(m, "PySignalRef")
            .def_property_readonly(
                "emit", [](PyPySignalRef* this_) { return this_->boundPyEmitFn(); })
            .def("connect", &PyPySignalRef::connect)
            .def("connect", &PyPySignalRef::connectCallback)
            .def("disconnect", &PyPySignalRef::disconnect)
            .def("disconnect", &PyPySignalRef::disconnectAll)
            .def("disconnect", &PyPySignalRef::disconnectSlot);

    auto pyAbstractCppSlotRef =
        py::class_<PyAbstractCppSlotRef, PyAbstractSlotRef>(m, "AbstractCppSlotRef");

    auto pyCppSlotRef = py::class_<PyCppSlotRef, PyAbstractCppSlotRef>(m, "CppSlotRef")
                            .def("__call__", [](py::object self, py::args args) {
                                PyCppSlotRef* this_ = self.cast<PyCppSlotRef*>();
                                this_->unboundPySlotFn()(this_->object(), *args);
                            });

    auto pyCppSignalRef =
        py::class_<PyCppSignalRef, PyAbstractCppSlotRef>(m, "CppSignalRef")
            .def(
                "emit",
                [](PyCppSignalRef* this_, py::args args) {
                    return this_->unboundPySlotFn()(this_->object(), *args);
                })
            .def("connect", &PyCppSignalRef::connect)
            .def("connect", &PyCppSignalRef::connectCallback)
            .def("disconnect", &PyCppSignalRef::disconnect)
            .def("disconnect", &PyCppSignalRef::disconnectAll)
            .def("disconnect", &PyCppSignalRef::disconnectSlot);
}

} // namespace vgc::core::wraps

void wrap_signal(py::module& m) {
    // ref types
    vgc::core::wraps::wrapSignalAndSlotRefs(m);

    // decorators
    m.def("signal", &vgc::core::wraps::signalDecoratorFn);
    m.def("slot", &vgc::core::wraps::slotDecoratorFn);
}
