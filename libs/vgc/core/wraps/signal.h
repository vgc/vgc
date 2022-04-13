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
// In our current implementation, signals and slots are property getters that creates a temporary slotref.
// Another possibility would be to pre-instanciate the slotrefs on object instanciation.

// Python function slot wrap

// some facts to help designing this part:
// 1) connect(..) is member of sigrefs
//     -> we only need an easy overload for connect that doesn't just try all
//     -> so we need a common type for all slots that we dispatch in C++ (with switch or dyn casts..)
// 

Int getFunctionArity(const py::function& method);
Int getFunctionArity(py::handle inspect, const py::function& method);

// Common py interface for Signals and Slots.
// XXX Requires pybind11's custom type setup for GC support.
class PyAbstractSlotRef {
public:
    using SignalTransmitter = core::internal::SignalTransmitter;
    using Id = core::internal::ObjectMethodId;

    PyAbstractSlotRef(
        const core::Object* obj,
        Id id,
        Int arity) :

        obj_(const_cast<core::Object*>(obj)),
        id_(id),
        arity_(arity) {

    }

    virtual ~PyAbstractSlotRef() = default;

    core::Object* object() const {
        return obj_;
    }

    const Id& id() const {
        return id_;
    }

    Int arity() const {
        return arity_;
    }

    virtual SignalTransmitter buildPyTransmitter() = 0;

protected:
    // Object bound to the slot.
    core::Object* const obj_; 
    // Unique identifier of the slot.
    Id id_;
    // Arity of the bound slot.
    Int arity_;
};

// C++ Slots wrapped in a cpp_function expect positional arguments and not a py::args.
// Thus in case of Py Signals the behavior is also to expand args before the transmit.
//
// For a C++ Signal, a SignalTransmitter can be built only with the knowledge of the Signal Args used by the Slot.
// C++ Signal -> C++ Slot : truncation of C++ arguments pack, see C++ impl.
// C++ Signal -> Py  Slot : same but the transmit forwards arguments to a py::function.
// For a Py Signal, pybind11 is responsible for the arguments conversion and testing.
// Our transmitter could either accept (py::args, arity) or a truncation of the expanded args.
// Py Signal -> C++ Slot : truncation of expanded python arguments pack. C++ transmit must be wrapped in cpp_function.
//                         Special case: Slot with 0 arguments. We can directly use the C++ transmit even with a Py Signal.
// Py Signal -> Py  Slot : truncation of expanded python arguments pack.
//

class PyPySlotRef : public PyAbstractSlotRef {
public:
    using Id = core::internal::ObjectMethodId;

    // 'self' does not count in arity.
    PyPySlotRef(
        const core::Object* obj,
        Id id,
        py::function unboundFn,
        Int arity) :

        PyAbstractSlotRef(obj, id, arity),
        unboundFn_(unboundFn) {

    }

    const py::function& unboundFn() const {
        return unboundFn_;
    }

    virtual SignalTransmitter buildPyTransmitter() override {
        py::object self = py::cast(object());
        if (arity_ == 0) {
            return SignalTransmitter(std::function<void()>(
                [=]() {
                    unboundFn_(self);
                }));
        }
        else {
            return SignalTransmitter(std::function<void(const py::args& args)>(
                [=](const py::args& args) {
                    auto newArgs = py::tuple(arity_);
                    for (Int i = 0; i < arity_; ++i) {
                        newArgs[i] = args[i];
                    }
                    unboundFn_(self, *newArgs);
                }));
        }
    }

protected:
    // Unbound slot py-method.
    py::function unboundFn_;
};

class PyPySignalRef : public PyAbstractSlotRef {
public:
    PyPySignalRef(
        const core::Object* obj,
        Id id,
        py::function boundEmitFn,
        Int arity) :

        PyAbstractSlotRef(obj, id, arity),
        boundEmitFn_(boundEmitFn) {

    }

    // XXX connects

    /*
    ConnectionHandle connect(SlotRefT&& slotRef)
    ConnectionHandle connect(FreeFunctionT&& callback)
    ConnectionHandle connect(Functor&& funcObj)
    bool disconnect()
    bool disconnect(ConnectionHandle h)
    bool disconnect(SlotRefT&& slotRef)
    */

    ConnectionHandle connect(PyAbstractSlotRef* slot) {
        if (arity() < slot->arity()) {
            throw py::value_error("The slot signature cannot be longer than the signal signature.");
        }
        // XXX "Signals and Slots are limited to 7 parameters."

        return core::internal::SignalHub::connect(object(), id(), slot->buildPyTransmitter(), core::internal::BoundObjectMethodId(slot->object(), slot->id()));
    }

