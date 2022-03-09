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

namespace vgc {
namespace core {

class Object;
using ConnectionHandle = uintptr_t;

namespace internal {

using SlotId = std::pair<const Object*, StringId>;
using FreeFuncId = void*;

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
template<typename... Args>
class Signal {
private:
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

    /// Triggers the signal, that is, calls all connected functions.
    ///
    void emit(Args... args) const {
        for(const auto& l : listeners_) {
            l(args...);
        }
    }

    /// Deprecated
    // XXX Deprecated
    void connect(FnType fn) const {
        addListener(fn);
    }

private:
    // This class defines connect helpers and thus needs access to Signal internals.
    friend class SignalOps;

    struct Listener {
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

    mutable Array<Listener> listeners_;

    static ConnectionHandle genHandle()
    {
        static ConnectionHandle s = 0;
        // XXX make this thread-safe
        return ++s;
    }

    void removeListener(ConnectionHandle h) const {
        auto it = std::remove_if(listeners_.begin(), listeners_.end(), [h](const Listener& l) { return l == h; });
        listeners_.erase(it, listeners_.end());
    }

    void removeListener(Object* o, StringId slotName) const {
        auto it = std::remove_if(listeners_.begin(), listeners_.end(), [h](const Listener& l) {
            return std::holds_alternative<SlotId>(l.id) && std::get<SlotId>(l.id) == SlotId(o, slotName);
        });
        listeners_.erase(it, listeners_.end());
    }

    void removeListener(void* freeFunc) const {
        auto it = std::remove_if(listeners_.begin(), listeners_.end(), [h](const Listener& l) {
            return std::holds_alternative<FreeFuncId>(l.id) && l.id == freeFunc;
        });
        listeners_.erase(it, listeners_.end());
    }

    ConnectionHandle addListener(FnType fn) const
    {
        auto h = genHandle();
        listeners_.append({fn, h, std::monostate{}});
        return h;
    }
};



template <class F, class ArgsTuple, std::size_t... I>
void applyPartialImpl(F&& f, ArgsTuple&& t, std::index_sequence<I...>) {
    std::invoke(std::forward<F>(f), std::get<I>(std::forward<ArgsTuple>(t))...);
}
 
template <size_t N, class F, class Tuple>
void applyPartial(F&& f, Tuple&& t) {
    applyPartialImpl(
        std::forward<F>(f), std::forward<Tuple>(t),
        std::make_index_sequence<N>{});
}

//template<typename... Args>
//class SignalRef
//{
//    Object* po_;
//    Signal<Args...>* ps_;
//};

class SignalOps {
public:
    //template<typename... Args>
    //static void disconnect(Signal<Args...>& signal) {
    //
    //    // XXX 
    //}

    template<typename SlotPrivateT, typename SenderObj, typename ReceiverObj, typename... Args, typename... SlotArgs>
    static std::enable_if_t<std::conjunction_v<std::is_base_of<Object, SenderObj>, std::is_base_of<Object, ReceiverObj>>,
    ConnectionHandle>
    connect(
        const SenderObj* sender, Signal<Args...> SenderObj::* signal,
        const ReceiverObj* receiver, void (ReceiverObj::* mfn)(SlotArgs...)) {

        // XXX make sender listen on receiver destroy to automatically disconnect signals

        typename Signal<Args...>::FnType f = [=](Args... args) {
            auto&& argsTuple = std::forward_as_tuple(std::move(const_cast<ReceiverObj*>(receiver)), std::forward<Args>(args)...);
            internal::applyPartial<1 + sizeof...(SlotArgs)>(mfn, argsTuple);
        };

        auto& lz = (sender->*signal).listeners_;
        lz.emplace(lz.end(), typename Signal<Args...>::Listener{f, ConnectionHandle(), SlotId(receiver, SlotPrivateT::name())});
        return 0;
    }

   /* template<typename SenderObj, typename... Args,
             std::enable_if_t<std::conjunction_v<std::is_base_of<Object, SenderObj>>, int> = 0>
    static void connect(
        const SenderObj* sender, Signal<Args...> SenderObj::* signal,
        const std::function<void(Args...)>& fn) {

        sender.*signal.slots_.emplace_back(fn);
    }*/
};

} // namespace internal

// XXX temporary until all pieces of code use VGC_SIGNAL
template<typename... Args>
using Signal = internal::Signal<Args...>;

} // namespace core
} // namespace vgc


#define VGC_SIGNAL(name, ...) \
    mutable internal::Signal<__VA_ARGS__> name; \
    class SignalPrivate_##name##_ {}

#define VGC_SLOT_PRIVATE_TNAME_(name) \
    SlotPrivate_##name##_

#define VGC_SLOT_PRIVATE_(sname) \
    class VGC_SLOT_PRIVATE_TNAME_(sname) { \
        friend class ::vgc::core::internal::SignalOps; \
        static const StringId& name() { \
            static StringId s(#sname); return s; \
        } \
    }

// you can either declare:
// VGC_SLOT(foo, int);
// or define in class:
// VGC_SLOT(foo, int) { /* impl */ }

#define VGC_SLOT(name, ...) \
    VGC_SLOT_PRIVATE_(name); \
    void name(__VA_ARGS__)

#define VGC_VIRTUAL_SLOT(name, ...) \
    VGC_SLOT_PRIVATE_(name); \
    virtual void name(__VA_ARGS__)

#define VGC_OVERRIDE_SLOT(name, ...) \
    struct SlotPrivateVCheck_ : VGC_SLOT_PRIVATE_TNAME_(name) {}; \
    void name(__VA_ARGS__) override

#define VGC_DEFINE_SLOT(name, ...) void name(__VA_ARGS__)


#define VGC_CONNECT_SLOT(sender, signalName, receiver, slotName) \
    ::vgc::core::internal::SignalOps::connect<typename std::remove_pointer_t<decltype(receiver)>:: VGC_SLOT_PRIVATE_TNAME_(slotName)>( \
        sender, & std::remove_pointer_t<decltype(sender)> ::signalName, receiver, & std::remove_pointer_t<decltype(receiver)> ::slotName)

#define VGC_CONNECT_FUNC(sender, signalName, function) \
    ::vgc::core::internal::SignalOps::connect(sender, &(sender->signalName), function)

#define VGC_CONNECT_EXPAND_(x) x
#define VGC_CONNECT3_(...) VGC_CONNECT_FUNC(__VA_ARGS__)
#define VGC_CONNECT4_(...) VGC_CONNECT_SLOT(__VA_ARGS__)
#define VGC_CONNECT_DISPATCH_(_1, _2, _3, _4, NAME, ...) NAME
#define VGC_CONNECT(...) VGC_CONNECT_EXPAND_(VGC_CONNECT_DISPATCH_(__VA_ARGS__, VGC_CONNECT_SLOT, VGC_CONNECT_FUNC, UNUSED)(__VA_ARGS__))

namespace vgc::core::internal {

template<typename ObjectT = ::vgc::core::Object>
class TestSignalObject : public ObjectT {
public:
    VGC_SIGNAL(signalIntDouble, int, double);
    VGC_SLOT(slotInt, int) { slotIntCalled = true; }
    VGC_SLOT(slotIntDouble, int) { slotIntDoubleCalled = true; }

    bool slotIntCalled = false;
    bool slotIntDoubleCalled = false;
};

} // namespace vgc::core::internal


#endif // VGC_CORE_SIGNAL_H
