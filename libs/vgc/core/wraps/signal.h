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

#include <vgc/core/wraps/common.h>
#include <vgc/core/signal.h>
#include <vgc/core/object.h>

struct Args {
    template<size_t N>
    constexpr Args(const char (&str)[N]) {}
};

namespace vgc::core::wraps {

class AbstractCppSlotRef {
protected:
    AbstractCppSlotRef(const core::Object* obj, std::initializer_list<std::type_index> parameters) :
        obj_(const_cast<core::Object*>(obj)), parameters_(parameters) {}

public:
    const auto& parameters() const {
        return parameters_;
    }

    core::Object* object() const {
        return obj_;
    }

    virtual py::cpp_function getPyWrappedBoundSlot() const = 0;

protected:
    std::vector<std::type_index> parameters_;
    core::Object* obj_;
};

template<typename... SlotArgs>
class CppSlotRef : public AbstractCppSlotRef {
public:
    // Note: forwarded arguments as for transmitters
    using BoundSlotSig = void(SlotArgs&&...);
    using BoundSlot = std::function<BoundSlotSig>;

    using AbstractCppSlotRef::AbstractCppSlotRef;

    BoundSlot getBoundSlot() const {
        return bind_(object());
    }

    virtual py::cpp_function getPyWrappedBoundSlot() const override {
        return py::cpp_function(getBoundSlot());
    }

private:
    virtual BoundSlot bind_(Object* obj) const = 0;
};

template<typename Obj, typename... SlotArgs>
class CppSlotRefImpl : public CppSlotRef<SlotArgs...> {
    using SuperClass = CppSlotRef<SlotArgs...>;
    using MFnT = void (Obj::*)(SlotArgs...);
    using BoundSlot = typename SuperClass::BoundSlot;

public:
    CppSlotRefImpl(const Obj* obj, MFnT mfn) :
        SuperClass(obj, {std::type_index(typeid(SlotArgs))...}),
        mfn_(mfn) {}

private:
    MFnT mfn_;

    virtual BoundSlot bind_(core::Object* obj) const override {
        // XXX can add alive check here or in the lambda..
        using MFnT = void (Obj::*)(SlotArgs...);
        return [=](SlotArgs&&... args) {
            (static_cast<Obj*>(obj)->*mfn_)(std::forward<SlotArgs>(args)...);
        };
    }
};


template<typename ArgsTuple>
struct CppSlotRefTypeFromArgsTuple_;
template<typename... SlotArgs>
struct CppSlotRefTypeFromArgsTuple_<std::tuple<SlotArgs...>> {
    using type = CppSlotRef<SlotArgs...>;
};
template<typename ArgsTuple>
using CppSlotRefTypeFromArgsTuple = typename CppSlotRefTypeFromArgsTuple_<ArgsTuple>::type;


class AbstractCppSignalRef {
protected:
    AbstractCppSignalRef(const Object* obj, const core::internal::SignalId& id, std::initializer_list<std::type_index> parameters) :
        obj_(const_cast<core::Object*>(obj)), parameters_(parameters) {}

public:
    const auto& parameters() const {
        return parameters_;
    }

    core::Object* object() const {
        return obj_;
    }

    virtual [[nodiscard]] core::internal::AbstractSignalTransmitter*
    createTransmitter(AbstractCppSlotRef* slotRef) const = 0;

protected:
    std::vector<std::type_index> parameters_;
    core::Object* obj_;
    core::internal::SignalId id_;
};

void checkCompatibility(const AbstractCppSignalRef* signalRef, const AbstractCppSlotRef* slotRef) {
    const auto& signalParams = signalRef->parameters();
    const auto& slotParams = slotRef->parameters();
    if (slotParams.size() > signalParams.size()) {
        // XXX subclass LogicError
        throw LogicError("Slot signature is too long for this Signal.");
    }
    if (!std::equal(slotParams.begin(), slotParams.end(), signalParams.begin())) {
        // XXX subclass LogicError
        throw LogicError("Slot/Signal signature mismatch.");
    }
}

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
struct CppSignalRef : public AbstractCppSignalRef {
    using TransmitterType = vgc::core::internal::SignalTransmitter<SignalArgs...>;

    CppSignalRef(const Object* obj, const core::internal::SignalId& id) :
        AbstractCppSignalRef(obj, id, {std::type_index(typeid(SignalArgs))...}) {}

    virtual [[nodiscard]] core::internal::AbstractSignalTransmitter*
    createTransmitter(AbstractCppSlotRef* slotRef) const override {
        checkCompatibility(this, slotRef);
        const auto& slotParams = slotRef->parameters();
        switch (slotParams.size()) {
        CREATE_TRANSMITTER_SWITCH_CASE(0);
        CREATE_TRANSMITTER_SWITCH_CASE(1);
        CREATE_TRANSMITTER_SWITCH_CASE(2);
        CREATE_TRANSMITTER_SWITCH_CASE(3);
        CREATE_TRANSMITTER_SWITCH_CASE(4);
        CREATE_TRANSMITTER_SWITCH_CASE(5);
        CREATE_TRANSMITTER_SWITCH_CASE(6);
        CREATE_TRANSMITTER_SWITCH_CASE(7);
        CREATE_TRANSMITTER_SWITCH_CASE(8);
        CREATE_TRANSMITTER_SWITCH_CASE(9);
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