    ConnectionHandle connectCallback(py::function callback) {
        auto inspect = py::module::import("inspect");
        Int arity = getFunctionArity(inspect, callback);
        return core::internal::SignalHub::connect(
            object(), id(),
            SignalTransmitter(std::function<void(const py::args& args)>(
                [=](const py::args& args) {
                    callback(*(args[py::slice(0, arity)]));
                })),
            std::monostate{});
    }

    const py::function& boundEmitFn() const {
        return boundEmitFn_;
    }

    virtual SignalTransmitter buildPyTransmitter() override {
        return SignalTransmitter(std::function<void(const py::args& args)>(
            [=](const py::args& args) {
                boundEmitFn_(args);
            }));
    }

protected:
    // Bound emit py-function.
    py::function boundEmitFn_;
};

// Holds a C++ transmitter, meant to be cached.
class PyAbstractCppSlotRef : public PyAbstractSlotRef {
public:
    using SignalTransmitter = core::internal::SignalTransmitter;

protected:
    template<typename Obj, typename... Args>
    PyAbstractCppSlotRef(
        const Obj* obj,
        const Id id,
        SignalTransmitter&& cppTransmitter,
        std::tuple<Args...>* sig) :

        PyAbstractSlotRef(obj, id, sizeof...(Args)),
        parameters_({std::type_index(typeid(Args))...}),
        cppTransmitter_(std::move(cppTransmitter)) {

    }

public:
    const auto& parameters() const {
        return parameters_;
    }
    
    const SignalTransmitter& cppTransmitter() const {
        return cppTransmitter_;
    }

private:
    std::vector<std::type_index> parameters_;
    SignalTransmitter cppTransmitter_;
};

// Should only be constructed from the return value of a Slot method defined
// with the VGC_SLOT macro.
//
class PyCppSlotRef : public PyAbstractCppSlotRef {
public:
    template<typename Method, std::enable_if_t<isMethod<RemoveCRef<Method>>, int> = 0>
    PyCppSlotRef(const Id id, Method&& method, typename MethodTraits<Method>::This obj) :
        PyAbstractCppSlotRef(
            obj,
            id,
            SignalTransmitter::build<typename CallableTraits<Method>::ArgsTuple>(method, obj),
            static_cast<typename CallableTraits<Method>::ArgsTuple*>(nullptr)) {

        //py::cpp_function(bindMethod(obj, method, static_cast<typename CallableTraits<Method>::ArgsTuple*>(nullptr))),

    }

    template<typename SlotRefT,
        std::enable_if_t<core::internal::isSlotRef<SlotRefT>, int> = 0>
    PyCppSlotRef(const SlotRefT& slotRef) :
        PyCppSlotRef(SlotRefT::id(), slotRef.method(), slotRef.object()) {
    
    }

    virtual SignalTransmitter buildPyTransmitter() override {
        // XXX todo
        return {};
    }

protected:
    template<typename Obj, typename Method, typename... MethodArgs>
    [[nodiscard]] static auto bindMethod(Obj* obj, Method&& method, std::tuple<MethodArgs...>*) {
        return std::function<void(MethodArgs...)>(
            [=](MethodArgs... args) {
                std::invoke(method, obj, std::forward<MethodArgs>(args)...);
            });
    }
};

// Should only be constructed from the return value of a Signal method defined
// with the VGC_SIGNAL macro.
//
class PyCppSignalRef : public PyAbstractCppSlotRef {
public:
    using SignalTransmitter = core::internal::SignalTransmitter;

protected:
    template<typename SignalRefT, typename ObjT, typename... Args,
        std::enable_if_t<core::internal::isSignalRef<SignalRefT>, int> = 0>
    PyCppSignalRef(const SignalRefT& signalRef, ObjT* object, std::tuple<Args...>* sig) :
        // SignalRefs are non-copyable to prevent ppl from storing them.
        // But that's what we need to do here. Solution is to rebuild one.
        PyAbstractCppSlotRef(
            object,
            signalRef.id(),
            SignalTransmitter::build<std::tuple<Args...>>(
                [=](Args&&... args) {
                    SignalRefT(object).emit(std::forward<Args>(args)...);
                }),
            static_cast<std::tuple<Args...>*>(nullptr)) {

    }

public:
    template<typename SignalRefT,
        std::enable_if_t<core::internal::isSignalRef<SignalRefT>, int> = 0>
    PyCppSignalRef(const SignalRefT& signalRef) :
        PyCppSignalRef(signalRef, signalRef.object(), static_cast<typename SignalRefT::ArgsTuple*>(nullptr)) {
    
    }

    const core::internal::SignalMethodId& id() const {
        return PyAbstractCppSlotRef::id();
    }

    virtual SignalTransmitter buildPyTransmitter() override {
        // XXX todo
        return {};
    }

    // XXX connects

private:
    std::function<SignalTransmitter*(py::function slot, Int arity)> transmitterFactory_;
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
