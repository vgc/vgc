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

#ifndef VGC_CORE_SIGNAL_H
#define VGC_CORE_SIGNAL_H

#include <functional>
#include <memory>
#include <tuple>
#include <type_traits>
#include <typeindex>
#include <utility>
#include <variant>
#include <vector>

// XXX temporary
#include <iostream>

#include <vgc/core/api.h>
#include <vgc/core/array.h>
#include <vgc/core/pp.h>
#include <vgc/core/stringid.h>
#include <vgc/core/templateutil.h>

// XXX rewrite
/// \class Signal
/// \brief Implements a signal-slot notification mechanism.
///
/// This is VGC's implementation of a Qt-style signal-slot notification system,
/// similar to the excellent Boost::signals2 library. It allows a "sender" to
/// notify a "listener" that something happened to the sender.
///
/// Typically, this is used in model-view paradigms, where views must be
/// notified when models change in order to redraw them.
///
/// For now, the VGC signal-slot mechanism is neither thread-safe nor allows
/// disconnections, unlike Boost::signals2. The reason VGC is not using
/// Boost::signals2 is two-folds: first, we desired to avoid the dependency to
/// Boost, and secondly, we desire to fine-tune this mechanism to the VGC
/// object model, and make it play nice with Python.
///
/// Example 1:
///
/// \code
/// void printInt(int i) { std::cout << i << std::endl; }
/// int main() {
///     vgc::core::Signal<int> s;
///     s.connect(&printInt);
///     s(42); // print 42
/// }
/// \endcode
///
/// Example 2:
///
/// \code
/// class Model {
/// int x_;
/// public:
///     vgc::core::Signal<> changed;
///     int x() const { return x_; }
///     void setX(int x) { x_ = x; changed(); }
/// };
///
/// class View {
/// const Model* m_;
/// public:
///     View(const Model* m) : m_(m) {}
///     void update() { std::cout << m_->x() << std::endl; }
/// };
///
/// int main() {
///     Model model;
///     View view(&model);
///     model.changed.connect(std::bind(&View::update, &view));
///     model.setX(42); // print 42
/// }
/// \endcode
///

