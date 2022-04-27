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

Int getFunctionArity(const py::function& method);
Int getFunctionArity(py::handle inspect, const py::function& method);

/*template<typename Obj, typename Method, typename... MethodArgs>
[[nodiscard]] static auto bindMethod(Obj* obj, Method&& method, std::tuple<MethodArgs...>*) {
    return std::function<void(MethodArgs...)>(
        [=](MethodArgs... args) {
            std::invoke(method, obj, std::forward<MethodArgs>(args)...);
        });
}*/

using SignalTransmitter = core::internal::SignalTransmitter;
using SignalArgRefsArray = core::internal::SignalArgRefsArray;

// Common py interface for Signals and Slots.
class PyAbstractSlotRef {
public:
    using Id = core::internal::FunctionId;

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
    SignalTransmitter buildPyTransmitterFromUnboundPyFn(py::function pyFn) {
        py::handle self = py::cast(object());
        Int n = arity();
        if (n == 0) {
            return SignalTransmitter(
                [=](const SignalArgRefsArray& args) {
                    pyFn(self);
                });
        }
        else {
            return SignalTransmitter(
                [=](const SignalArgRefsArray& x) {
                    py::args args = x.get<py::args>(0);
                    auto newArgs = py::tuple(n);
                    for (Int i = 0; i < n; ++i) {
                        newArgs[i] = args[i];
                    }
                    pyFn(self, *newArgs);
                });
        }
    }

private:
    // Object bound to the slot.
    core::Object* const obj_; 
    // Unique identifier of the slot.
    Id id_;
    // Arity of the bound slot.
    Int arity_;
};


class PyPySlotRef : public PyAbstractSlotRef {
public:
    using Id = core::internal::FunctionId;

    // 'self' does not count in arity.
    PyPySlotRef(
        const core::Object* obj,
        Id id,
        py::function unboundPyFn,
        Int arity) :

        PyAbstractSlotRef(obj, id, arity),
        unboundPyFn_(unboundPyFn) {

    }

    const py::function& unboundPyFn() const {
        return unboundPyFn_;
    }

    virtual SignalTransmitter buildPyTransmitter() override {
        return buildPyTransmitterFromUnboundPyFn(unboundPyFn_);
    }

private:
    // Unbound slot py-method.
    py::function unboundPyFn_;
};

class PyPySignalRef : public PyAbstractSlotRef {
public:
    PyPySignalRef(
        const core::Object* obj,
        Id id,
        py::function boundEmitPyFn,
        Int arity) :

        PyAbstractSlotRef(obj, id, arity),
        boundEmitPyFn_(boundEmitPyFn) {

    }

    ConnectionHandle connect(PyAbstractSlotRef* slot) {
        if (arity() < slot->arity()) {
            throw py::value_error("The slot signature cannot be longer than the signal signature.");
        }
        // XXX "Signals and Slots are limited to 7 parameters."

        return core::internal::SignalHub::connect(object(), id(), slot->buildPyTransmitter(), core::internal::ObjectSlotId(slot->object(), slot->id()));
    }

    ConnectionHandle connectCallback(py::function callback) {
        auto inspect = py::module::import("inspect");
        Int arity = getFunctionArity(inspect, callback);
        return core::internal::SignalHub::connect(
            object(), id(),
            SignalTransmitter(
                [=](const SignalArgRefsArray& x) {
                    py::args args = x.get<py::args>(0);
                    callback(*(args[py::slice(0, arity)]));
                }),
            std::monostate{});
    }

    const py::function& boundEmitPyFn() const {
        return boundEmitPyFn_;
    }

    virtual SignalTransmitter buildPyTransmitter() override {
        return SignalTransmitter(
            [=](const SignalArgRefsArray& x) {
                py::args args = x.get<py::args>(0);
                boundEmitPyFn_(args);
            });
    }

protected:
    // Bound emit py-function.
    py::function boundEmitPyFn_;
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
        py::function unboundPyFn,
        std::tuple<Args...>* sig) :

        PyAbstractSlotRef(obj, id, sizeof...(Args)),
        parameters_({std::type_index(typeid(Args))...}),
        unboundPyFn_(unboundPyFn) {

    }

