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

#include <vgc/core/detail/signal.h>
#include <vgc/core/object.h>
#include <vgc/core/wraps/common.h>

namespace vgc::core::wraps {

using SignalTransmitter = core::detail::SignalTransmitter;
using TransmitArgs = core::detail::TransmitArgs;

// The signal/slot API in python is:
//  - declaration:  @signal | @slot
//  - connect:      objectA.signalA.connect(objectB.slotB | objectB.signalB | function)
//  - emit:         objectA.signalA.emit(args...)
//  - slot call:    objectB.slotB(args...)
//
// In our current implementation, signals and slots are property getters that
// creates and caches a slotref. Another possibility would be to pre-instanciate
// the slotrefs on object instanciation.

Int getFunctionArity(const py::function& method);
Int getFunctionArity(py::handle inspect, const py::function& method);

inline py::tuple truncatePyArgs(const py::args& args, Int n) {
    auto ret = py::tuple(n);
    for (Int i = 0; i < n; ++i) {
        ret[i] = args[i];
    }
    return ret;
}

// Common interface for Python signals and slots.
class PyAbstractSlotRef {
public:
    using Id = core::detail::FunctionId;

    PyAbstractSlotRef(const core::Object* obj, Id id, Int arity)
        : obj_(const_cast<core::Object*>(obj))
        , id_(id)
        , arity_(arity) {
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

    // Builds a transmitter accepting a py::args argument.
    virtual SignalTransmitter buildPyTransmitter() = 0;

protected:
    SignalTransmitter buildPyTransmitterFromUnboundPySlotFn(py::function pySlotFn) {
        py::handle self = py::cast(object());
        Int n = arity();
        if (n == 0) {
            return [=](const TransmitArgs&) { pySlotFn(self); };
        }
        return [=](const TransmitArgs& x) {
            py::args args = x.get<const py::args&>(0);
            pySlotFn(self, *truncatePyArgs(args, n));
        };
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
    using Id = core::detail::FunctionId;

    // 'self' does not count in arity.
    PyPySlotRef(const core::Object* obj, Id id, Int arity, py::function unboundPySlotFn)
        : PyAbstractSlotRef(obj, id, arity)
        , unboundPySlotFn_(unboundPySlotFn) {
    }

    const py::function& unboundPySlotFn() const {
        return unboundPySlotFn_;
    }

    virtual SignalTransmitter buildPyTransmitter() override {
        return buildPyTransmitterFromUnboundPySlotFn(unboundPySlotFn_);
    }

private:
    // Unbound slot py-method.
    py::function unboundPySlotFn_;
};

class PyPySignalRef : public PyAbstractSlotRef {
public:
    PyPySignalRef(const core::Object* obj, Id id, Int arity, py::function boundPyEmitFn)
        : PyAbstractSlotRef(obj, id, arity)
        , boundPyEmitFn_(boundPyEmitFn) {
    }

    const py::function& boundPyEmitFn() const {
        return boundPyEmitFn_;
    }

    virtual SignalTransmitter buildPyTransmitter() override {
        vgc::core::Object* this_ = object();
        Id idcopy = id(); // copying for capturing in the lambda
        return SignalTransmitter([=](const TransmitArgs& x) {
            py::args args = x.get<const py::args&>(0);
            using SignalArgRefsTuple = std::tuple<const py::args&>;
            core::detail::SignalHub::emitFwd<SignalArgRefsTuple>(this_, idcopy, args);
        });
    }

    ConnectionHandle connect(PyAbstractSlotRef* slot) {
        if (arity() < slot->arity()) {
            throw py::value_error(
                "The slot signature cannot be longer than the signal signature.");
        }
        core::detail::ObjectSlotId slotId(slot->object(), slot->id());
        return core::detail::SignalHub::connect(
            object(), id(), slot->buildPyTransmitter(), slotId);
    }

    ConnectionHandle connectCallback(py::function callback) {
        auto inspect = py::module::import("inspect");
        Int slotArity = getFunctionArity(inspect, callback);
        if (arity() < slotArity) {
            throw py::value_error(
                "The slot signature cannot be longer than the signal signature.");
        }
        return core::detail::SignalHub::connect(
            object(),
            id(),
            SignalTransmitter([=](const TransmitArgs& x) {
                py::args args = x.get<const py::args&>(0);
                callback(*truncatePyArgs(args, slotArity));
            }),
            std::monostate{});
    }

    bool disconnect(ConnectionHandle h) {
        return core::detail::SignalHub::disconnect(object(), id(), h);
    }

    bool disconnectAll() {
        return core::detail::SignalHub::disconnect(object(), id());
    }

    bool disconnectSlot(PyAbstractSlotRef* slotRef) const {
        return core::detail::SignalHub::disconnect(
            object(),
            id(),
            vgc::core::detail::ObjectSlotId(slotRef->object(), slotRef->id()));
    }

protected:
    // Bound emit py-function.
    py::function boundPyEmitFn_;
};

// Holds a C++ transmitter, meant to be cached.
class PyAbstractCppSlotRef : public PyAbstractSlotRef {
public:
    using SignalTransmitter = core::detail::SignalTransmitter;

protected:
    template<typename Obj, typename... ArgRefs>
    PyAbstractCppSlotRef(
        const Obj* obj,
        const Id id,
        py::function unboundPySlotFn,
        std::tuple<ArgRefs...>* /*sig*/)

        : PyAbstractSlotRef(obj, id, sizeof...(ArgRefs))
        , parameters_({std::type_index(typeid(ArgRefs))...})
        , unboundPySlotFn_(unboundPySlotFn) {
    }

public:
    const auto& parameters() const {
        return parameters_;
    }

    const py::function& unboundPySlotFn() const {
        return unboundPySlotFn_;
    }

    virtual SignalTransmitter buildPyTransmitter() override {
        return buildPyTransmitterFromUnboundPySlotFn(unboundPySlotFn_);
    }

    virtual SignalTransmitter buildCppTransmitter() = 0;

private:
    // XXX change to span, and hold a static array in the cpp ref.
    std::vector<std::type_index> parameters_;
    // Unbound slot py-method.
    py::function unboundPySlotFn_;
};

// Should only be constructed from the return value of a slot method (defined
// with the VGC_SLOT macro).
//
class PyCppSlotRef : public PyAbstractCppSlotRef {
    using PyAbstractCppSlotRef::PyAbstractCppSlotRef;
};

template<typename Method, VGC_REQUIRES(isMethod<RemoveCVRef<Method>>)>
class PyCppSlotRefImpl : public PyCppSlotRef {
private:
    // clang-format off
    template<typename SlotRefT>
    static constexpr bool isCompatible_ = 
        core::detail::isSlotRef<SlotRefT>
        && std::is_same_v<typename SlotRefT::SlotMethod, Method>;
    // clang-format on

public:
    using Obj = typename MethodTraits<Method>::Class;
    using This = typename MethodTraits<Method>::This;
    using ArgsTuple = typename MethodTraits<Method>::ArgsTuple;
    using SignalArgRefsTuple = core::detail::MakeSignalArgRefsTuple<ArgsTuple>;

    PyCppSlotRefImpl(const Id id, Method method, This obj)
        : PyCppSlotRef(
            obj,
            id,
            py::cpp_function(method),
            static_cast<SignalArgRefsTuple*>(nullptr))
        , method_(method) {
    }

    template<typename SlotRefT, VGC_REQUIRES(isCompatible_<SlotRefT>)>
    PyCppSlotRefImpl(const SlotRefT& slotRef)
        : PyCppSlotRefImpl(SlotRefT::id(), slotRef.method(), slotRef.object()) {
    }

    virtual SignalTransmitter buildCppTransmitter() override {
        return SignalTransmitter::build<SignalArgRefsTuple>(
            method_, static_cast<Obj*>(object()));
    }

private:
    Method method_;
};

class PyCppSignalRef : public PyAbstractCppSlotRef {
    using PyAbstractCppSlotRef::PyAbstractCppSlotRef;

protected:
    using CppToPyTransmitterFactoryFn =
        std::function<SignalTransmitter(py::handle obj, py::function slot, Int arity)>;

public:
    const core::detail::SignalId& id() const {
        return PyAbstractCppSlotRef::id();
    }

    ConnectionHandle connect(PyAbstractSlotRef* slot) {
        if (arity() < slot->arity()) {
            throw py::value_error(
                "The slot signature cannot be longer than the signal signature.");
        }

        core::detail::ObjectSlotId slotId(slot->object(), slot->id());

        auto cppSlot = dynamic_cast<PyAbstractCppSlotRef*>(slot);
        if (cppSlot != nullptr) {
            const auto& signalParams = parameters();
            const auto& slotParams = cppSlot->parameters();
            if (!std::equal(slotParams.begin(), slotParams.end(), signalParams.begin())) {
                throw py::value_error("The slot and signal signatures do not match.");
            }

            return core::detail::SignalHub::connect(
                object(), id(), cppSlot->buildCppTransmitter(), slotId);
        }

        auto pySlot = dynamic_cast<PyPySlotRef*>(slot);
        if (pySlot != nullptr) {
            py::handle self = py::cast(slot->object());
            return core::detail::SignalHub::connect(
                object(),
                id(),
                cppToPyTransmitterFactory_(
                    self, pySlot->unboundPySlotFn(), pySlot->arity()),
                slotId);
        }

        auto pySignal = dynamic_cast<PyPySignalRef*>(slot);
        if (pySignal != nullptr) {
            return core::detail::SignalHub::connect(
                object(),
                id(),
                cppToPyTransmitterFactory_(
                    py::none(), pySignal->boundPyEmitFn(), pySignal->arity()),
                slotId);
        }

        throw py::value_error("Unsupported subclass of PyAbstractSlotRef.");
    }

    ConnectionHandle connectCallback(py::function callback) {
        auto inspect = py::module::import("inspect");
        Int arity = getFunctionArity(inspect, callback);
        return core::detail::SignalHub::connect(
            object(),
            id(),
            cppToPyTransmitterFactory_(py::none(), callback, arity),
            std::monostate{});
    }

    bool disconnect(ConnectionHandle h) {
        return core::detail::SignalHub::disconnect(object(), id(), h);
    }

    bool disconnectAll() {
        return core::detail::SignalHub::disconnect(object(), id());
    }

    bool disconnectSlot(PyAbstractSlotRef* slotRef) const {
        return core::detail::SignalHub::disconnect(
            object(),
            id(),
            vgc::core::detail::ObjectSlotId(slotRef->object(), slotRef->id()));
    }

protected:
    CppToPyTransmitterFactoryFn cppToPyTransmitterFactory_;
};

// Should only be constructed from the return value of a Signal method defined
// with the VGC_SIGNAL macro.
//
template<typename SignalRefT, VGC_REQUIRES(core::detail::isSignalRef<SignalRefT>)>
class PyCppSignalRefImpl : public PyCppSignalRef {
public:
    using ArgRefsTuple = typename SignalRefT::ArgRefsTuple;
    using Obj = typename SignalRefT::Obj;

protected:
    template<typename... ArgRefs>
    PyCppSignalRefImpl(
        const SignalRefT& signalRef,
        Obj* object,
        std::tuple<ArgRefs...>* sig)

        : PyCppSignalRef(
            object,
            signalRef.id(),
            buildUnboundPyEmitFn(signalRef.id(), sig),
            sig) {

        cppToPyTransmitterFactory_ = buildCppToPyTransmitterFactory<ArgRefs...>();
    }

public:
    PyCppSignalRefImpl(const SignalRefT& signalRef)
        : PyCppSignalRefImpl(
            signalRef,
            signalRef.object(),
            static_cast<ArgRefsTuple*>(nullptr)) {
    }

    virtual SignalTransmitter buildCppTransmitter() override {
        // only perfect match..
        return core::detail::buildRetransmitter<ArgRefsTuple, ArgRefsTuple>(
            object(), id());
    }

protected:
    // XXX use detail::unboundEmit
    template<typename... ArgRefs>
    static py::function
    buildUnboundPyEmitFn(core::detail::SignalId id, std::tuple<ArgRefs...>*) {
        return py::cpp_function([=](Object* obj, ArgRefs... args) {
            core::detail::SignalHub::emitFwd<ArgRefsTuple>(obj, id, args...);
        });
    }

    template<typename... SignalArgRefs>
    static CppToPyTransmitterFactoryFn buildCppToPyTransmitterFactory() {
        return [](py::handle obj, py::function slot, Int arity) -> SignalTransmitter {
            using SignalArgRefsTuple = std::tuple<SignalArgRefs...>;
            const auto& makeWrapperFuncs = getPySlotWrapperFnArray<SignalArgRefsTuple>(
                std::make_index_sequence<std::tuple_size_v<SignalArgRefsTuple> + 1>{});
            try {
                return makeWrapperFuncs[arity](obj, slot);
            }
            catch (std::out_of_range&) {
                throw core::LogicError(
                    "The slot signature cannot be longer than the signal signature.");
            }
        };
    }

protected:
    template<typename SignalArgRefsTuple, size_t... Is>
    static auto getPySlotWrapperFnArray(std::index_sequence<Is...>) {
        using MakeWrapperFn = decltype(makePySlotWrapper<std::tuple<>>)*;
        static constexpr std::array<MakeWrapperFn, sizeof...(Is)> makeWrapperFuncs = {
            makePySlotWrapper<SubTuple<0, Is, SignalArgRefsTuple>>...};
        return makeWrapperFuncs;
    }

    template<typename SignalArgRefsTuple>
    static typename SignalTransmitter::SlotWrapper
    makePySlotWrapper(py::handle obj, py::function slot) {
        return makePySlotWrapper_<SignalArgRefsTuple>(
            obj, slot, std::make_index_sequence<std::tuple_size_v<SignalArgRefsTuple>>{});
    }

    template<typename SignalArgRefsTuple, size_t... Is>
    static typename SignalTransmitter::SlotWrapper
    makePySlotWrapper_(py::handle obj, py::function slot, std::index_sequence<Is...>) {
        if (obj.is_none()) {
            return typename SignalTransmitter::SlotWrapper(
                [=]([[maybe_unused]] const TransmitArgs& args) {
                    slot(args.get<std::tuple_element_t<Is, SignalArgRefsTuple>>(Is)...);
                });
        }
        else {
            return typename SignalTransmitter::SlotWrapper(
                [=]([[maybe_unused]] const TransmitArgs& args) {
                    slot(
                        obj,
                        args.get<std::tuple_element_t<Is, SignalArgRefsTuple>>(Is)...);
                });
        }
    }
};

} // namespace vgc::core::wraps

#endif // VGC_CORE_WRAPS_SIGNAL_H