namespace vgc::core {

class Object;

namespace internal {

template<typename T, typename Enable = void>
struct hasObjectTypedefs_ : std::false_type {};
template<typename T>
struct hasObjectTypedefs_<T, std::void_t<typename T::ThisClass, typename T::SuperClass>> :
    std::is_same<T, typename T::ThisClass> {};

// If T is an Object, it also checks that VGC_OBJECT has been used in T.
// 
template<typename T, typename Enable = void>
struct isObject_ : std::false_type {};
template<>
struct isObject_<Object, void> : std::true_type {};
template<typename T>
struct isObject_<T, std::enable_if_t<std::is_base_of_v<Object, T>, void>> : std::true_type {
    static_assert(hasObjectTypedefs_<T>::value,
        "Missing VGC_OBJECT(..) in T.");
};

} // namespace internal

template <typename T>
inline constexpr bool isObject = internal::isObject_<T>::value;

// Checks that Type inherits from Object.
// Also checks that VGC_OBJECT(..) has been used in Type.
#define CHECK_TYPE_IS_VGC_OBJECT(Type) \
    static_assert(::vgc::core::internal::isObject<Type>, #Type " should inherit from vgc::core::Object.");

namespace internal {

// Signals and Slots getters can be identified by signature.
//template<typename T>
//struct isSignalOrSlotGetter_ : std::false_type {};
//template<typename R, typename C, typename... Args>
//struct isSignalOrSlotGetter_<R (C::*)(Args...) const> {
//    static constexpr bool value = IsSignalTraits<R>;
//    static_assert(!value || isObject<C>, "Signals must reside in an Objects.");
//};
//template <typename T>
//inline constexpr bool isSignal = isSignal_<T>::value;


using SignalId = std::type_index;
using SlotId = std::type_index;
using BoundSlotId = std::pair<Object*, SlotId>;
using FreeFuncId = void*;
using SignalHandlerId = std::variant<std::monostate, BoundSlotId, FreeFuncId>;
using ConnectionHandle = UInt64;


//template<typename PointerToMemberFunction>
//struct isSignalFunction_ : std::false_type {};
//template<typename C, typename... Args>
//struct isSignalFunction_<EmitCheck (C::*)(Args...) const> : std::true_type {
//    static_assert(isObject<C>, "Signals can only be declared in Objects.");
//};
//template <typename T>
//inline constexpr bool isSignalFunction = isSignalFunction_<T>::value;

template<typename C, bool IsCallOperator, typename R, typename... Args>
struct FunctionTraitsDef_ {
    using ReturnType = R;
    using Obj = C;
    using ArgsTuple = std::tuple<Args...>;
    static constexpr size_t arity = sizeof...(Args);
    static constexpr bool isCallOperator = IsCallOperator;
};

template<typename MemberFunctionPointer>
struct MemberFunctionTraits_;
template<typename R, typename C, typename... Args>
struct MemberFunctionTraits_<R (C::*)(Args...)> : FunctionTraitsDef_<C, false, R, Args...> {};
template <typename R, typename C, typename... Args>
struct MemberFunctionTraits_<R (C::*)(Args...) const> : FunctionTraitsDef_<C, false, R, Args...> {};
template<typename T>
using MemberFunctionTraits = MemberFunctionTraits_<std::remove_reference_t<T>>;


// XXX could benefit from a reusable FunctionTraits with a category attribute.
//
// callable's operator
template<typename T>
struct SignalCallableHandlerTraits_;
template<typename R, typename C, typename... Args>
struct SignalCallableHandlerTraits_<R (C::*)(Args...)> : FunctionTraitsDef_<C, true, R, Args...> {};
template<typename R, typename C, typename... Args>
struct SignalCallableHandlerTraits_<R (C::*)(Args...) const> : FunctionTraitsDef_<C, true, R, Args...> {};
// primary with fallback
template<typename T, typename Enable = void>
struct SignalFreeHandlerTraits_;
template<typename T>
struct SignalFreeHandlerTraits_<T, std::void_t<decltype(&T::operator())>> : SignalCallableHandlerTraits_<decltype(&T::operator())> {};
// free function
template<typename R, typename... Args>
struct SignalFreeHandlerTraits_<R (*)(Args...), void> : FunctionTraitsDef_<void, false, R, Args...> {};
//
template<typename T>
using SignalFreeHandlerTraits = SignalFreeHandlerTraits_<std::remove_reference_t<T>>;

template<typename T, typename Enable = void>
struct isSignalFreeHandler_ : std::false_type {};
template<typename T>
struct isSignalFreeHandler_<T, typename SignalFreeHandlerTraits<T>::ReturnType> : std::true_type {};
template <typename T>
inline constexpr bool isSignalFreeHandler = isSignalFreeHandler_<T>::value;


template<class F, class ArgsTuple, std::size_t... I>
void applyPartialImpl(F&& f, ArgsTuple&& t, std::index_sequence<I...>) {
    std::invoke(
        std::forward<F>(f),
        std::get<I>(std::forward<ArgsTuple>(t))...);
}

template<size_t N, class F, class ArgsTuple>
void applyPartial(F&& f, ArgsTuple&& t) {
    applyPartialImpl(
        std::forward<F>(f),
        std::forward<ArgsTuple>(t),
        std::make_index_sequence<N>{});
}

VGC_CORE_API
ConnectionHandle genConnectionHandle();


// This is a polymorphic adapter class for slots and free functions.
// It is used to store the handlers of all signals of a given object
// in a single container.
// Moreover it provides a common handler signature per signal.
// Handlers with less arguments than the signal they are connected
// to are supported. The tail arguments are simply omitted.
// For instance, a handler adapting slot(double a) to
// signal(int a, double b) would be equivalent to this:
// handler(int&& a, double&& b) { slot(std::forward<int>(a)); }
//
class AbstractSignalTransmitter {
protected:
    AbstractSignalTransmitter() = default;

public:
    virtual ~AbstractSignalTransmitter() = default;
};


template<typename... SignalArgs>
class SignalTransmitter : public AbstractSignalTransmitter {
public:
    using SignalArgsTuple = std::tuple<SignalArgs...>;
    using CallArgsTuple = std::tuple<SignalArgs&&...>;
    using CallSig = void(SignalArgs&&...);
    using FnType = std::function<CallSig>;

    // left public for python bindings
    SignalTransmitter(const FnType& fn) :
        fn_(fn) {}

    // left public for python bindings
    SignalTransmitter(FnType&& fn) :
        fn_(std::move(fn)) {}

    inline void operator()(SignalArgs&&... args) const {
        fn_(std::forward<SignalArgs>(args)...);
    }

    // pointer-to-member-function
    template<typename R, typename Obj, typename... SlotArgs>
    [[nodiscard]] static inline
    SignalTransmitter* create(Obj* o, R (Obj::* mfn)(SlotArgs...)) {
        static_assert(sizeof...(SignalArgs) >= sizeof...(SlotArgs),
            "The slot signature cannot be longer than the signal signature.");
        static_assert(std::is_same_v<std::tuple<SlotArgs...>, SubPackAsTuple<0, sizeof...(SlotArgs), SignalArgs...>>,
            "The slot signature does not match the signal signature.");
        return new SignalTransmitter(
            [=](SignalArgs&&... args) {
                applyPartial<1 + sizeof...(SlotArgs)>(mfn, std::forward_as_tuple(o, std::forward<SignalArgs>(args)...));
            });
    }

    // free functions and callables
    template<typename FreeHandler>
    [[nodiscard]] static inline
    SignalTransmitter* create(FreeHandler&& f) {
        using HTraits = SignalFreeHandlerTraits<FreeHandler>;
        // optimization: when the target has already the desired signature
        if constexpr (std::is_same_v<CallArgsTuple, typename HTraits::ArgsTuple>) {
            return new SignalTransmitter(std::forward<FreeHandler>(f));
        }
        return new SignalTransmitter(
            [=](SignalArgs&&... args) {
                applyPartial<HTraits::arity>(f, std::forward_as_tuple(std::forward<SignalArgs>(args)...));
            });
    }

private:
    FnType fn_;
};


// Its member functions are static because we have to operate from the context of its owner Object.
class SignalHub {
private:
    struct Connection_ {
        template<typename To>
        Connection_(std::unique_ptr<AbstractSignalTransmitter>&& f, ConnectionHandle h, SignalId from, To&& to) :
            f(std::move(f)), h(h), from(from), to(std::forward<To>(to)) {}

        std::unique_ptr<AbstractSignalTransmitter> f;
        ConnectionHandle h;
        SignalId from;
        SignalHandlerId to;
    };

    struct ListenedObjectInfo {
        Object* object = nullptr;
        Int numInboundConnections = 0;
    };

    SignalHub(const SignalHub&) = delete;
    SignalHub& operator=(const SignalHub&) = delete;

public:
    SignalHub() = default;

    // Defined in object.h
    static constexpr SignalHub& access(Object* o);

    // Must be called by Object's destructor.
    static void onDestroy(Object* sender) {
        auto& hub = access(sender);

        // XXX TODO !!!
    }

    static ConnectionHandle connect(Object* sender, SignalId signalId, std::unique_ptr<AbstractSignalTransmitter>&& transmitter, const SignalHandlerId& handlerId) {
        auto& hub = access(sender);
        auto h = genConnectionHandle();
        hub.connections_.emplaceLast(std::move(transmitter), h, signalId, handlerId);
        return h;
    }

    // XXX add to public interface of Object
    static bool disconnect(Object* sender, ConnectionHandle h) {
        return removeConnectionIf_(sender, [=](const Connection_& c) {
            return c.h == h; 
        });
    }

    static bool disconnect(Object* sender, SignalId signalId) {
        return removeConnectionIf_(sender, [=](const Connection_& c) {
            return c.from == signalId;
        });
    }

    static bool disconnect(Object* sender, SignalId signalId, ConnectionHandle h) {
        return removeConnectionIf_(sender, [=](const Connection_& c) {
            return c.from == signalId && c.h == h;
        });
    }

    static bool disconnect(Object* sender, SignalId signalId, const SignalHandlerId& signalHandlerId) {
        return removeConnectionIf_(sender, [=](const Connection_& c) {
            return c.from == signalId && c.to == signalHandlerId;
        });
    }

    // DANGEROUS.
    //
    template<typename SignalTransmitterType, typename... Args>
    std::enable_if_t<std::is_base_of_v<AbstractSignalTransmitter, SignalTransmitterType>>
    static emit_(Object* sender, SignalId id, Args&&... args) {
        auto& hub = access(sender);
        using TransmitterType = SignalTransmitter<Args...>;
        for (auto& c : hub.connections_) {
            if (c.from == id) {
                // XXX replace with static_cast after initial test rounds
                const auto* f = dynamic_cast<SignalTransmitterType*>(c.f.get());
                if (!f) {
                    throw std::logic_error("wrong SignalTransmitterType");
                }
                (*f)(std::forward<Args>(args)...);
            }
        }
    }

protected:

    static void disconnect_(Object* sender, Object* receiver) {
        auto& hub = access(sender);
        auto it = std::remove_if(hub.connections_.begin(), hub.connections_.end(), [=](const Connection_& c) {
                return std::holds_alternative<BoundSlotId>(c.to) && std::get<BoundSlotId>(c.to).first == receiver; 
            });
        hub.connections_.erase(it, hub.connections_.end());
    }

    ListenedObjectInfo& getListenedObjectInfoRef(Object* object) {
        for (ListenedObjectInfo& x : listenedObjectInfos_) {
            if (x.object == object) {
                return x;
            }
        }
        throw LogicError("Info should be present.");
    }

    ListenedObjectInfo& findOrCreateListenedObjectInfo(Object* object) {
        for (ListenedObjectInfo& x : listenedObjectInfos_) {
            if (x.object == object) {
                return x;
            }
            if (x.numInboundConnections == 0) {
                x.object = object;
                return x;
            }
        }
        auto info = listenedObjectInfos_.emplaceLast();
        info.object = object;
        return info;
    }

private:
    Array<Connection_> connections_;
    // Used to auto-disconnect on destroy.
    Array<ListenedObjectInfo> listenedObjectInfos_;

    // Returns true if any connection is removed.
    template<typename Pred>
    static bool removeConnectionIf_(Object* sender, Pred pred) {
        auto& hub = access(sender);
        auto end = hub.connections_.end();
        auto it = std::find_if(hub.connections_.begin(), end, pred);
        auto insertIt = it;
        if (it != end) {
            while (++it != end) {
                if (!pred(*it)) {
                    *insertIt = std::move(*it);
                    ++insertIt;
                }
                else {
                    const Connection_& c = *it;
                    if (std::holds_alternative<BoundSlotId>(c.to)) {
                        // Decrement numInboundConnections in the receiver's info about sender.
                        const auto& bsid = std::get<BoundSlotId>(c.to);
                        auto& info = access(bsid.first).getListenedObjectInfoRef(sender);
                        info.numInboundConnections--;
                    }
                }
            }
        }
        if (insertIt != end) {
            hub.connections_.erase(insertIt, end);
            return true;
        }
        return false;
    }    
};


template<typename UniqueIdentifierTag, typename SlotFunctionT>
class SlotRef {
public:
    using SlotFunction = SlotFunctionT;
    using FnTraits = MemberFunctionTraits<SlotFunction>;
    using Obj = typename FnTraits::Obj;
    using ArgsTuple = typename FnTraits::ArgsTuple;

    static_assert(isObject<Obj>, "Slots must be declared only in Objects");

    SlotRef(const Obj* object, SlotFunction mfn) :
        object_(const_cast<Obj*>(object)), mfn_(mfn) {}

    SlotRef(const SlotRef&) = delete;
    SlotRef& operator=(const SlotRef&) = delete;

    static constexpr SlotId id() {
        return SlotId(typeid(UniqueIdentifierTag));
    }

    constexpr Obj* object() const {
        return object_;
    }

    constexpr const SlotFunction& mfn() const {
        return mfn_;
    }

private:
    Obj* const object_;
    SlotFunction const mfn_;
};

// It does not define emit(..) and is intended to be locally subclassed in
// the signal getter.
//
template<typename UniqueIdentifierTag, typename ObjT, typename... Args>
class SignalRefBase {
public:
    using Obj = ObjT;
    using ArgsTuple = std::tuple<Args...>;

    static_assert(isObject<Obj>, "Signals must be declared only in Objects");

    SignalRefBase(const Obj* object) :
        object_(const_cast<Obj*>(object)) {}

    SignalRefBase(const SignalRefBase&) = delete;
    SignalRefBase& operator=(const SignalRefBase&) = delete;

    static constexpr SignalId id() {
        return SignalId(typeid(UniqueIdentifierTag));
    }

    constexpr Obj* object() const {
        return object_;
    }

    template<typename UIdTag, typename SlotFunctionT>
    ConnectionHandle connect(SlotRef<UIdTag, SlotFunctionT>&& slot) const {
        // XXX make owner listen on receiver destroy to automatically disconnect signals
        std::unique_ptr<AbstractSignalTransmitter> transmitter(
            SignalTransmitter<Args...>::create(slot.object(), slot.mfn()));
        return SignalHub::connect(object_, id(), std::move(transmitter), BoundSlotId(slot.object(), slot.id()));
    }

    template<typename R, typename... FnArgs>
    ConnectionHandle connect(R (*callback)(FnArgs...)) const {
        std::unique_ptr<AbstractSignalTransmitter> transmitter(
            SignalTransmitter<Args...>::create(callback));
        return SignalHub::connect(object_, id(), std::move(transmitter), FreeFuncId(callback));
    }

    template<typename Callable, std::enable_if_t<SignalFreeHandlerTraits<Callable>::isCallOperator, int> = 0>
    ConnectionHandle connect(Callable&& callable) const {
        std::unique_ptr<AbstractSignalTransmitter> transmitter(
            SignalTransmitter<Args...>::create(callable));
        return SignalHub::connect(object_, id(), std::move(transmitter), std::monostate{});
    }

    bool disconnect() const {
        return SignalHub::disconnect(object_, id());
    }

    bool disconnect(ConnectionHandle h) const {
        return SignalHub::disconnect(object_, id(), h);
    }

    template<typename UIdTag, typename SlotFunctionT>
    bool disconnect(SlotRef<UIdTag, SlotFunctionT>&& slot) const {
        return SignalHub::disconnect(object_, id(), BoundSlotId(slot.object(), slot.id()));
    }

    template<typename R, typename... FnArgs>
    bool disconnect(R (*callback)(FnArgs...)) const {
        return SignalHub::disconnect(object_, id(), FreeFuncId(callback));
    }

private:
    Obj* const object_;
};

// To enforce the use of VGC_EMIT when emitting a signal.
struct VGC_NODISCARD("Please use VGC_EMIT.") EmitCheck {
    EmitCheck(const EmitCheck&) = delete;
    EmitCheck& operator=(const EmitCheck&) = delete;
};

} // namespace internal

} // namespace vgc::core


#define VGC_SIG_PTYPE2_(t, n) t
#define VGC_SIG_PNAME2_(t, n) n
#define VGC_SIG_PBOTH2_(t, n) t n
#define VGC_SIG_PTYPE_(x) VGC_SIG_PTYPE2_ x
#define VGC_SIG_PNAME_(x) VGC_SIG_PNAME2_ x
#define VGC_SIG_PBOTH_(x) VGC_SIG_PBOTH2_ x

// ... must end with VaEnd
#define VGC_PARAMS_TYPE_(...) VGC_PP_TRIM_VAEND(VGC_PP_TRANSFORM(VGC_SIG_PTYPE_, __VA_ARGS__))
// ... must end with VaEnd
#define VGC_PARAMS_NAME_(...) VGC_PP_TRIM_VAEND(VGC_PP_TRANSFORM(VGC_SIG_PNAME_, __VA_ARGS__))
// ... must end with VaEnd
#define VGC_PARAMS_(...) VGC_PP_TRIM_VAEND(VGC_PP_TRANSFORM(VGC_SIG_PBOTH_, __VA_ARGS__))

#define VGC_SIG_FWD2_(t, n) std::forward<t>(n)
#define VGC_SIG_FWD_(x) VGC_SIG_FWD2_ x


// a->changed().emit(...)
// a->changed().connect(onChangedSlot(slot))
// 
// VGC_SIGNAL(changed, (args..))
// VGC_SLOT(onChangedSlot)


/// Macro to define VGC Object Signals.
///
/// Example:
/// 
/// \code
/// struct A : vgc::core::Object {
///     VGC_SIGNAL(signal1);
///     VGC_SIGNAL(signal2, (type0, arg0), (type1, arg1));
/// };
/// \endcode
///
#define VGC_SIGNAL(...) VGC_PP_EXPAND(VGC_SIGNAL_(__VA_ARGS__, VaEnd))
#define VGC_SIGNAL_(name_, ...)                                                                         \
    auto name_() {                                                                                      \
        struct Tag {};                                                                                  \
        using MyClass = std::remove_pointer_t<decltype(this)>;                                          \
        using SignalRefBase = ::vgc::core::internal::SignalRefBase<                                     \
            Tag, VGC_PARAMS_TYPE_((MyClass,), __VA_ARGS__)>;                                            \
        class SignalRef : public SignalRefBase {                                                        \
        public:                                                                                         \
            using SignalRefBase::SignalRefBase;                                                         \
            ::vgc::core::internal::EmitCheck                                                            \
            emit(VGC_PARAMS_(__VA_ARGS__)) const {                                                      \
                using TransmitterType = ::vgc::core::internal::SignalTransmitter<                       \
                    VGC_PARAMS_TYPE_(__VA_ARGS__)>;                                                     \
                SignalHub::emit_<TransmitterType>(                                                      \
                    VGC_PP_TRIM_VAEND(                                                                  \
                        object(), SignalRefBase::id(), VGC_PP_TRANSFORM(VGC_SIG_FWD_, __VA_ARGS__)));   \
                return {};                                                                              \
            }                                                                                           \
        };                                                                                              \
        return SignalRef(this);                                                                         \
    }

#define VGC_EMIT std::ignore =


#define VGC_SLOT(name_)                                                                                 \
    auto name_##Slot() {                                                                                \
        struct Tag {};                                                                                  \
        using MyClass = std::remove_pointer_t<decltype(this)>;                                          \
        using SlotRef = ::vgc::core::internal::SlotRef<                                                 \
            Tag, decltype(&MyClass::name_)>;                                                            \
        return SlotRef(this, &MyClass::name_);                                                          \
    }

