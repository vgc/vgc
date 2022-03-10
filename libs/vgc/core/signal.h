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
//#include <vgc/core/object.h>
#include <vgc/core/stringid.h>

namespace vgc::core {

class Object;
using ConnectionHandle = UInt64;

namespace internal {

using SlotId = std::pair<const Object*, StringId>;
using FreeFuncId = void*;


template<typename R, typename... Args>
struct SignalHandlerTraitsDef_ {
    using ReturnType = R;
    using ArgsTuple = std::tuple<Args...>;
    static constexpr size_t arity = sizeof...(Args);
};

// primary with fallback
template<typename T, bool FallBack = false>
struct SignalHandlerTraits_ : SignalHandlerTraits_<decltype(&T::operator()), true> {};

// free function
template<typename R, typename... Args>
struct SignalHandlerTraits_<R (*)(Args...), false> : SignalHandlerTraitsDef_<R, Args...> {};

// member function
template<typename R, typename L, typename... Args>
struct SignalHandlerTraits_<R (L::*)(Args...), false> : SignalHandlerTraitsDef_<R, Args...> {};

// callable
template<typename R, typename L, typename... Args>
struct SignalHandlerTraits_<R (L::*)(Args...)const, true> : SignalHandlerTraitsDef_<R, Args...> {};

template<typename T>
using SignalHandlerTraits = SignalHandlerTraits_<std::remove_reference_t<T>>;


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

struct SignalTag {};

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
    FnType adaptHandler(Object* o, void (Object::* mfn)(SlotArgs...)) {
        return [=](Args... args) {
            auto&& argsTuple = std::forward_as_tuple(o, std::forward<Args>(args)...);
            applyPartial<1 + sizeof...(SlotArgs)>(mfn, argsTuple);
        };
    }