public:
    const auto& parameters() const {
        return parameters_;
    }

    const py::function& unboundPyFn() const {
        return unboundPyFn_;
    }

    virtual SignalTransmitter buildPyTransmitter() override {
        return buildPyTransmitterFromUnboundPyFn(unboundPyFn_);
    }

    virtual SignalTransmitter buildCppTransmitter() = 0;

private:
    // XXX change to span, and hold a static array in the cpp ref.
    std::vector<std::type_index> parameters_;
    // Unbound slot py-method.
    py::function unboundPyFn_;
};

// Should only be constructed from the return value of a Slot method defined
// with the VGC_SLOT macro.
//
class PyCppSlotRef : public PyAbstractCppSlotRef {
    using PyAbstractCppSlotRef::PyAbstractCppSlotRef;
};

template<typename Method, std::enable_if_t<isMethod<RemoveCVRef<Method>>, int> = 0>
class PyCppSlotRefImpl : public PyCppSlotRef {
public:
    using ArgsTuple = typename MethodTraits<Method>::ArgsTuple;
    using Obj = typename MethodTraits<Method>::Obj;
    using This = typename MethodTraits<Method>::This;

    PyCppSlotRefImpl(const Id id, Method method, This obj) :
        PyCppSlotRef(obj, id, py::cpp_function(method), static_cast<ArgsTuple*>(nullptr)),
        method_(method) {}

    template<typename SlotRefT, std::enable_if_t<
        core::internal::isSlotRef<SlotRefT> &&
        std::is_same_v<typename SlotRefT::SlotMethod, Method>, int> = 0>
    PyCppSlotRefImpl(const SlotRefT& slotRef) :
        PyCppSlotRefImpl(SlotRefT::id(), slotRef.method(), slotRef.object()) {}

    virtual SignalTransmitter buildCppTransmitter() override {
        return SignalTransmitter::build<ArgsTuple>(method_, static_cast<Obj*>(object()));
    }

private:
    Method method_;
};

class PyCppSignalRef : public PyAbstractCppSlotRef {
    using PyAbstractCppSlotRef::PyAbstractCppSlotRef;
};

// Should only be constructed from the return value of a Signal method defined
// with the VGC_SIGNAL macro.
//
template<typename SignalRefT, std::enable_if_t<core::internal::isSignalRef<SignalRefT>, int> = 0>
class PyCppSignalRefImpl : public PyCppSignalRef {
public:
    using ArgsTuple = typename SignalRefT::ArgsTuple;
    using Obj = typename SignalRefT::Obj;

protected:
    template<typename... Args>
    PyCppSignalRefImpl(const SignalRefT& signalRef, Obj* object, std::tuple<Args...>* sig) :
        PyCppSignalRef(object, signalRef.id(), buildUnboundEmitPyFn(sig), sig) {

        cppToPyTransmitterFactory_ = buildCppToPyTransmitterFactory<Args...>();
    }

public:
    PyCppSignalRefImpl(const SignalRefT& signalRef) :
        PyCppSignalRefImpl(signalRef, signalRef.object(), static_cast<ArgsTuple*>(nullptr)) {}

    const core::internal::SignalId& id() const {
        return PyCppSignalRef::id();
    }

    virtual SignalTransmitter buildCppTransmitter() override {
        return SignalTransmitter(buildCppTransmitFn(static_cast<ArgsTuple*>(nullptr)));
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

        core::internal::ObjectSlotId slotId(slot->object(), slot->id());

        auto cppSlot = dynamic_cast<PyAbstractCppSlotRef*>(slot);
        if (cppSlot != nullptr) {
            const auto& signalParams = parameters();
            const auto& slotParams = cppSlot->parameters();
            if (!std::equal(slotParams.begin(), slotParams.end(), signalParams)) {
                throw py::value_error("The slot signature is not compatible with the signal.");
            }

            return core::internal::SignalHub::connect(
                object(), id(),
                cppSlot->buildCppTransmitter(),
                slotId);
        }

        auto pySlot = dynamic_cast<PyPySlotRef*>(slot);
        if (pySlot != nullptr) {
            return core::internal::SignalHub::connect(
                object(), id(), 
                cppToPyTransmitterFactory_(slot->object(), pySlot->unboundPyFn(), pySlot->arity()),
                slotId);
        }

        auto pySignal = dynamic_cast<PyPySignalRef*>(slot);
        if (pySignal != nullptr) {
            return core::internal::SignalHub::connect(
                object(), id(), 
                cppToPyTransmitterFactory_(py::none(), pySignal->boundEmitPyFn(), pySignal->arity()),
                slotId);
        }

        throw py::value_error("Unsupported subclass of PyAbstractSlotRef.");
    }