#define VGC_SLOT_ALIAS(name_, funcName_)                                                                \
    auto name_() {                                                                                      \
        struct Tag {};                                                                                  \
        using MyClass = std::remove_pointer_t<decltype(this)>;                                          \
        using SlotRef = ::vgc::core::internal::SlotRef<                                                 \
            Tag, decltype(&MyClass::funcName_)>;                                                        \
        return SlotRef(this, &MyClass::funcName_);                                                      \
    }


namespace vgc::core {

using ConnectionHandle = internal::ConnectionHandle;

} // namespace vgc::core::internal











// Simple impl

namespace vgc::core::internal {

template<typename Sig>
class SignalImpl;

template<typename... Args>
class SignalImpl<void(Args...)> {
private:
    using ArgsTuple = std::tuple<Args...>;
    using CFnType = void(Args...);
    using FnType = std::function<CFnType>;

public:
    /// Triggers the signal, that is, calls all connected functions.
    ///
    // XXX should this be non-const to prevent outsiders from emitting the signal?
    void operator()(Args... args) const {
        for(const auto& l : listeners_) {
            l(args...);
        }
    }

    /// Deprecated
    // XXX Deprecated
    void connect(FnType fn) const {
        addListener_(fn);
    }

    void emit(Args... args) const {
        for(const auto& l : listeners_) {
            l(args...);
        }
    }

private:
    // This class defines connect helpers and thus needs access to Signal internals.
    friend class SignalOps;

