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
#include <vector>

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

// Python function slot wrap

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
// Requires pybind11's custom type setup for GC support.
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

class PyPySignalRef : public PyPySlotRef {
    using PyPySlotRef::PyPySlotRef;

    ConnectionHandle connect(PyPySlotRef* slot) {

    }

    // XXX need to dynamically assign the emit to have a custom __doc__..
    const py::function emitFn;
};



// Holds a bound method, meant to be cached.
// Requires pybind11's custom type setup for GC support.
class PyCppMethodSlotRef : public PyAbstractSlotRef {
public:
    using SignalTransmitter = core::internal::SignalTransmitter;

protected:
    template<typename Obj, typename... Args>
    PyCppMethodSlotRef(
        const Obj* obj,
        const Id id,
        SignalTransmitter&& transmitter,
        std::tuple<Args...>* sig) :

        PyAbstractSlotRef(obj, id),
        parameters_({std::type_index(typeid(Args))...}),
        transmitter_(std::move(transmitter)) {
    
        pyfn_ = py::cpp_function(transmitter_.getFn<std::tuple<Args&&...>>());
    }

public:
    template<typename Method, std::enable_if_t<isMethod<RemoveCRef<Method>>, int> = 0>
    PyCppMethodSlotRef(const Id id, Method&& method, typename MethodTraits<Method>::This obj) :
        PyCppMethodSlotRef(
            obj,
            id,
            SignalTransmitter::build<typename CallableTraits<Method>::ArgsTuple>(method, obj),
            static_cast<typename CallableTraits<Method>::ArgsTuple*>(nullptr)) {

    }

    template<typename SlotRefT,
        std::enable_if_t<core::internal::isSlotRef<SlotRefT>, int> = 0>
    PyCppMethodSlotRef(const SlotRefT& slotRef) :
        PyCppMethodSlotRef(SlotRefT::id(), slotRef.method(), slotRef.object()) {}

    const auto& parameters() const {
        return parameters_;
    }
    
    const SignalTransmitter& getTransmitter() const {
        return transmitter_;
    }

    const py::cpp_function& getPyFunction() const {
        return pyfn_;
    }

private:
    std::vector<std::type_index> parameters_;
    SignalTransmitter transmitter_;
    py::cpp_function pyfn_;
};


// Should only be constructed from the return value of a Signal method defined
// with the VGC_SIGNAL macro.
//
class PyCppSignalRef : public PyCppMethodSlotRef {
public:
    using SignalTransmitter = core::internal::SignalTransmitter;

protected:
    template<typename SignalRefT, typename ObjT, typename... Args,
        std::enable_if_t<core::internal::isSignalRef<SignalRefT>, int> = 0>
    PyCppSignalRef(const SignalRefT& signalRef, ObjT* object, std::tuple<Args...>* sig) :
        // SignalRefs are non-copyable to prevent ppl from storing them.
        // But that's what we need to do here. Solution is to rebuild one.
        PyCppMethodSlotRef(
            object,
            signalRef.id(),
            SignalTransmitter::build<std::tuple<Args...>>(
                [=](Args&&... args){
                    VGC_EMIT SignalRefT(object).emit(std::forward<Args>(args)...);
                }),
            static_cast<std::tuple<Args...>*>(nullptr)) {

    }

public:
    template<typename SignalRefT,
        std::enable_if_t<core::internal::isSignalRef<SignalRefT>, int> = 0>
    PyCppSignalRef(const SignalRefT& signalRef) :
        PyCppSignalRef(signalRef, signalRef.object(), static_cast<typename SignalRefT::ArgsTuple*>(nullptr)) {}

    const core::internal::SignalMethodId& id() const {
        return PyCppMethodSlotRef::id();
    }

    // XXX connects
};


// left todo: 2 decorators + connect/disconnect..

// I need special transmitters for py signals !!
// The only way is to re-wrap, let's do that in connect.

// to bind cpp signal to py slot -> create a transmitter from lambda that captures the py slot ref, do the connect with the slot it !















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


// to be used with connect
// should define emit(...)

// simple test impl, temporary
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
            throw vgc::core::NotAliveError(o);
        }

    }
};

} // namespace vgc::core::wraps

#endif // VGC_CORE_WRAPS_SIGNAL_H
