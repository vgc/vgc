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

#include <vgc/core/api.h>
#include <vgc/core/array.h>
#include <vgc/core/stringid.h>

namespace vgc::core {

class Object;
using ConnectionHandle = UInt64;

namespace internal {

using SignalId = StringId;
using SlotId = std::pair<const Object*, StringId>;
using FreeFuncId = void*;


//template <const char* const StringLiteral>
//struct StringIdSingleton {
//    StringIdSingleton(const StringIdSingleton&) = delete;
//    StringIdSingleton& operator=(const StringIdSingleton&) = delete;
//
//    static const StringId& get()
//    {
//        static StringId s(StringLiteral);
//        return s;
//    }
//
//private:
//    StringIdSingleton() {}
//    ~StringIdSingleton() {}
//};


// Serves as a reminder to use VGC_EMIT to emit signals.
// It is used as return type of signals in conjunction with [[nodiscard]].
//
struct YouForgot_VGC_EMIT {
    using me = YouForgot_VGC_EMIT;
    YouForgot_VGC_EMIT(const me&) = delete; 
    me& operator=(const me&) = delete;
};

// To be able to simplify doxygen doc
#define VGC_SIGNAL_RET_TYPE_ ::vgc::core::internal::YouForgot_VGC_EMIT

// Used to inline sfinae-based tests on type ArgType.
// see VGC_CONSTEXPR_IS_ID_ADDRESSABLE_IN_CLASS_ for an example.
//
template<typename ArgType>
struct LambdaSfinae {
    static constexpr bool check(...)  { return false; }
    template <class Lambda>
    static constexpr auto check(Lambda lambda) 
        -> decltype(lambda(std::declval<ArgType>()), bool{}) { return true; }
};

// compile-time evaluates to true only if &cls::id is a valid expression.
//
#define VGC_CONSTEXPR_IS_ID_ADDRESSABLE_IN_CLASS_(cls, id) \
    LambdaSfinae<cls*>::check( \
        [](auto* v) -> std::void_t<decltype(&std::remove_pointer_t<decltype(v)>::id)> {} \
    )


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

template<typename T>
struct isSignalImpl_ : std::false_type {};
template<typename C, typename... Args>
struct isSignalImpl_<VGC_SIGNAL_RET_TYPE_ (C::*)(Args...) const> : std::true_type {
    static_assert(isObject<C>, "Signals must reside in an Objects.");
};
template<typename T>
struct isSignal_ : isSignalImpl_<std::remove_const_t<std::decay_t<T>>> {};
template <typename T>
inline constexpr bool isSignal = isSignal_<T>::value;


// Checks that Type inherits from Object.
// Also checks that VGC_OBJECT(..) has been used in Type.
#define CHECK_TYPE_IS_VGC_OBJECT(Type) \
    static_assert(::vgc::core::internal::isObject<Type>, #Type " should be a vgc::core::Object.");


template<typename R, typename... Args>
struct FunctionTraitsDef_ {
    using ReturnType = R;
    using ArgsTuple = std::tuple<Args...>;
    static constexpr size_t arity = sizeof...(Args);
};


// callable's operator
template<typename T>
struct SignalCallableHandlerTraits_;
template<typename R, typename L, typename... Args>
struct SignalCallableHandlerTraits_<R (L::*)(Args...)const> : FunctionTraitsDef_<R, Args...> {};
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
 
template<size_t N, class F, class Tuple>
void applyPartial(F&& f, Tuple&& t) {
    applyPartialImpl(
        std::forward<F>(f), std::forward<Tuple>(t),
        std::make_index_sequence<N>{});
}


VGC_CORE_API
ConnectionHandle genConnectionHandle();

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


// previous impl (Simple impl)

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



// new impl

class SignalHandler {
protected:
    SignalHandler() = default;

public:
    virtual ~SignalHandler() = default;
};

template<typename SignalType>
class SignalHandlerTpl;

template<typename ObjT, typename... Args>
class SignalHandlerTpl<VGC_SIGNAL_RET_TYPE_ (ObjT::*)(Args...) const> :
    public SignalHandlerTpl<void(Args...)> {

};

template<typename... Args>
class SignalHandlerTpl<void(Args...)> : public SignalHandler {
public:
    using CFnType = void(Args&&...);
    using FnType = std::function<CFnType>;

protected:
    SignalHandlerTpl(FnType&& fn) : fn_(std::move(fn)) {

    }

public:
    void operator()(Args&&... args) const {
        fn_(std::forward<Args>(args)...);
    }

    // pointer-to-member-function
    template<typename ObjectT, typename... SlotArgs>
    static inline
    [[nodiscard]] SignalHandlerTpl*
    create(ObjectT* o, void (ObjectT::* mfn)(SlotArgs...)) {
        return new SignalHandlerTpl(
            [=](Args&&... args) {
                auto&& argsTuple = std::forward_as_tuple(o, std::forward<Args>(args)...);
                applyPartial<1 + sizeof...(SlotArgs)>(mfn, argsTuple);
            });
    }

    // free functions and callables
    template<typename FreeHandler>
    static inline
    [[nodiscard]] SignalHandlerTpl*
    create(FreeHandler&& f) {
        using HTraits = SignalFreeHandlerTraits<FreeHandler>;
        static_assert(std::is_same_v<typename HTraits::ReturnType, void>, "Signal handlers must return void.");
        return new SignalHandlerTpl(
            [=](Args&&... args) {
                auto&& argsTuple = std::forward_as_tuple(std::forward<Args>(args)...);
                applyPartial<HTraits::arity>(f, argsTuple);
            });
    }

private:
    FnType fn_;
};


class SignalMgr {
private:
    struct Connection_ {

        template<typename To>
        Connection_(SignalHandler* f, ConnectionHandle h, SignalId from, To&& to) :
            f(f), h(h), from(from), to(std::forward<To>(to)) {
        
        }

        std::unique_ptr<SignalHandler> f;
        ConnectionHandle h;
        SignalId from;
        std::variant<
            std::monostate,
            SlotId,
            FreeFuncId
        > to;
    };

    SignalMgr(const SignalMgr&) = delete;
    SignalMgr& operator=(const SignalMgr&) = delete;

public:
    SignalMgr() = default;

    // slot
    template<typename SignalT, typename Receiver, typename... SlotArgs>
    ConnectionHandle addSlotConnection(StringId signalId, const Receiver* receiver, StringId slotName, void (Receiver::* slot)(SlotArgs...)) {
        using HandlerType = SignalHandlerTpl<SignalT>;
        auto h = genConnectionHandle();
        auto signalHandler = HandlerType::create(const_cast<Receiver*>(receiver), slot);
        connections_.emplace(connections_.end(), signalHandler, h, signalId, SlotId{receiver, slotName});
        return h;
    }

    // free-function
    template<typename SignalT, typename... HandlerArgs>
    ConnectionHandle addConnection(StringId signalId, void (*ffn)(HandlerArgs...)) {
        using HandlerType = SignalHandlerTpl<SignalT>;
        auto h = genConnectionHandle();
        auto signalHandler = HandlerType::create(ffn);
        connections_.emplace(connections_.end(), signalHandler, h, signalId, FreeFuncId(ffn));
        return h;
    }

    // free-callables
    template<typename SignalT, typename Callable>
    ConnectionHandle addConnection(StringId signalId, Callable&& c) {
        using HandlerType = SignalHandlerTpl<SignalT>;
        auto h = genConnectionHandle();
        auto signalHandler = HandlerType::create(c);
        connections_.emplace(connections_.end(), signalHandler, h, signalId, std::monostate{});
        return h;
    }

    // any
    void removeConnection(StringId /*signalId*/, ConnectionHandle h) {
        removeConnectionIf_([=](const Connection_& c) {
            return c.h == h; 
        });
    }

    // slot
    void removeConnection(StringId signalId, const Object* o, const StringId& slotName) {
        removeConnectionIf_([=](const Connection_& c) {
            return c.from == signalId && std::holds_alternative<SlotId>(c.to) && std::get<SlotId>(c.to) == SlotId(o, slotName);
        });
    }

    // free-callables
    void removeConnection(StringId signalId, void* freeFunc) {
        removeConnectionIf_([=](const Connection_& c) {
            return c.from == signalId && std::holds_alternative<FreeFuncId>(c.to) && std::get<FreeFuncId>(c.to) == FreeFuncId(freeFunc);
        });
    }

    // DANGEROUS.
    // Args pack must be given explicitly !!
    //
    template<bool Enable, typename... Args>
    void emit_(SignalId id, Args&&... args) const {
        using HandlerType = SignalHandlerTpl<void(Args...)>;
        for (auto& c : connections_) {
            if (c.from == id) {
                // XXX replace with static_cast after initial test rounds
                const auto& f = *dynamic_cast<HandlerType*>(c.f.get());
                f(std::forward<Args>(args)...);
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
    template<typename SignalTp, typename Sender, typename Receiver, typename... SlotArgs>
    static ConnectionHandle
    connect(const Sender* sender, StringId signalId, const Receiver* receiver, StringId slotName, void (Receiver::*slot)(SlotArgs...)) {
        //static_assert(isSignal<SignalT>, "signal must be a Signal (declared with VGC_SIGNAL).");
        //static_assert(isObject<Sender>, "Signals must reside in Objects");
        //static_assert(isObject<Receiver>, "Slots must reside in Objects");

        // XXX make sender listen on receiver destroy to automatically disconnect signals

        return sender->signalMgr_.addSlotConnection<SignalTp >(signalId, receiver, slotName, slot);
    }

    // free-callables
    template<typename SignalT, typename Sender, typename Fn>
    static ConnectionHandle
    connect(const Sender* sender, StringId signalId, Fn&& fn) {
        static_assert(isSignal<SignalT>, "signal must be a Signal (declared with VGC_SIGNAL).");
        static_assert(isObject<Sender>, "Signals must reside in Objects");

        return sender->signalMgr_.addConnection<SignalT>(signalId, std::forward<Fn>(fn));
    }

    template<typename SignalT, typename Sender, typename... Args>
    static void
    disconnect(const Sender* sender, Args&&... args) {
        static_assert(isSignal<SignalT>, "signal must be a Signal (declared with VGC_SIGNAL).");
        static_assert(isObject<Sender>, "Signals must reside in Objects");

        return sender->signalMgr_.removeConnection(std::forward<Args>(args)...);
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
#define VGC_TRANSFORM_(F, ...) \
  VGC_EXPAND(VGC_TF_DISPATCH_(__VA_ARGS__, \
    VGC_TF_9_,VGC_TF_8_,VGC_TF_7_,VGC_TF_6_,VGC_TF_5_, \
    VGC_TF_4_,VGC_TF_3_,VGC_TF_2_,VGC_TF_1_,VGC_TF_0_ \
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

// unused
#define VGC_SLOT_PRIVATE_TNAME_(name) \
    SlotPrivate_##name##_
// unused
#define VGC_SLOT_PRIVATE_(sname) \
    struct VGC_SLOT_PRIVATE_TNAME_(sname) { \
        static const ::vgc::core::StringId& name_() { \
            static ::vgc::core::StringId s(#sname); return s; \
        } \
    }


//#define VGC_SIGNAL(name, ...) \
//    class Signal_##name : ::vgc::core::internal::SignalTag { \
//    public: \
//        void emit(VGC_TRANSFORM(VGC_SIG_PBOTH_, __VA_ARGS__)) const { \
//            impl_.emit(VGC_TRANSFORM(VGC_SIG_PNAME_, __VA_ARGS__)); \
//        } \
//    private: \
//        friend class ::vgc::core::internal::SignalOps; \
//        using SignalImplT = ::vgc::core::internal::SignalImpl<void(VGC_TRANSFORM(VGC_SIG_PTYPE_, __VA_ARGS__))>; \
//        mutable SignalImplT impl_; \
//    } name





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
#define VGC_SIGNAL_(name, ...) \
    [[nodiscard]] VGC_SIGNAL_RET_TYPE_ \
    name(VGC_PARAMS_(__VA_ARGS__)) const { \
        CHECK_TYPE_IS_VGC_OBJECT(std::remove_pointer_t<decltype(this)>); \
        static_assert(!VGC_CONSTEXPR_IS_ID_ADDRESSABLE_IN_CLASS_(SuperClass, name), "Signal names are not allowed to be identifiers of superclass members."); \
        static_assert(VGC_CONSTEXPR_IS_ID_ADDRESSABLE_IN_CLASS_(ThisClass, name), "Overloading signals is not allowed."); \
        static ::vgc::core::StringId signalId_(#name); \
        /* Argument types must be passed explicitly! */ \
        signalMgr_.emit_<true, VGC_PARAMS_TYPE_(__VA_ARGS__)>( \
            VGC_TRIM_VAEND_(signalId_, VGC_TRANSFORM_(VGC_SIG_FWD_, __VA_ARGS__))); \
        return {}; \
    }

#define VGC_EMIT std::ignore =

// you can either declare:
// VGC_SLOT(foo, int);
// or define in class:
// VGC_SLOT(foo, int) { /* impl */ }
// 
#define VGC_SLOT(...) VGC_EXPAND(VGC_SLOT_(__VA_ARGS__, VaEnd))
#define VGC_SLOT_(name, ...) \
    void name(VGC_PARAMS_(__VA_ARGS__))

#define VGC_VIRTUAL_SLOT(...) VGC_EXPAND(VGC_VIRTUAL_SLOT_(__VA_ARGS__, VaEnd))
#define VGC_VIRTUAL_SLOT_(name, ...) \
    static_assert(not VGC_CONSTEXPR_IS_ID_ADDRESSABLE_IN_CLASS_(SuperClass, name), "Slot names are not allowed to be identifiers of superclass members (except virtual slots)."); \
    static_assert(VGC_CONSTEXPR_IS_ID_ADDRESSABLE_IN_CLASS_(ThisClass, name), "Overloading slots is not allowed."); \
    virtual void name(VGC_PARAMS_(__VA_ARGS__))

// XXX add comment to explain SlotPrivateVCheck_
#define VGC_OVERRIDE_SLOT(...) VGC_EXPAND(VGC_OVERRIDE_SLOT_(__VA_ARGS__, VaEnd))
#define VGC_OVERRIDE_SLOT_(name, ...) \
    void name(VGC_PARAMS_(__VA_ARGS__)) override

#define VGC_DEFINE_SLOT(...) VGC_EXPAND(VGC_DEFINE_SLOT_(__VA_ARGS__, VaEnd))
#define VGC_DEFINE_SLOT_(cls, name, ...) \
    void cls::name(VGC_PARAMS_(__VA_ARGS__))


#define VGC_CONNECT_SLOT(sender, signalName, receiver, slotName) \
    [&](){ \
        static ::vgc::core::StringId signalId_(#signalName); \
        static ::vgc::core::StringId slotName_(#slotName); \
        return ::vgc::core::internal::SignalOps::connect<decltype(&std::remove_pointer_t<decltype(sender)>::signalName)>( \
            sender, signalId_, receiver, slotName_, \
            &std::remove_pointer_t<decltype(receiver)>::slotName); \
    }()

#define VGC_CONNECT_FUNC(sender, signalName, function) \
    [&](){ \
        static ::vgc::core::StringId signalId_(#signalName); \
        return ::vgc::core::internal::SignalOps::connect<decltype(&std::remove_pointer_t<decltype(sender)>::signalName)>( \
            sender, signalId_, function); \
    }()

#define VGC_CONNECT_EXPAND_(x) x
#define VGC_CONNECT3_(...) VGC_CONNECT_FUNC(__VA_ARGS__)
#define VGC_CONNECT4_(...) VGC_CONNECT_SLOT(__VA_ARGS__)
#define VGC_CONNECT_DISPATCH_(_1, _2, _3, _4, NAME, ...) NAME
#define VGC_CONNECT(...) VGC_CONNECT_EXPAND_(VGC_CONNECT_DISPATCH_(__VA_ARGS__, VGC_CONNECT_SLOT, VGC_CONNECT_FUNC, NOOVERLOAD, NOOVERLOAD)(__VA_ARGS__))


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

#define VGC_DISCONNECT_EXPAND_(x) x
#define VGC_DISCONNECT3_(...) VGC_DISCONNECT_HANDLE_OR_FUNC(__VA_ARGS__)
#define VGC_DISCONNECT4_(...) VGC_DISCONNECT_SLOT(__VA_ARGS__)
#define VGC_DISCONNECT_DISPATCH_(_1, _2, _3, _4, NAME, ...) NAME
#define VGC_DISCONNECT(...) VGC_DISCONNECT_EXPAND_(VGC_DISCONNECT_DISPATCH_(__VA_ARGS__, VGC_DISCONNECT_SLOT, VGC_DISCONNECT_HANDLE_OR_FUNC, NOOVERLOAD, NOOVERLOAD)(__VA_ARGS__))

namespace vgc::core::internal {

template<typename ObjectT = ::vgc::core::Object>
class TestSignalObject : public ObjectT {
public:
    using ThisClass = TestSignalObject;
    using SuperClass = ObjectT;

    VGC_SIGNAL(signalNoArgs);
    VGC_SIGNAL(signalInt, (int, a));
    VGC_SIGNAL(signalIntDouble, (int, a), (double, b));
    VGC_SIGNAL(signalIntDoubleBool, (int, a), (double, b), (bool, c));

    VGC_SLOT(slotInt, (int, a)) { slotIntCalled = true; }
    VGC_SLOT(slotIntDouble, (int, a), (double, b)) { slotIntDoubleCalled = true; }
    VGC_SLOT(slotUInt, (unsigned int, a)) { slotUIntCalled = true; }

    void selfConnect() {
        VGC_CONNECT(this, signalIntDouble, this, slotIntDouble);
        VGC_CONNECT(this, signalIntDouble, this, slotInt);
        VGC_CONNECT(this, signalIntDouble, this, slotUInt);
        VGC_CONNECT(this, signalIntDouble, staticFuncInt);
        VGC_CONNECT(this, signalIntDouble, std::function<void(int, double)>([&](int, double) { fnIntDoubleCalled = true; }));
        VGC_CONNECT(this, signalIntDouble, [&](int, double) { fnIntDoubleCalled = true; } );
        VGC_CONNECT(this, signalIntDouble, std::function<void(unsigned int)>([&](unsigned int) { fnUIntCalled = true; }));
        VGC_CONNECT(this, signalIntDouble, [&](unsigned int) { fnUIntCalled = true; });
    }

    static inline void staticFuncInt() {
        sfnIntCalled = true;
    }

    void resetFlags() {
        slotIntDoubleCalled = false;
        slotIntCalled = false;
        slotUIntCalled = false;
        sfnIntCalled = false;
        fnIntDoubleCalled = false;
        fnUIntCalled = false;
    }

    bool slotIntDoubleCalled = false;
    bool slotIntCalled = false;
    bool slotUIntCalled = false;
    static inline bool sfnIntCalled = false;
    bool fnIntDoubleCalled = false;
    bool fnUIntCalled = false;
};

} // namespace vgc::core::internal


#endif // VGC_CORE_SIGNAL_H
