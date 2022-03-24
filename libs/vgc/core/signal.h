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
#include <utility>
#include <variant>
#include <vector>

#include <iostream>

#include <vgc/core/api.h>
#include <vgc/core/array.h>
#include <vgc/core/stringid.h>

#include <vgc/core/internal/templateutil.h>

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
struct hasSuperClassTypedef_ : std::false_type {};
template<typename T>
struct hasSuperClassTypedef_<T, std::void_t<typename T::SuperClass>> : std::true_type {
    static_assert(std::is_same_v<typename T::SuperClass, typename T::SuperClass::ThisClass>,
        "Missing VGC_OBJECT(..) in T's superclass."); \
};

template<typename T, typename Enable = void>
struct isObject_ : std::false_type {};
template<typename T>
struct isObject_<T, std::enable_if_t<std::is_base_of_v<Object, T>, void>> : std::true_type {
    static_assert(hasSuperClassTypedef_<T>::value || std::is_same_v<T, Object>, \
        "Missing VGC_OBJECT(..) in T."); \
};
template <typename T>
inline constexpr bool isObject = isObject_<T>::value;

// Checks that Type inherits from Object.
// Also checks that VGC_OBJECT(..) has been used in Type.
#define CHECK_TYPE_IS_VGC_OBJECT(Type) \
    static_assert(::vgc::core::internal::isObject<Type>, #Type " should be a vgc::core::Object.");


using SignalId = StringId;
using SlotId = std::pair<const Object*, StringId>;
using FreeFuncId = void*;
using ConnectionHandle = UInt64;

struct SignalInfo {
    StringId name;
};

struct SlotInfo {
    StringId name;
};

// To enforce the use of VGC_EMIT.
struct [[VGC_NODISCARD("Please use VGC_EMIT.")]] EmitCheck {
    EmitCheck(const EmitCheck&) = delete;
    EmitCheck& operator=(const EmitCheck&) = delete;
};


struct SignalTraits {
    SignalTraits(const SignalTraits&) = delete;
    SignalTraits& operator=(const SignalTraits&) = delete;
};
template <typename T>
inline constexpr bool IsSignalTraits = std::is_base_of_v<SignalTraits, T>;


// XXX maybe unused after rework
template<typename T>
struct isSignal_ : std::false_type {};
template<typename R, typename C, typename... Args>
struct isSignal_<R (C::*)(Args...) const> {
    static constexpr bool value = IsSignalTraits<R>;
    static_assert(!value || isObject<C>, "Signals must reside in an Objects.");
};
template <typename T>
inline constexpr bool isSignal = isSignal_<T>::value;


template<typename R, typename... Args>
struct FunctionTraitsDef_ {
    using ReturnType = R;
    using ArgsTuple = std::tuple<Args...>;
    static constexpr size_t arity = sizeof...(Args);
};


template<typename MemberFunctionPointerType>
struct MemberFunctionTraits_;
template<typename R, typename C, typename... Args>
struct MemberFunctionTraits_<R (C::*)(Args...) const> : FunctionTraitsDef_<R, Args...> {};
template <typename R, typename C, typename... Args>
struct MemberFunctionTraits_<R (C::*)(Args...)> : FunctionTraitsDef_<R, Args...> {};
template<typename T>
using MemberFunctionTraits = MemberFunctionTraits_<std::remove_reference_t<T>>;


// callable's operator
template<typename T>
struct SignalCallableHandlerTraits_;
template<typename R, typename C, typename... Args>
struct SignalCallableHandlerTraits_<R (C::*)(Args...) const> : FunctionTraitsDef_<R, Args...> {};
// primary with fallback
template<typename T>
struct SignalFreeHandlerTraits_ : SignalCallableHandlerTraits_<decltype(&T::operator())> {};
// free function
template<typename R, typename... Args>
struct SignalFreeHandlerTraits_<R (*)(Args...)> : FunctionTraitsDef_<R, Args...> {};

template<typename T>
using SignalFreeHandlerTraits = SignalFreeHandlerTraits_<std::remove_reference_t<T>>;


template<class F, class ArgsTuple, std::size_t... I>
void applyPartialImpl(F&& f, ArgsTuple&& t, std::index_sequence<I...>) {
    std::invoke(std::forward<F>(f), std::get<I>(std::forward<ArgsTuple>(t))...);
}

template<size_t N, class F, class ArgsTuple>
void applyPartial(F&& f, ArgsTuple&& t) {
    applyPartialImpl(
        std::forward<F>(f), std::forward<ArgsTuple>(t),
        std::make_index_sequence<N>{});
}

VGC_CORE_API
ConnectionHandle genConnectionHandle();







// new impl

// This is a polymorphic adapter class for slots and free functions.
// It is used to store signal receivers in a single container as well as
// provide a common signature for all the connected receivers of a given
// signal. The input argument list is forwarded to the receiver and truncated
// if necessary.
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
    template<typename Obj, typename... SlotArgs>
    [[nodiscard]] static inline
    SignalTransmitter*
    create(Obj* o, void (Obj::* mfn)(SlotArgs...)) {

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
    SignalTransmitter*
    create(FreeHandler&& f) {
        using HTraits = SignalFreeHandlerTraits<FreeHandler>;
        static_assert(std::is_same_v<typename HTraits::ReturnType, void>, "Signal handlers must return void.");
        
        // optimization: when the target has already the desired signature
        if constexpr (std::is_same_v<std::tuple<SignalArgs&&...>, typename HTraits::ArgsTuple>) {
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

template<typename Signal>
struct SignalTransmitterTypeFor_;
template<typename R, typename ObjT, typename... SignalArgs>
struct SignalTransmitterTypeFor_<R (ObjT::*)(SignalArgs...) const> {
    using type = std::enable_if_t<IsSignalTraits<R>, SignalTransmitter<SignalArgs...>>;
};
template<typename SignalType>
using SignalTransmitterTypeFor = typename SignalTransmitterTypeFor_<SignalType>::type;


class SignalHub {
private:
    struct Connection_ {
        template<typename To>
        Connection_(std::unique_ptr<AbstractSignalTransmitter>&& f, ConnectionHandle h, SignalId from, To&& to) :
            f(std::move(f)), h(h), from(from), to(std::forward<To>(to)) {

        }

        std::unique_ptr<AbstractSignalTransmitter> f;
        ConnectionHandle h;
        SignalId from;
        std::variant<
            std::monostate,
            SlotId,
            FreeFuncId
        > to;
    };

    SignalHub(const SignalHub&) = delete;
    SignalHub& operator=(const SignalHub&) = delete;

public:
    SignalHub() = default;

    // slot
    template<typename Receiver>
    ConnectionHandle connectSlot(StringId signalId, std::unique_ptr<AbstractSignalTransmitter>&& transmitter, const Receiver* receiver, StringId slotName) {
        auto h = genConnectionHandle();
        // XXX use emplaceLast when possible
        connections_.emplace(connections_.end(), std::move(transmitter), h, signalId, SlotId{receiver, slotName});
        return h;
    }

    // free-function
    template<typename... FnArgs>
    ConnectionHandle connectCallback(StringId signalId, std::unique_ptr<AbstractSignalTransmitter>&& transmitter, void (*ffn)(FnArgs...)) {
        auto h = genConnectionHandle();
        // XXX use emplaceLast when possible
        connections_.emplace(connections_.end(), std::move(transmitter), h, signalId, FreeFuncId(ffn));
        return h;
    }

    // free-callables
    ConnectionHandle connectCallback(StringId signalId, std::unique_ptr<AbstractSignalTransmitter>&& transmitter) {
        auto h = genConnectionHandle();
        // XXX use emplaceLast when possible
        connections_.emplace(connections_.end(), std::move(transmitter), h, signalId, std::monostate{});
        return h;
    }

    // any
    void disconnect(StringId /*signalId*/, ConnectionHandle h) {
        removeConnectionIf_([=](const Connection_& c) {
            return c.h == h; 
        });
    }

    // slot
    void disconnect(StringId signalId, const Object* o, const StringId& slotName) {
        removeConnectionIf_([=](const Connection_& c) {
            return c.from == signalId && std::holds_alternative<SlotId>(c.to) && std::get<SlotId>(c.to) == SlotId(o, slotName);
        });
    }

    // free-callables
    void disconnect(StringId signalId, void* freeFunc) {
        removeConnectionIf_([=](const Connection_& c) {
            return c.from == signalId && std::holds_alternative<FreeFuncId>(c.to) && std::get<FreeFuncId>(c.to) == FreeFuncId(freeFunc);
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

class SignalOps {
public:
    // slot
    template<typename Signal, typename Sender, typename Receiver, typename... SlotArgs>
    static ConnectionHandle
    connect(const Sender* sender, StringId signalId, const Receiver* receiver, StringId slotName, void (Receiver::*slot)(SlotArgs...)) {
        static_assert(isSignal<Signal>, "signal must be a Signal (declared with VGC_SIGNAL).");
        static_assert(isObject<Sender>, "Signals must reside in Objects");
        static_assert(isObject<Receiver>, "Slots must reside in Objects");

        // XXX make sender listen on receiver destroy to automatically disconnect signals

        std::unique_ptr<AbstractSignalTransmitter> transmitter(
            SignalTransmitterTypeFor<Signal>::create(const_cast<Receiver*>(receiver), slot));

        return sender->signalHub_.connectSlot(signalId, std::move(transmitter), receiver, slotName);
    }

    // free-function
    template<typename Signal, typename Sender, typename... FnArgs>
    static ConnectionHandle
    connect(const Sender* sender, StringId signalId, void (*ffn)(FnArgs...)) {
        static_assert(isSignal<Signal>, "signal must be a Signal (declared with VGC_SIGNAL).");
        static_assert(isObject<Sender>, "Signals must reside in Objects");

        std::unique_ptr<AbstractSignalTransmitter> transmitter(
            SignalTransmitterTypeFor<Signal>::create(ffn));

        return sender->signalHub_.connectCallback(signalId, std::move(transmitter), ffn);
    }

    // free-callables
    template<typename Signal, typename Sender, typename Fn>
    static ConnectionHandle
    connect(const Sender* sender, StringId signalId, Fn&& fn) {
        static_assert(isSignal<Signal>, "signal must be a Signal (declared with VGC_SIGNAL).");
        static_assert(isObject<Sender>, "Signals must reside in Objects");

        std::unique_ptr<AbstractSignalTransmitter> transmitter(
            SignalTransmitterTypeFor<Signal>::create(std::forward<Fn>(fn)));

        return sender->signalHub_.connectCallback(signalId, std::move(transmitter));
    }

    template<typename Signal, typename Sender, typename... Args>
    static void
    disconnect(const Sender* sender, Args&&... args) {
        static_assert(isSignal<Signal>, "signal must be a Signal (declared with VGC_SIGNAL).");
        static_assert(isObject<Sender>, "Signals must reside in Objects");

        return sender->signalHub_.disconnect(std::forward<Args>(args)...);
    }
};

} // namespace internal

// XXX temporary until all pieces of code use VGC_SIGNAL
template<typename... Args>
using Signal = internal::SignalImpl<void(Args...)>;

} // namespace vgc::core


// XXX move this in pp.h

#define VGC_STR(x) #x
#define VGC_XSTR(x) VGC_STR(x)

#define VGC_EXPAND(x) x
#define VGC_EXPAND2(x) VGC_EXPAND(x)

#define VGC_FIRST_(a, ...) a
#define VGC_SUBLIST_1_END_(_0,...) __VA_ARGS__
#define VGC_SUBLIST_2_END_(_0,_1,...) __VA_ARGS__

#define VGC_TVE_0_(_)
#define VGC_TVE_1_(x, _)   x
#define VGC_TVE_2_(x, ...) x, VGC_EXPAND(VGC_TVE_1_(__VA_ARGS__))
#define VGC_TVE_3_(x, ...) x, VGC_EXPAND(VGC_TVE_2_(__VA_ARGS__))
#define VGC_TVE_4_(x, ...) x, VGC_EXPAND(VGC_TVE_3_(__VA_ARGS__))
#define VGC_TVE_5_(x, ...) x, VGC_EXPAND(VGC_TVE_4_(__VA_ARGS__))
#define VGC_TVE_6_(x, ...) x, VGC_EXPAND(VGC_TVE_5_(__VA_ARGS__))
#define VGC_TVE_7_(x, ...) x, VGC_EXPAND(VGC_TVE_6_(__VA_ARGS__))
#define VGC_TVE_8_(x, ...) x, VGC_EXPAND(VGC_TVE_7_(__VA_ARGS__))
#define VGC_TVE_9_(x, ...) x, VGC_EXPAND(VGC_TVE_8_(__VA_ARGS__))
#define VGC_TVE_DISPATCH_(_0,_1,_2,_3,_4,_5,_6,_7,_8,_9,S,...) S
// ... must end with VaEnd
#define VGC_TRIM_VAEND_(...) \
  VGC_EXPAND(VGC_TVE_DISPATCH_(__VA_ARGS__, \
    VGC_TVE_9_,VGC_TVE_8_,VGC_TVE_7_,VGC_TVE_6_,VGC_TVE_5_, \
    VGC_TVE_4_,VGC_TVE_3_,VGC_TVE_2_,VGC_TVE_1_,VGC_TVE_0_ \
  )(__VA_ARGS__))

// XXX check that the VGC_EXPAND fix for msvc works with all F
#define VGC_TF_0_(F, _)      VaEnd
#define VGC_TF_1_(F, x, ...) F(x), VaEnd
#define VGC_TF_2_(F, x, ...) F(x), VGC_EXPAND(VGC_TF_1_(F, __VA_ARGS__))
#define VGC_TF_3_(F, x, ...) F(x), VGC_EXPAND(VGC_TF_2_(F, __VA_ARGS__))
#define VGC_TF_4_(F, x, ...) F(x), VGC_EXPAND(VGC_TF_3_(F, __VA_ARGS__))
#define VGC_TF_5_(F, x, ...) F(x), VGC_EXPAND(VGC_TF_4_(F, __VA_ARGS__))
#define VGC_TF_6_(F, x, ...) F(x), VGC_EXPAND(VGC_TF_5_(F, __VA_ARGS__))
#define VGC_TF_7_(F, x, ...) F(x), VGC_EXPAND(VGC_TF_6_(F, __VA_ARGS__))
#define VGC_TF_8_(F, x, ...) F(x), VGC_EXPAND(VGC_TF_7_(F, __VA_ARGS__))
#define VGC_TF_9_(F, x, ...) F(x), VGC_EXPAND(VGC_TF_8_(F, __VA_ARGS__))
#define VGC_TF_DISPATCH_(_0,_1,_2,_3,_4,_5,_6,_7,_8,_9,S,...) S
// ... must end with VaEnd
#define VGC_TRANSFORM_(F, ...)                              \
  VGC_EXPAND(VGC_TF_DISPATCH_(__VA_ARGS__,                  \
    VGC_TF_9_,VGC_TF_8_,VGC_TF_7_,VGC_TF_6_,VGC_TF_5_,      \
    VGC_TF_4_,VGC_TF_3_,VGC_TF_2_,VGC_TF_1_,VGC_TF_0_       \
  )(F, __VA_ARGS__))

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


#ifndef DOXYGEN
#define VGC_SIGNAL_RET_TYPE_ auto
#define VGC_SIGNAL_RET_() return Traits{}
#else
#define VGC_SIGNAL_RET_TYPE_ void
#define VGC_SIGNAL_RET_()
#endif

#define VGC_SIGNAL_REFFUNC_POSTFIX_ _InternalSignalRef_

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
    VGC_SIGNAL_RET_TYPE_                                                                                \
    name_(VGC_PARAMS_(__VA_ARGS__)) const {                                                             \
        using MyClass_ = std::remove_pointer_t<decltype(this)>;                                         \
        CHECK_TYPE_IS_VGC_OBJECT(MyClass_);                                                             \
        static_assert(!VGC_CONSTEXPR_IS_ID_ADDRESSABLE_IN_CLASS_(SuperClass, name_),                    \
                      "Signal names are not allowed to be identifiers of superclass members.");         \
        using ::vgc::core::internal::SignalInfo;                                                        \
        using ::vgc::core::internal::SignalTransmitter;                                                 \
        using Parent = std::remove_pointer_t<decltype(this)>;                                           \
        struct [[nodiscard]] Traits : public ::vgc::core::internal::SignalTraits {                      \
            using MyClass = MyClass_;                                                                   \
            /* enable_if_t to remove unused-local-typedef warning */                                    \
            static std::enable_if_t<std::is_same_v<MyClass, MyClass>,                                   \
            const SignalInfo&> getInfo() {                                                              \
                static SignalInfo s = { ::vgc::core::StringId(#name_) };                                \
                return s;                                                                               \
            }                                                                                           \
        };                                                                                              \
        using TransmitterType = SignalTransmitter<VGC_PARAMS_TYPE_(__VA_ARGS__)>;                       \
        signalHub_.emit_<TransmitterType>(                                                              \
            VGC_TRIM_VAEND_(Traits::getInfo().name, VGC_TRANSFORM_(VGC_SIG_FWD_, __VA_ARGS__)));        \
        VGC_SIGNAL_RET_();                                                                              \
    }                                                                                                   \
    auto name_ ## VGC_SIGNAL_REFFUNC_POSTFIX_ () { return std::make_pair(this, &ThisClass::name_); }

#define VGC_EMIT std::ignore =


#define VGC_SLOT_REFFUNC_POSTFIX_ _InternalSlotRef_
#define VGC_SLOT_TRAITS_(name_) _internal_##name_
#define VGC_SLOT_REF_(name_) SlotRef##name_

#define VGC_SLOT_TRAITS_DEF_(name_)                                     \
    struct VGC_SLOT_TRAITS_(name_) {                                    \
        using MyClass = ThisClass;                                      \
        using SlotInfo = ::vgc::core::internal::SlotInfo;               \
        /* enable_if_t to remove unused-local-typedef warning */        \
        static std::enable_if_t<std::is_same_v<MyClass, MyClass>,       \
        const SlotInfo&> getInfo() {                                    \
            static SlotInfo s = {::vgc::core::StringId(#name_)};        \
            return s;                                                   \
        }                                                               \
    };

template<typename T, typename Traits_>
struct WithTraits : T {
    using T::T;
    using UnderlyingType = T;
    using Traits = Traits_;
};


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
    VGC_SLOT_TRAITS_DEF_(name_);                                                        \
    auto name_ ## VGC_SLOT_REFFUNC_POSTFIX_ () {                                        \
        auto pair = std::make_pair(this, &ThisClass::name_);                            \
        return WithTraits<decltype(pair), VGC_SLOT_TRAITS_(name_)>(std::move(pair));    \
    }                                                                                   \
    void name_(VGC_PARAMS_(__VA_ARGS__))

#define VGC_VIRTUAL_SLOT(...) VGC_EXPAND(VGC_VIRTUAL_SLOT_(__VA_ARGS__, VaEnd))
#define VGC_VIRTUAL_SLOT_(name_, ...) \
    static_assert(!VGC_CONSTEXPR_IS_ID_ADDRESSABLE_IN_CLASS_(SuperClass, name_),        \
                  "Slot names are not allowed to be identifiers of superclass "         \
                  "members (except virtual slots).");                                   \
    VGC_SLOT_TRAITS_DEF_(name_);                                                        \
    auto name_ ## VGC_SLOT_REFFUNC_POSTFIX_ () {                                        \
        auto pair = std::make_pair(this, &ThisClass::name_);                            \
        return WithTraits<decltype(pair), VGC_SLOT_TRAITS_(name_)>(std::move(pair));    \
    }                                                                                   \
    virtual void name_(VGC_PARAMS_(__VA_ARGS__))

// XXX add comment to explain SlotPrivateVCheck_
#define VGC_OVERRIDE_SLOT(...) VGC_EXPAND(VGC_OVERRIDE_SLOT_(__VA_ARGS__, VaEnd))
#define VGC_OVERRIDE_SLOT_(name, ...) \
    void name(VGC_PARAMS_(__VA_ARGS__)) override

#define VGC_DEFINE_SLOT(...) VGC_EXPAND(VGC_DEFINE_SLOT_(__VA_ARGS__, VaEnd))
#define VGC_DEFINE_SLOT_(cls, name, ...) \
    void cls::name(VGC_PARAMS_(__VA_ARGS__))


#define VGC_CONNECT_SLOT(sender, signalName, receiver, slotName)                    \
    [&]() -> ::vgc::core::ConnectionHandle {                                        \
        using Sender = std::remove_pointer_t<decltype(sender)>;                     \
        using Signal = decltype(&Sender::signalName);                               \
        using SignalTraits = typename ::vgc::core::internal::MemberFunctionTraits<Signal>::ReturnType; \
        using Receiver = std::remove_pointer_t<decltype(receiver)>;                 \
        using Slot = decltype(&Receiver::slotName);                                 \
        using SlotTraits = Receiver:: VGC_SLOT_TRAITS_(slotName);                   \
        static_assert(VGC_CONSTEXPR_IS_TYPE_DEFINED_IN_CLASS_(                      \
                          Receiver, VGC_SLOT_TRAITS_(slotName)),                    \
                      #slotName " is not a Slot! Did you forget VGC_SLOT ?");       \
        static_assert(VGC_CONSTEXPR_IS_ID_ADDRESSABLE_IN_CLASS_(                    \
                          Receiver, slotName),                                      \
                      "Slot " #slotName " has overloads, which is not allowed!");   \
        return ::vgc::core::internal::SignalOps::connect<Signal>(                   \
            sender, SignalTraits::getInfo().name,                                   \
            receiver, SlotTraits::getInfo().name,                                   \
            &Receiver::slotName);                                                   \
    }()

#define VGC_CONNECT_FUNC(sender, signalName, function)                              \
    [&]() -> ::vgc::core::ConnectionHandle {                                        \
        using Sender = std::remove_pointer_t<decltype(sender)>;                     \
        using Signal = decltype(&Sender::signalName);                               \
        using SignalTraits = typename ::vgc::core::internal::MemberFunctionTraits<Signal>::ReturnType; \
        return ::vgc::core::internal::SignalOps::connect<Signal>(                   \
            sender, SignalTraits::getInfo().name,                                   \
            function);                                                              \
    }()

#define VGC_CONNECT_SLOT2(signal, slot)                                             \
    [&]() -> ::vgc::core::ConnectionHandle {                                        \
        auto signalRef = signal ## VGC_SIGNAL_REFFUNC_POSTFIX_ ();                  \
        auto slotRef = slot ## VGC_SLOT_REFFUNC_POSTFIX_ ();                        \
        using Sender = std::remove_pointer_t<decltype(signalRef.first)>;            \
        using Signal = decltype(signalRef.second);                                  \
        using SignalTraits = typename ::vgc::core::internal::MemberFunctionTraits<Signal>::ReturnType; \
        using Receiver = std::remove_pointer_t<decltype(slotRef.first)>;            \
        using Slot = decltype(slotRef.second);                                      \
        using SlotTraits = decltype(slotRef)::Traits;                               \
        return ::vgc::core::internal::SignalOps::connect<Signal>(                   \
            signalRef.first, SignalTraits::getInfo().name,                          \
            slotRef.first, SlotTraits::getInfo().name,                              \
            slotRef.second);                                                        \
    }()


#define VGC_CONNECT_DISPATCH_(_1, _2, _3, _4, NAME, ...) NAME
#define VGC_CONNECT(...) VGC_EXPAND(VGC_CONNECT_DISPATCH_(__VA_ARGS__, VGC_CONNECT_SLOT, VGC_CONNECT_FUNC, VGC_CONNECT_SLOT2, NOOVERLOAD)(__VA_ARGS__))


#define VGC_DISCONNECT_SLOT(sender, signalName, receiver, slotName) \
    [&](){ \
        static ::vgc::core::StringId signalId_(#signalName); \
        static ::vgc::core::StringId slotName_(#slotName); \
        ::vgc::core::internal::SignalOps::disconnect<decltype(&std::remove_pointer_t<decltype(sender)>::signalName)>( \
            sender, signalId_, receiver, slotName_); \
    }()

#define VGC_DISCONNECT_HANDLE_OR_FUNC(sender, signalName, functionOrHandle) \
    [&](){ \
        static ::vgc::core::StringId signalId_(#signalName); \
        ::vgc::core::internal::SignalOps::disconnect<decltype(&std::remove_pointer_t<decltype(sender)>::signalName)>( \
            sender, signalId_, functionOrHandle); \
    }()

#define VGC_DISCONNECT_DISPATCH_(_1, _2, _3, _4, NAME, ...) NAME
#define VGC_DISCONNECT(...) VGC_EXPAND(VGC_DISCONNECT_DISPATCH_(__VA_ARGS__, VGC_DISCONNECT_SLOT, VGC_DISCONNECT_HANDLE_OR_FUNC, NOOVERLOAD, NOOVERLOAD)(__VA_ARGS__))


namespace vgc::core {

using ConnectionHandle = internal::ConnectionHandle;

} // namespace vgc::core


#endif // VGC_CORE_SIGNAL_H
