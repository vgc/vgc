// Copyright 2022 The VGC Developers
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

#ifndef VGC_CORE_WRAPS_SIGNAL_H
#define VGC_CORE_WRAPS_SIGNAL_H

#include <functional>
#include <typeindex>

#include <vgc/core/wraps/common.h>
#include <vgc/core/object.h>

namespace vgc::core::wraps {

// The Signal/Slot api in python is:
//  - declaration:  @signal | @slot
//  - connect:      objectA.signalA.connect(objectB.slotB | function)
//  - emit:         objectA.signalA.emit(args...)
//  - slot call:    objectB.slotB(args...)
// 
// In our current implementation, signals and slots are functions that creates a temporary slotref.
// Another possibility would be to pre-instanciate the slotrefs on object instanciation.

// some facts to help designing this part:
// 1) connect(..) is member of sigrefs
//     -> we only need an easy overload for connect that doesn't just try all
//     -> so we need a common type for all slots that we dispatch in C++ (with switch or dyn casts..)
// 

// Common py interface for slots.
// Keeping a simple Object* is safe as long as instances are used as rvalues.
class PyAbstractSlotRef {
public:
    using Id = core::internal::ObjectMethodId;

    PyAbstractSlotRef(const core::Object* obj, const Id id) :
        obj_(const_cast<core::Object*>(obj)), id_(id) {}

    core::Object* object() const {
        return obj_;
    }

    const Id& id() const {
        return id_;
    }

private:
    core::Object* const obj_;
    const Id id_;
};

// For Slots/Signals declared in Python.
class PyPySlotRef : public PyAbstractSlotRef {
public:
    PyPySlotRef(
        const core::Object* obj,
        const Id id,
        const py::function fn) :

        PyAbstractSlotRef(obj, id),
        fn_(fn){}

    const py::function& getFunction() const {
        return fn_;
    }

private:
    const py::function fn_;
};


template<typename R, typename ArgsTuple>
struct BoundCppMethodSlotSig_;
template<typename R, typename... Args>
struct BoundCppMethodSlotSig_<R, std::tuple<Args...>> {
    using type = R(Args&&...);
};

//template<typename R, typename C, typename... Args>
//struct ObjectMethodTraits_<R (C::*)(Args...)> : FunctionTraitsDef_<C, false, R, Args...> {};
//template <typename R, typename C, typename... Args>
//struct ObjectMethodTraits_<R (C::*)(Args...) const> : FunctionTraitsDef_<C, false, R, Args...> {};
//template<typename T>
//using ObjectMethodTraits = ObjectMethodTraits_<std::remove_reference_t<T>>;

// a slotref is created from:
//  cpp: obj + method
//  py: through cpp decorator that builds a cpp type instance wrapping the py::function.

// an instance is created on each getattr..

// Holds a bound method.
// What matters is to at least defer the cpp_function instanciation.
// While we are at it why not deferring the object binding ?
// To do so, we can only use a polymorphic object.
// It seems more future-proof to implement it in a separate class.
class BoundCppMethodSlot {
protected:
    template<typename Fn, typename... Args>
    BoundCppMethodSlot(Fn&& fn, void()) :
        parameters_({std::type_index(typeid(Args))...}),
        pyfn_(fn), fn_(new std::remove_reference_t<Fn>(std::forward<Fn>(fn))) {}

public:
    /*template<typename Fn>
    BoundCppMethodSlot(
        std::initializer_list<std::type_index> parameters,
        py::cpp_function pyfn) :

        parameters_(parameters),
        pyfn_(pyfn) {} */

    const auto& parameters() const {
        return parameters_;
    }
    
    const py::cpp_function& getPyFunction() const {
        return pyfn_;
    }

private:
    const std::vector<std::type_index> parameters_;
    const py::cpp_function pyfn_;
    const std::unique_ptr<const void*> fn_;
};

// 
template<typename... Args>
class BoundObjectMethodSlotT : public AbstractBoundObjectMethodSlot {
public:
    using FunctionSig = void(Args&&...);
    using Function = std::function<FunctionSig>;

    template<typename Fn>
    BoundObjectMethodSlot(Fn&& fn) :
        AbstractBoundObjectMethodSlot(
            {std::type_index(typeid(Args))...},
            py::cpp_function(fn)),
        fn_(std::forward<Fn>(fn)) {}

    const Function fn_;
};

class PyAbstractCppSlotRef : public PyAbstractSlotRef {
public:
    PyAbstractCppSlotRef(
        const core::Object* obj,
        const core::internal::ObjectMethodId id,
        std::unique_ptr<const AbstractBoundObjectMethodSlot>&& afn) :

        PyAbstractSlotRef(obj, id),
        afn_(std::move(afn)) {}

