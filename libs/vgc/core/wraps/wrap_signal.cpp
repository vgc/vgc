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
#include <vgc/core/signal.h>
#include <vgc/core/object.h>

namespace vgc::core::wraps {

class TestWrapObject : vgc::core::Object
{
    VGC_OBJECT(TestWrapObject, vgc::core::Object)

    VGC_SIGNAL(signalIFI, (int, a), (float, b), (int, c));
    VGC_SLOT(slotID, (int, a), (double, b));
};

// simple adapter
py::cpp_function pySlotAdapter(py::args args, py::kwargs kwargs) {
    return {};
}

template<typename... SignalArgs>
[[nodiscard]] static inline
auto createCppToPyTransmitter(py::cpp_function) {
    return new core::internal::SignalTransmitter<SignalArgs...>(
        [=](SignalArgs&&... args) {
            
            // forward N args to cpp_function
            // N must be retrieved from func slot attrs.. (if i can add them even for cpp slots..)
        });
}

// used for py-signals only ! 
//
class PyToPyTransmitter : public ::vgc::core::internal::AbstractSignalTransmitter {
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

py::cpp_function signalDecorator(const py::function& signal) {

    py::str signalName = signal.attr("__name__");
    py::cpp_function wrapper([=](py::object self, py::args args, py::kwargs kwargs) {
        transmit(self, signalName, args, kwargs);

        // XXX test
        return 2;
    });

    // XXX impossible this way, maybe do it manually via Extra args of cpp_function constructor
    // 
    //py::function update_wrapper = py::module::import("functools").attr("update_wrapper");
    //update_wrapper(wrapper, signal);

    return wrapper;
}


// SlotRef doit contenir une définition générique de la fonction slot pour qu'il n'y ait qu'un seul layout
// sinon on doit partir en virtuel..


// transmitter will receive forwarded signal args..
// need easy way to build transmitter from slotref...


class AbstractCppSlotRef {
protected:
    AbstractCppSlotRef(ObjectPtr obj, std::initializer_list<std::type_index> parameters) :
        obj(obj), parameters(parameters) {}

public:
    std::vector<std::type_index> parameters;
    ObjectPtr obj;

    virtual ~AbstractCppSlotRef() = default;
    virtual py::cpp_function getPyWrappedBoundSlot() = 0;
};

// XXX could cache bound slot and cpp_function..

template<typename... SlotArgs>
class CppSlotRef : public AbstractCppSlotRef {
public:
    using BoundSlot = void(SlotArgs...);

    using AbstractCppSlotRef::AbstractCppSlotRef;

    std::function<BoundSlot> getBoundSlot() {
        return bind_(obj.get());
    }
    
    virtual py::cpp_function getPyWrappedBoundSlot() override {
        return py::cpp_function(getBoundSlot());
    }

private:
    virtual std::function<BoundSlot> bind_(ObjectPtr obj) = 0;
};

template<typename Obj, typename... SlotArgs>
class CppSlotRefImpl : public CppSlotRef<SlotArgs...> {
    using SuperClass = CppSlotRef<SlotArgs...>;
    using MFnT = void (Obj::*)(SlotArgs...);

public:
    CppSlotRefImpl(ObjPtr<Obj>* obj, MFnT mfn) :
        SuperClass(obj, { std::type_index(typeid(SlotArgs))... }),
        mfn_(mfn) {}

private:
    MFnT mfn_;

    virtual BoundSlot bind_(ObjectPtr obj) {
        // XXX can add alive check here or in the lambda..
        using MFnT = void (Obj::*)(SlotArgs...);
        return [obj](SlotArgs&&... args) {
            auto mfn = *reinterpret_cast<MFnT*>(&pmfn_);
            mfn(obj, std::forward<SlotArgs>(args));
        };
    }
};


class AbstractCppSignalRef {
public:
    std::vector<std::type_index> parameters;

    ~AbstractCppSignalRef() = default;

    virtual std::unique_ptr<vgc::core::internal::AbstractSignalTransmitter> createTransmitter(AbstractCppSlotRef* slotRef) = 0;
};



template<typename... SignalArgs, size_t SlotArgCount>
std::unique_ptr<vgc::core::internal::AbstractSignalTransmitter> createTransmitter_(AbstractCppSlotRef* slotRef) {

    const auto& slotParams = slotRef->parameters;

    if (parameters.size() < slotParams.size()) {
        // XXX error wrong sig
        return nullptr;
    }
    if (!std::equal(slotParams.begin(), slotParams.end(), parameters.begin())) {
        // XXX error wrong sig
        return nullptr;
    }

    // here implementation dispatch..
    vgc::core::SignalTransmitter<SignalArgs...>::create()
}


template<typename... SignalArgs>
struct CppSignalRef : public AbstractCppSignalRef {
    using TransmitterType = vgc::core::internal::SignalTransmitter<SignalArgs...>;

    virtual std::unique_ptr<vgc::core::internal::AbstractSignalTransmitter> createTransmitter(AbstractCppSlotRef* slotRef) override {
        
        const auto& slotParams = slotRef->parameters;

        if (parameters.size() < slotParams.size()) {
            // XXX error wrong sig
            return nullptr;
        }
        if (!std::equal(slotParams.begin(), slotParams.end(), parameters.begin())) {
            // XXX error wrong sig
            return nullptr;
        }

        // here implementation dispatch..

    }
};


// connect (signalRef, slotRef)


py::function slotDecorator(const py::function& slot) {
    slot.attr("__slot_tag__") = 1;
    return slot;
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

} // namespace vgc::core::wraps

void wrap_signal(py::module& m)
{
    m.def("signal", &vgc::core::wraps::signalDecorator);
    m.def("slot", &vgc::core::wraps::slotDecorator);
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
}