    // free functions and callables
    template<typename Handler>
    static inline
    FnType adaptHandler(Handler&& f) {
        using HTraits = SignalHandlerTraits<Handler>;
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


class SignalOps {
public:
    template<typename SenderObj, typename SignalT, typename... Args>
    static
    ConnectionHandle
    connect(const SenderObj* sender, SignalT SenderObj::* signal, Args&&... args) {
        static_assert(std::is_base_of_v<SignalTag, SignalT>, "Signals must be declared with VGC_SIGNAL");
        static_assert(std::is_base_of_v<Object, SenderObj>, "Signals must reside in Objects");

        return connectImpl(sender, (sender->*signal).impl_, std::forward<Args>(args)...);
    }

    // slot
    template<typename SenderObj, typename SignalImplT, typename ReceiverObj, typename... HandlerArgs>
    static
    ConnectionHandle
    connectImpl(const SenderObj* sender, const SignalImplT& sImpl, ReceiverObj* receiver, void (ReceiverObj::*mfn)(HandlerArgs...), const StringId& id) {
        static_assert(std::is_base_of_v<Object, ReceiverObj>, "Slots must reside in Objects");

        // XXX make sender listen on receiver destroy to automatically disconnect signals

        return sImpl.addListener_(SignalImplT::adaptHandler(receiver, mfn), receiver, id);
    }

    // free function
    template<typename SenderObj, typename SignalImplT, typename... HandlerArgs>
    static
    ConnectionHandle
    connectImpl(const SenderObj* sender, const SignalImplT& sImpl, void (*ffn)(HandlerArgs...)) {

        return sImpl.addListener_(SignalImplT::adaptHandler(ffn), ffn);
    }

    // callables
    template<typename SenderObj, typename SignalImplT, typename Fn>
    static
    ConnectionHandle
    connectImpl(const SenderObj* sender, const SignalImplT& sImpl, Fn&& fn) {

        return sImpl.addListener_(SignalImplT::adaptHandler(std::forward<Fn>(fn)));
    }

    template<typename SenderObj, typename SignalT, typename... Args>
    static void
    disconnect(const SenderObj* sender, SignalT SenderObj::* signal, Args&&... args) {
        static_assert(std::is_base_of_v<SignalTag, SignalT>, "Signals must be declared with VGC_SIGNAL");
        static_assert(std::is_base_of_v<Object, SenderObj>, "Signals must reside in Objects");

        return (sender->*signal).impl_.removeListener_(std::forward<Args>(args)...);
    }
};

} // namespace internal

// XXX temporary until all pieces of code use VGC_SIGNAL
template<typename... Args>
using Signal = internal::SignalImpl<void(Args...)>;

} // namespace vgc::core

// transform macro
#define VGC_TF_1_(F, x) F(x)
#define VGC_TF_2_(F, x, ...) F(x), VGC_TF_1_(F, __VA_ARGS__)
#define VGC_TF_3_(F, x, ...) F(x), VGC_TF_2_(F, __VA_ARGS__)
#define VGC_TF_4_(F, x, ...) F(x), VGC_TF_3_(F, __VA_ARGS__)
#define VGC_TF_5_(F, x, ...) F(x), VGC_TF_4_(F, __VA_ARGS__)
#define VGC_TF_6_(F, x, ...) F(x), VGC_TF_5_(F, __VA_ARGS__)
#define VGC_TF_7_(F, x, ...) F(x), VGC_TF_6_(F, __VA_ARGS__)
#define VGC_TF_8_(F, x, ...) F(x), VGC_TF_7_(F, __VA_ARGS__)
#define VGC_TF_9_(F, x, ...) F(x), VGC_TF_8_(F, __VA_ARGS__)
#define VGC_TF_EXPAND_(x) x
#define VGC_TF_DISPATCH_(_1,_2,_3,_4,_5,_6,_7,_8,_9,S,...) S 
#define VGC_TRANSFORM(F, ...) \
  VGC_TF_EXPAND_(VGC_TF_DISPATCH_(__VA_ARGS__,\
    VGC_TF_9_,VGC_TF_8_,VGC_TF_7_,VGC_TF_6_,\
    VGC_TF_5_,VGC_TF_4_,VGC_TF_3_,VGC_TF_2_,VGC_TF_1_\
  )(F,__VA_ARGS__))

#define VGC_SIG_PTYPE2_(t, n) t
#define VGC_SIG_PNAME2_(t, n) n
#define VGC_SIG_PBOTH2_(t, n) t n
#define VGC_SIG_PTYPE_(x) VGC_SIG_PTYPE2_ x
#define VGC_SIG_PNAME_(x) VGC_SIG_PNAME2_ x
#define VGC_SIG_PBOTH_(x) VGC_SIG_PBOTH2_ x

#define VGC_SIGNAL(name, ...) \
    class Signal_##name : ::vgc::core::internal::SignalTag { \
    public: \
        void emit(VGC_TRANSFORM(VGC_SIG_PBOTH_, __VA_ARGS__)) const { \
            impl_.emit(VGC_TRANSFORM(VGC_SIG_PNAME_, __VA_ARGS__)); \
        } \
    private: \
        friend class ::vgc::core::internal::SignalOps; \
        using SignalImplT = ::vgc::core::internal::SignalImpl<void(VGC_TRANSFORM(VGC_SIG_PTYPE_, __VA_ARGS__))>; \
        mutable SignalImplT impl_; \
    } name

#define VGC_SLOT_PRIVATE_TNAME_(name) \
    SlotPrivate_##name##_