    struct Listener_ {
        FnType fn;
        ConnectionHandle h;
        std::variant<
            std::monostate,
            SlotId,
            FreeFuncId
        > id;

        void operator()(Args... args) const {
            fn(args...);
        }
    };

    mutable Array<Listener_> listeners_;

    template<typename Pred>
    void removeListenerIf_(Pred pred) const {
        auto it = std::remove_if(listeners_.begin(), listeners_.end(), pred);
        listeners_.erase(it, listeners_.end());
    }

    void removeListener_(ConnectionHandle h) const {
        removeListenerIf_([=](const Listener_& l) {
            return l.h == h; 
            });
    }

    void removeListener_(const Object* o, const StringId& slotName) const {
        removeListenerIf_([=](const Listener_& l) {
            return std::holds_alternative<SlotId>(l.id) && std::get<SlotId>(l.id) == SlotId(o, slotName);
            });
    }

    void removeListener_(void* freeFunc) const {
        removeListenerIf_([=](const Listener_& l) {
            return std::holds_alternative<FreeFuncId>(l.id) && l.id == freeFunc;
            });
    }

    ConnectionHandle addListener_(FnType fn) const {
        auto h = genConnectionHandle();
        listeners_.append({fn, h, std::monostate{}});
        return h;
    }

    ConnectionHandle addListener_(FnType fn, Object* o, StringId slotName) const {
        auto h = genConnectionHandle();
        listeners_.append({fn, h, SlotId{o, slotName}});
        return h;
    }

