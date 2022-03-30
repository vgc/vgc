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
using BoundSlotId = std::pair<const Object*, SlotId>;
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

    SignalHub(const SignalHub&) = delete;
    SignalHub& operator=(const SignalHub&) = delete;

public:
    SignalHub() = default;

    ConnectionHandle connect(SignalId signalId, std::unique_ptr<AbstractSignalTransmitter>&& transmitter, const SignalHandlerId& handlerId) {
        auto h = genConnectionHandle();
        connections_.emplaceLast(std::move(transmitter), h, signalId, handlerId);
        return h;
    }

    // XXX add to public interface of Object
    void disconnect(ConnectionHandle h) {
        removeConnectionIf_([=](const Connection_& c) {
            return c.h == h; 
        });
    }

    void disconnect(SignalId signalId) {
        removeConnectionIf_([=](const Connection_& c) {
            return c.from == signalId;
        });
    }

    void disconnect(SignalId signalId, const Object* o, const SignalHandlerId& signalHandlerId) {
        removeConnectionIf_([=](const Connection_& c) {
            return c.from == signalId && c.to == signalHandlerId;
        });
    }

    // DANGEROUS.
    //
    template<typename SignalTransmitterType, typename... Args>
    std::enable_if_t<std::is_base_of_v<AbstractSignalTransmitter, SignalTransmitterType>>
    emit_(SignalId id, Args&&... args) const {
        using TransmitterType = SignalTransmitter<Args...>;
        for (auto& c : connections_) {
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

private:
    Array<Connection_> connections_;

    template<typename Pred>
    void removeConnectionIf_(Pred pred) {
        auto it = std::remove_if(connections_.begin(), connections_.end(), pred);
        connections_.erase(it, connections_.end());
    }
};

// Friend of vgc::core::Object.
struct SignalHubAccess {
    template<typename = void>
    static constexpr SignalHub* get(Object* o) { return &o->signalHub_; }
};

// todo: subclass SignalRef in the getter (VGC_SIGNAL) to define emit
// todo: add SlotTag slot in Object

template<typename UniqueIdentifierTag, typename SlotFunctionT>
class SlotRef {
public:
    using UIdTag = UniqueIdentifierTag;
    using SlotFunction = SlotFunctionT;
    using FnTraits = MemberFunctionTraits<SlotFunction>;
    using Obj = typename FnTraits::Obj;
    using ArgsTuple = typename FnTraits::ArgsTuple;

    static_assert(isObject<Obj>, "Slots must be declared only in Objects");

    SlotRef(const Obj* object, SlotFunction mfn) :
        object_(const_cast<Obj*>(object)), mfn_(mfn) {}

    SlotRef(const SlotRef&) = delete;
    SlotRef& operator=(const SlotRef&) = delete;

    static constexpr SlotId id() const {
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

template<typename UniqueIdentifierTag, typename ObjT, typename... Args>
class SignalRef {
public:
    using UIdTag = UniqueIdentifierTag;
    using Obj = ObjT;
    using ArgsTuple = std::tuple<Args...>;

    static_assert(isObject<Obj>, "Signals must be declared only in Objects");

    SignalRef(const Obj* object) :
        object_(const_cast<Obj*>(object)) {}

    SignalRef(const SignalRef&) = delete;
    SignalRef& operator=(const SignalRef&) = delete;

    static constexpr SignalId id() const {
        return SignalId(typeid(UniqueIdentifierTag));
    }

    constexpr Obj* object() const {
        return object_;
    }

    template<typename UIdTag, typename SlotFunctionT>
    static ConnectionHandle connect(SlotRef<UIdTag, SlotFunctionT>&& slot) const {
        // XXX make owner listen on receiver destroy to automatically disconnect signals
        std::unique_ptr<AbstractSignalTransmitter> transmitter(
            SignalTransmitter<Args...>::create(slot.object(), slot.mfn()));
        return SignalHubAccess::get(object_).connect(id(), std::move(transmitter), BoundSlotId(slot.object(), slot.id()));
    }

    template<typename R, typename... FnArgs>
    static ConnectionHandle connect(R (*callback)(Args...)) const {
        std::unique_ptr<AbstractSignalTransmitter> transmitter(
            SignalTransmitter<Args...>::create(callback));
        return SignalHubAccess::get(object_).connect(id(), std::move(transmitter), FreeFuncId(callback));
    }

    template<typename Callable, std::enable_if_t<SignalFreeHandlerTraits<Callable>::isCallOperator, int> = 0>
    static ConnectionHandle connect(Callable&& callable) const {
        std::unique_ptr<AbstractSignalTransmitter> transmitter(
            SignalTransmitter<Args...>::create(callable));
        return SignalHubAccess::get(object_).connect(id(), std::move(transmitter), std::monostate{});
    }

private:
    Obj* const object_;
};

// todo: add vector<pair<obj*, connectCount>> in Object to track listened objects and disconnect on destroy.
// 



// XXX to remove
class SignalOps {
public:

   
  

    template<typename Signal, typename Sender, typename... Args>
    static void
    disconnect(const Sender* sender, Args&&... args) {
        static_assert(isSignal<Signal>, "signal must be a Signal (declared with VGC_SIGNAL).");
        static_assert(isObject<Sender>, "Signals must reside in Objects");

        return sender->signalHub_.disconnect(std::forward<Args>(args)...);
    }


    template<typename PointerToMemberSignal, typename FunctionOrHandle>
    ConnectionHandle disconnect(const internal::SignalRef<PointerToMemberSignal>& sg, 
        FunctionOrHandle&& x) {
        return internal::SignalOps::disconnect<PointerToMemberSignal>(
            sg.object(), std::type_index(typeid(PointerToMemberSignal)),
            std::forward<FunctionOrHandle>(x));
    }
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
#define VGC_PARAMS_TYPE_(...) VGC_TRIM_VAEND_(VGC_TRANSFORM_(VGC_SIG_PTYPE_, __VA_ARGS__))
// ... must end with VaEnd
#define VGC_PARAMS_NAME_(...) VGC_TRIM_VAEND_(VGC_TRANSFORM_(VGC_SIG_PNAME_, __VA_ARGS__))
// ... must end with VaEnd
#define VGC_PARAMS_(...) VGC_TRIM_VAEND_(VGC_TRANSFORM_(VGC_SIG_PBOTH_, __VA_ARGS__))

#define VGC_SIG_FWD2_(t, n) std::forward<t>(n)
#define VGC_SIG_FWD_(x) VGC_SIG_FWD2_ x


// a->changed().emit(...)
// a->changed().connect(onChangedSlot(slot))
// 
// VGC_SIGNAL(changed, (args..))
// VGC_SLOT(onChangedSlot)
//
// SlotTag in Object
//

#include <iostream>
using namespace std;


struct Tag {};

struct Tag2 {};

struct Foo {
    template<typename Enable = void>
    Tag slot1() {
        std::cout << "slot1 getter called" << std::endl;
        return {};
    }

    void slot1(int a, double b) {
        std::cout << "slot function called" << std::endl;
    }

    template<typename Enable = void>
    Tag slot() {
        std::cout << "slot getter called" << std::endl;
        return {};
    }

    void slot() {
        std::cout << "slot function called" << std::endl;
    }


};

void connect(Tag) {}

int main() {
    Foo f;
    connect(f.slot1());

    connect(f.slot<>());
    f.slot();
    return 0;
}





#define VGC_SIGNAL_REFFUNC_POSTFIX_ Signal
#define VGC_SLOT_REFFUNC_POSTFIX_ Slot

/// Macro to define VGC Object Signals.
///
/// Example:
/// 
/// \code
/// struct A : vgc::core::Object {
///     VGC_SIGNAL(signal1);
///     VGC_SIGNAL(signal2, arg0);
/// };
/// \endcode
///
#define VGC_SIGNAL(...) VGC_EXPAND(VGC_SIGNAL_(__VA_ARGS__, VaEnd))
#define VGC_SIGNAL_(name_, ...)                                                                         \
    auto VGC_XCONCAT(name_, VGC_SIGNAL_REFFUNC_POSTFIX_)() {                                            \
        using PointerToMemberSignal = decltype(&ThisClass::name_);                                      \
        return SignalRef<PointerToMemberSignal>(this);                                                  \
    }                                                                                                   \
    ::vgc::core::internal::EmitCheck name_(VGC_PARAMS_(__VA_ARGS__)) const {                            \
        using MyClass_ = std::remove_pointer_t<decltype(this)>;                                         \
        CHECK_TYPE_IS_VGC_OBJECT(MyClass_);                                                             \
        static_assert(!VGC_CONSTEXPR_IS_ID_ADDRESSABLE_IN_CLASS_(SuperClass, name_),                    \
                      "Signal names are not allowed to be identifiers of superclass members.");         \
        using ::vgc::core::internal::SignalTransmitter;                                                 \
        using TransmitterType = SignalTransmitter<VGC_PARAMS_TYPE_(__VA_ARGS__)>;                       \
        std::type_index signalId(typeid(&ThisClass::name_));                                            \
        signalHub_.emit_<TransmitterType>(                                                              \
            VGC_TRIM_VAEND_(signalId, VGC_TRANSFORM_(VGC_SIG_FWD_, __VA_ARGS__)));                      \
    }
    
#define VGC_EMIT std::ignore =

// you can either declare:
// VGC_SLOT(foo, int);
// or define in class:
// VGC_SLOT(foo, int) { /* impl */ }
// 
#define VGC_SLOT(...) VGC_EXPAND(VGC_SLOT_(__VA_ARGS__, VaEnd))
#define VGC_SLOT_(name_, ...) \
    static_assert(!VGC_CONSTEXPR_IS_ID_ADDRESSABLE_IN_CLASS_(SuperClass, name_),        \
                  "Slot names are not allowed to be identifiers of superclass "         \
                  "members (except virtual slots).");                                   \
    auto VGC_XCONCAT(name_, VGC_SLOT_REFFUNC_POSTFIX_)() {                              \
        using PointerToMemberSlot = decltype(&ThisClass::name_);                        \
        return SlotRef<PointerToMemberSlot>(this, &ThisClass::name_);                   \
    }                                                                                   \
    void name_(VGC_PARAMS_(__VA_ARGS__))

#define VGC_VIRTUAL_SLOT(...) VGC_EXPAND(VGC_VIRTUAL_SLOT_(__VA_ARGS__, VaEnd))
#define VGC_VIRTUAL_SLOT_(name_, ...) \
    static_assert(!VGC_CONSTEXPR_IS_ID_ADDRESSABLE_IN_CLASS_(SuperClass, name_),        \
                  "Slot names are not allowed to be identifiers of superclass "         \
                  "members (except virtual slots).");                                   \
    auto VGC_XCONCAT(name_, VGC_SLOT_REFFUNC_POSTFIX_)() {                              \
        using PointerToMemberSlot = decltype(&ThisClass::name_);                        \
        return SlotRef<PointerToMemberSlot>(this, &ThisClass::name_);                   \
    }                                                                                   \
    virtual void name_(VGC_PARAMS_(__VA_ARGS__))


#define VGC_OVERRIDE_SLOT(...) VGC_EXPAND(VGC_OVERRIDE_SLOT_(__VA_ARGS__, VaEnd))
#define VGC_OVERRIDE_SLOT_(name, ...) \
    void name(VGC_PARAMS_(__VA_ARGS__)) override

#define VGC_DEFINE_SLOT(...) VGC_EXPAND(VGC_DEFINE_SLOT_(__VA_ARGS__, VaEnd))
#define VGC_DEFINE_SLOT_(cls, name, ...) \
    void cls::name(VGC_PARAMS_(__VA_ARGS__))



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
