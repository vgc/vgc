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

#include <vgc/core/wraps/common.h>
#include <vgc/core/wraps/signal.h>
#include <vgc/core/wraps/object.h>
#include <vgc/core/object.h>

namespace vgc::core::wraps {

// Does something similar to what is done in ObjClass::defSignal().
py::object signalDecoratorFn(const py::function& signalMethod) {
    auto builtins = py::module::import("builtins");
    auto inspect = py::module::import("inspect");

    if (!inspect.attr("isfunction")(signalMethod)) {
        throw py::value_error("@signal only applies to functions");
    }

    py::str signalName = signalMethod.attr("__name__");

    // Create the property getter
    auto newId = core::internal::genObjectMethodId();
    py::cpp_function fget(
        [=](py::object self) -> PyPySignalRef* {
            // XXX more explicit error when self is not a core::Object.
            Object* this_ = self.cast<Object*>();
            PyPySignalRef* sref = new PyPySignalRef(this_, newId, signalMethod);
            py::object pysref = py::cast(sref, py::return_value_policy::take_ownership);
            py::setattr(self, signalName, pysref); // caching
            return sref; // pybind will find the object in registered_instances
        },
        py::keep_alive<0, 1>());

    // Create the property
    auto prop = builtins.attr("property")(fget);

    return prop;
}

// Does something similar to what is done in ObjClass::defSlot().
py::object slotDecoratorFn(const py::function& slotMethod) {
    auto builtins = py::module::import("builtins");
    auto inspect = py::module::import("inspect");

    if (!inspect.attr("isfunction")(slotMethod)) {
        throw py::value_error("@slot only applies to functions");
    }

    py::str slotName = slotMethod.attr("__name__");

    // Create the property getter
    auto newId = core::internal::genObjectMethodId();
    py::cpp_function fget(
        [=](py::object self) -> PyPySlotRef* {
            // XXX more explicit error when self is not a core::Object.
            Object* this_ = self.cast<Object*>();
            PyPySlotRef* sref = new PyPySlotRef(this_, newId, slotMethod);
            py::object pysref = py::cast(sref, py::return_value_policy::take_ownership);
            py::setattr(self, slotName, pysref); // caching
            return sref; // pybind will find the object in registered_instances
        },
        py::keep_alive<0, 1>());

    // Create the property
    auto prop = builtins.attr("property")(fget);

    return prop;
}


void wrapCppSlotRef(py::module& m)
{
   /* py::class_<AbstractCppSlotRef>(m, "AbstractCppSlotRef")
        .def("__call__", [](py::object self, py::args args) {
            auto ref = self.cast<AbstractCppSlotRef*>();
            ref->getPyWrappedBoundSlot().call(*args);
        })
    ;*/
}

void wrapCppSignalRef(py::module& m)
{
   // py::class_<AbstractCppSignalRef>(m, "AbstractCppSignalRef")
   //     .def("__call__", [](py::object self, py::args args) {
   //         auto ref = self.cast<AbstractCppSignalRef*>();
   //         // XXX emit
   //         
   //     })
   //;
}

// connect (signalRef, slotRef)




// simple adapter
py::cpp_function pySlotAdapter(py::args args, py::kwargs kwargs) {
    return {};
}


//template<typename... SignalArgs>
//[[nodiscard]] static inline
//auto createCppToPyTransmitter(py::cpp_function) {
//    return new core::internal::SignalTransmitter<SignalArgs...>(
//        [=](SignalArgs&&... args) {
//
//            // forward N args to cpp_function
//            // N must be retrieved from func slot attrs.. (if i can add them even for cpp slots..)
//        });
//}

// used for py-signals only ! 
//
class PyToPyTransmitter : public core::internal::AbstractSignalTransmitterOld {
public:
    py::function boundSlot_;
};

//

// cpp-signal binding must provide function to CREATE a transmitter from py signature'd slot

// cpp-signal overridden by py-signal can be checked in connect

// slot binding must define a py signature'd c++ wrapper.. for the above


// signal_impl(py::object, py::args args, const py::kwargs& kwargs

void transmit(py::object self, py::str, py::args, py::kwargs)
{
    std::cout << "transmit" << std::endl;
}



void testBoundCallback(const py::function& cb) {
    if (py::hasattr(cb, "__slot_tag__")) {
        // slot
        cb(42);
    }
    else {
        cb(-42);
    }
}


// wrapping Signal as an object instead of a class requires:
//  - shared state, so that emitting on a copied signal does signal on the same connections!

// signals should live either with their owner object, or by themselves..
// -> shared_ptr to state VS shared_ptr to owner.

// emit:
// obj.signal.emit()

VGC_DECLARE_OBJECT(TestWrapObject);

class TestWrapObject : public Object {
    VGC_OBJECT(TestWrapObject, Object)

public:
    static inline TestWrapObjectPtr create() {
        return TestWrapObjectPtr(new TestWrapObject());
    }

    VGC_SIGNAL(signalIDI, (int, a), (double, b), (int, c));
    
    /*
    VGC_SLOT(slotID, (int, a), (double, b)) {
        a_ = a;
        b_ = b;
    }*/

    int a_ = 0;
    double b_ = 0.;
};

} // namespace vgc::core::wraps

void wrap_signal(py::module& m)
{
    vgc::core::wraps::wrapCppSlotRef(m);

    m.def("signal", &vgc::core::wraps::signalDecoratorFn);
    m.def("slot", &vgc::core::wraps::slotDecoratorFn);

    m.def("testBoundCallback", &vgc::core::wraps::testBoundCallback);

    using UnsharedOwnerSignal = vgc::core::Signal<>;
    py::class_<UnsharedOwnerSignal> c(m, "Signal");

    /*c.def("connect",
        [](UnsharedOwnerSignal& a, const std::function<void()>& slot_func) {
            a.connect(slot_func);
        }
    );*/

    c.def("emit",
        [](UnsharedOwnerSignal& a) {
            a();
        }
    );

    c.def(py::init([]() {
        UnsharedOwnerSignal res;
        return res;
    }));

    {
        using vgc::core::wraps::TestWrapObject;
        auto c = vgc::core::wraps::ObjClass<TestWrapObject>(m, "TestWrapObject")
            .def(py::init([]() { return TestWrapObject::create(); }))
            .def_signal("signalIDI", &TestWrapObject::signalIDI)
            //.def_slot("slotID", &TestWrapObject::slotID)
            .def_readwrite("a", &TestWrapObject::a_)
            .def_readwrite("b", &TestWrapObject::b_)
        ;
    }
}