    ConnectionHandle addListener_(FnType fn, void* freeFunc) const {
        auto h = genConnectionHandle();
        listeners_.append({fn, h, FreeFuncId{freeFunc}});
        return h;
    }

    // pointer-to-member-function
    template<typename Object, typename... SlotArgs>
    static inline
        FnType adaptHandler_(Object* o, void (Object::* mfn)(SlotArgs...)) {
        return [=](Args... args) {
            auto&& argsTuple = std::forward_as_tuple(o, std::forward<Args>(args)...);
            applyPartial<1 + sizeof...(SlotArgs)>(mfn, argsTuple);
        };
    }

    // free functions and callables
    template<typename Handler>
    static inline
        FnType adaptHandler_(Handler&& f) {
        using HTraits = SignalFreeHandlerTraits<Handler>;
        static_assert(std::is_same_v<typename HTraits::ReturnType, void>, "Signal handlers must return void.");
        return [=](Args... args) {
            auto&& argsTuple = std::forward_as_tuple(std::forward<Args>(args)...);
            applyPartial<HTraits::arity>(f, argsTuple);
        };
    }
};


template<typename T>
struct IsSignalImpl : std::false_type {};

template<typename Sig>
struct IsSignalImpl<SignalImpl<Sig>> : std::true_type {};

} // namespace vgc::core::internal

namespace vgc::core {

// XXX temporary until all pieces of code use VGC_SIGNAL
template<typename... Args>
using Signal = internal::SignalImpl<void(Args...)>;

} // namespace vgc::core


#endif // VGC_CORE_SIGNAL_H