    ConnectionHandle connectCallback(py::function callback) {
        auto inspect = py::module::import("inspect");
        Int arity = getFunctionArity(inspect, callback);
        return core::internal::SignalHub::connect(
            object(), id(),
            cppToPyTransmitterFactory_(py::none(), callback, arity),
            std::monostate{});
    }

protected:
    template<typename... Args>
    py::function buildUnboundEmitPyFn(std::tuple<Args...>* sig) {
        return py::cpp_function(
            [=](Obj* this_, Args... args) {
                SignalRefT(this_).emit(std::forward<Args>(args)...);
            });
    }

    template<typename... Args>
    auto buildCppTransmitFn(std::tuple<Args...>* sig) {
        Obj* this_ = static_cast<Obj*>(object());
        // SignalRefs are non-copyable to prevent ppl from storing them.
        // But that's what we need to do here. Solution is to rebuild one.
        return std::function<void(Args&&...)>(
            [=](Args&&... args) {
                SignalRefT(this_).emit(std::forward<Args>(args)...);
            });
    }

    using CppToPyTransmitterFactoryFn = std::function<SignalTransmitter(py::handle obj, py::function slot, Int arity)>;

    template<typename... SignalArgs>
    static CppToPyTransmitterFactoryFn buildCppToPyTransmitterFactory() {
        return [](py::handle obj, py::function slot, Int arity) -> SignalTransmitter {
            using SignalArgsTuple = std::tuple<SignalArgs...>;
            switch (arity) {
            case 0: {
                return SignalTransmitter(getZeroArgsPySlotFn(obj, slot));
                break;
            }

#define VGC_TRANSMITTER_FACTORY_CASE(i)                                                             \
            case i: if constexpr (sizeof...(SignalArgs) >= i) {                                     \
                using TruncatedSignalArgsTuple = SubTuple<0, i, SignalArgsTuple>;                   \
                return SignalTransmitter(getForwardingToPyFn(                                       \
                    obj, slot, static_cast<TruncatedSignalArgsTuple*>(nullptr)));                   \
                break;                                                                              \
            }                                                                                       \
            [[fallthrough]]

            VGC_TRANSMITTER_FACTORY_CASE(1);
            VGC_TRANSMITTER_FACTORY_CASE(2);
            VGC_TRANSMITTER_FACTORY_CASE(3);
            VGC_TRANSMITTER_FACTORY_CASE(4);
            VGC_TRANSMITTER_FACTORY_CASE(5);
            VGC_TRANSMITTER_FACTORY_CASE(6);
            VGC_TRANSMITTER_FACTORY_CASE(7);

#undef VGC_TRANSMITTER_FACTORY_CASE

            default:
                throw core::LogicError("The slot signature cannot be longer than the signal signature.");
                break;
            }
        };
    }

    static auto getZeroArgsPySlotFn(py::handle obj, py::function slot) {
        if (obj.is_none()) {
            return std::function<void()>([=](){ slot(); });
        }
        return std::function<void()>([=](){ slot(obj); });
    }

    template<typename... Args>
    static auto getForwardingToPyFn(py::handle obj, py::function slot, std::tuple<Args...>*) {
        if (obj.is_none()) {
            return std::function<void(Args&&... args)>(
                [=](Args&&... args) {
                    slot(args...);
                });
        }
        return std::function<void(Args&&... args)>(
            [=](Args&&... args) {
                slot(obj, args...);
            });
    }

private:
    CppToPyTransmitterFactoryFn cppToPyTransmitterFactory_;
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