    const auto& parameters() const {
        return afn_->parameters();
    }

private:
    const std::unique_ptr<const AbstractBoundObjectMethodSlot> afn_;
};

// XXX instead of a templated child, let's do a makePyCppSlotRef..




//template<typename Obj, typename... SlotArgs>
//class PyCppSlotRef : public PyAbstractCppSlotRef<SlotArgs...> {
//private:
//    using SuperClass = PyAbstractCppSlotRef<SlotArgs...>;
//
//public:
//    using MFnT = void (Obj::*)(SlotArgs...);
//    using BoundFunction = typename SuperClass::BoundFunction;
//
//    PyCppSlotRef(const Obj* obj, core::internal::SlotId id, MFnT mfn) :
//        SuperClass(obj, id),
//        mfn_(mfn) {}
//
//    virtual BoundFunction getBoundFunction() const override {
//        // XXX can add alive check here or in the lambda..
//        using MFnT = void (Obj::*)(SlotArgs...);
//        return [=](SlotArgs&&... args) {
//            (static_cast<Obj*>(object())->*mfn_)(std::forward<SlotArgs>(args)...);
//        };
//    }
//
//private:
//    MFnT mfn_;
//};



class PyAbstractCppSignalRef : public PyAbstractCppConnectableRef {
private:
    using SuperClass = PyAbstractCppConnectableRef;

public:
    using SuperClass::SuperClass;

    const core::internal::SignalId& id() const {
        return SuperClass::id();
    }

    // XXX do getBoundFunction, that is the bound emit

protected:
    virtual [[nodiscard]] core::internal::AbstractSignalTransmitter*
    createTransmitter(PyAbstractCppConnectableRef* connectableRef) const = 0;
};



// XXX split this, we want a fallback to python if basic compatibility is met (slot sig not too long)
//void checkCompatibility(const PyAbstractCppSignalRef* signalRef, const PyAbstractCppConnectableRef* connectableRef) {
//    const auto& signalParams = signalRef->parameters();
//    const auto& cParams = connectableRef->parameters();
//    if (cParams.size() > signalParams.size()) {
//        // XXX subclass LogicError
//        throw LogicError("Handler signature is too long for this Signal.");
//    }
//    if (!std::equal(cParams.begin(), slotParams.end(), signalParams.begin())) {
//        // XXX subclass LogicError
//        throw LogicError("Slot/Signal signature mismatch.");
//    }
//}

#define CREATE_TRANSMITTER_SWITCH_CASE(i)                                   \
    case i:                                                                 \
        if constexpr (sizeof...(SignalArgs) >= i) {                         \
            using SlotRefType = CppSlotRefTypeFromArgsTuple<                \
                core::internal::SubPackAsTuple<0, i, SignalArgs...>>;       \
            auto castedSlotRef = static_cast<SlotRefType*>(slotRef);        \
            return TransmitterType::create(castedSlotRef->getBoundSlot());  \
        }                                                                   \
        [[fallthrough]];



template<typename... SignalArgs>
class PyCppSignalRef : public PyAbstractCppSignalRef {
private:
    using TransmitterType = vgc::core::internal::SignalTransmitter<SignalArgs...>;

public:
    PyCppSignalRef(const Object* obj, const core::internal::SignalId& id) :
        PyAbstractCppSignalRef(obj, id, {std::type_index(typeid(SignalArgs))...}) {}

    virtual [[nodiscard]] core::internal::AbstractSignalTransmitter*
    createTransmitter(AbstractCppSlotRef* slotRef) const override {

        // XXX add fallback to python transmitter

        //checkCompatibility(this, slotRef);
        const auto& slotParams = slotRef->parameters();
        switch (slotParams.size()) {
        CREATE_TRANSMITTER_SWITCH_CASE(0);
        CREATE_TRANSMITTER_SWITCH_CASE(1);
        CREATE_TRANSMITTER_SWITCH_CASE(2);
        CREATE_TRANSMITTER_SWITCH_CASE(3);
        CREATE_TRANSMITTER_SWITCH_CASE(4);
        CREATE_TRANSMITTER_SWITCH_CASE(5);
        default:
            return nullptr;
        }
    }
};

#undef CREATE_TRANSMITTER_SWITCH_CASE

// to be used with connect
// should define emit(...)

class PySignal {
private:
    using ImplFn = void (*)(Object*, const py::args& args);

public:
    template<typename... Args>
    PySignal(Object* o, Signal<Args...>* s) :
        o_(o), s_(s) {
        f_ = emitImpl<Args...>;
    }

private:
    Object* o_;
    void* s_;
    ImplFn f_;

    template<typename... Args>
    static void emitImpl(Object* o, const py::args& args)
    {
        if (!o->isAlive()) {
            throw vgc::core::NotAliveError(obj);
        }

    }
};

} // namespace vgc::core::wraps

#endif // VGC_CORE_WRAPS_SIGNAL_H