#define VGC_SLOT_PRIVATE_(sname) \
    struct VGC_SLOT_PRIVATE_TNAME_(sname) { \
        static const ::vgc::core::StringId& name_() { \
            static ::vgc::core::StringId s(#sname); return s; \
        } \
    }

// you can either declare:
// VGC_SLOT(foo, int);
// or define in class:
// VGC_SLOT(foo, int) { /* impl */ }

#define VGC_SLOT(name, ...) \
    VGC_SLOT_PRIVATE_(name); \
    void name(VGC_TRANSFORM(VGC_SIG_PBOTH_, __VA_ARGS__))

#define VGC_VIRTUAL_SLOT(name, ...) \
    VGC_SLOT_PRIVATE_(name); \
    virtual void name(VGC_TRANSFORM(VGC_SIG_PBOTH_, __VA_ARGS__))

// XXX add comment to explain SlotPrivateVCheck_
#define VGC_OVERRIDE_SLOT(name, ...) \
    struct SlotPrivateVCheck_ : VGC_SLOT_PRIVATE_TNAME_(name) {}; \
    void name(VGC_TRANSFORM(VGC_SIG_PBOTH_, __VA_ARGS__)) override

#define VGC_DEFINE_SLOT(cls, name, ...) void cls::name(VGC_TRANSFORM(VGC_SIG_PBOTH_, __VA_ARGS__))


#define VGC_CONNECT_SLOT(sender, signalName, receiver, slotName) \
    ::vgc::core::internal::SignalOps::connect( \
        sender,   & std::remove_pointer_t<decltype(sender)>   ::signalName, \
        receiver, & std::remove_pointer_t<decltype(receiver)> ::slotName, \
        std::remove_pointer_t<decltype(receiver)>::VGC_SLOT_PRIVATE_TNAME_(slotName)::name_())

#define VGC_CONNECT_FUNC(sender, signalName, function) \
    ::vgc::core::internal::SignalOps::connect( \
        sender,   & std::remove_pointer_t<decltype(sender)>   ::signalName, \
        function)

#define VGC_CONNECT_EXPAND_(x) x
#define VGC_CONNECT3_(...) VGC_CONNECT_FUNC(__VA_ARGS__)
#define VGC_CONNECT4_(...) VGC_CONNECT_SLOT(__VA_ARGS__)
#define VGC_CONNECT_DISPATCH_(_1, _2, _3, _4, NAME, ...) NAME
#define VGC_CONNECT(...) VGC_CONNECT_EXPAND_(VGC_CONNECT_DISPATCH_(__VA_ARGS__, VGC_CONNECT_SLOT, VGC_CONNECT_FUNC, NOOVERLOAD, NOOVERLOAD)(__VA_ARGS__))


#define VGC_DISCONNECT_SLOT(sender, signalName, receiver, slotName) \
    ::vgc::core::internal::SignalOps::disconnect( \
        sender,   & std::remove_pointer_t<decltype(sender)>   ::signalName, \
        receiver, \
        std::remove_pointer_t<decltype(receiver)>::VGC_SLOT_PRIVATE_TNAME_(slotName)::name_())

#define VGC_DISCONNECT_HANDLE_OR_FUNC(sender, signalName, functionOrHandle) \
    ::vgc::core::internal::SignalOps::disconnect( \
        sender,   & std::remove_pointer_t<decltype(sender)>   ::signalName, \
        functionOrHandle)

#define VGC_DISCONNECT_EXPAND_(x) x
#define VGC_DISCONNECT3_(...) VGC_DISCONNECT_HANDLE_OR_FUNC(__VA_ARGS__)
#define VGC_DISCONNECT4_(...) VGC_DISCONNECT_SLOT(__VA_ARGS__)
#define VGC_DISCONNECT_DISPATCH_(_1, _2, _3, _4, NAME, ...) NAME
#define VGC_DISCONNECT(...) VGC_DISCONNECT_EXPAND_(VGC_DISCONNECT_DISPATCH_(__VA_ARGS__, VGC_DISCONNECT_SLOT, VGC_DISCONNECT_HANDLE_OR_FUNC, NOOVERLOAD, NOOVERLOAD)(__VA_ARGS__))

namespace vgc::core::internal {

template<typename ObjectT = ::vgc::core::Object>
class TestSignalObject : public ObjectT {
public:
    VGC_SIGNAL(signalIntDouble, (int, a), (double, b));
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
