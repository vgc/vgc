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

#include <any>
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

using ObjectMethodId = Int;
using BoundObjectMethodId = std::pair<Object*, ObjectMethodId>;
using FreeFuncId = void*;
using SlotId = std::variant<std::monostate, BoundObjectMethodId, FreeFuncId>;
using SignalMethodId = ObjectMethodId;

struct ConnectionHandle {
    Int64 id;
};

template<typename C, bool IsCallOperator, typename R, typename... Args>
struct FunctionTraitsDef_ {
    using ReturnType = R;
    using Obj = C;
    using ArgsTuple = std::tuple<Args...>;
    static constexpr size_t arity = sizeof...(Args);
    static constexpr bool isCallOperator = IsCallOperator;
};

template<typename MemberFunctionPointer>
struct MethodTraits_;
template<typename R, typename C, typename... Args>
struct MethodTraits_<R (C::*)(Args...)> : FunctionTraitsDef_<C, false, R, Args...> {};
template <typename R, typename C, typename... Args>
struct MethodTraits_<R (C::*)(Args...) const> : FunctionTraitsDef_<C, false, R, Args...> {};
// Non-static member function traits.
template<typename T>
using MethodTraits = MethodTraits_<std::remove_reference_t<T>>;

// Traits for function object operator().
template<typename T>
struct FunctionObjectCallTraits_;
template<typename R, typename C, typename... Args>
struct FunctionObjectCallTraits_<R (C::*)(Args...)> : FunctionTraitsDef_<C, true, R, Args...> {};
template<typename R, typename C, typename... Args>
struct FunctionObjectCallTraits_<R (C::*)(Args...) const> : FunctionTraitsDef_<C, true, R, Args...> {};

template<typename T, typename Enable = void>
struct FunctionObjectTraits_;
template<typename T>
struct FunctionObjectTraits_<T, std::void_t<decltype(&T::operator())>> : 
    FunctionObjectCallTraits_<decltype(&T::operator())> {};
// Traits for function object.
template<typename T>
using FunctionObjectTraits = FunctionObjectTraits_<std::remove_reference_t<T>>;

template<typename T, typename Enable = void>
struct SimpleCallableTraits_;
template<typename T>
struct SimpleCallableTraits_<T, std::void_t<typename FunctionObjectTraits<T>::ReturnType>> : FunctionObjectTraits<T> {};
template<typename R, typename... Args>
struct SimpleCallableTraits_<R (*)(Args...), void> : FunctionTraitsDef_<void, false, R, Args...> {};
// Traits for Callables except class methods.
template<typename T>
using SimpleCallableTraits = SimpleCallableTraits_<std::remove_reference_t<T>>;

template<typename T, typename Enable = void>
struct isSimpleCallable_ : std::false_type {};
template<typename T>
struct isSimpleCallable_<T, typename SimpleCallableTraits<T>::ReturnType> : std::true_type {};
template <typename T>
inline constexpr bool isSimpleCallable = isSimpleCallable_<T>::value;

VGC_CORE_API
ConnectionHandle genConnectionHandle();

VGC_CORE_API
ObjectMethodId genObjectMethodId();

template<typename ObjectMethodTag>
class ObjectMethodIdSingleton_ {
public:
    static ObjectMethodId get() {
        static ObjectMethodId s = genObjectMethodId();
        return s;
    }

private:
    ObjectMethodIdSingleton_() = delete;
};


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
class AbstractSignalTransmitterOld {
protected:
    AbstractSignalTransmitterOld() = default;

public:
    virtual ~AbstractSignalTransmitterOld() = default;
};


template<typename... SignalArgs>
class SignalTransmitterOld : public AbstractSignalTransmitterOld {
public:
    using SignalArgsTuple = std::tuple<SignalArgs...>;
    using CallArgsTuple = std::tuple<SignalArgs&&...>;
    using CallSig = void(SignalArgs&&...);
    using FnType = std::function<CallSig>;

    // left public for python bindings
    SignalTransmitterOld(const FnType& fn) :
        fn_(fn) {}

    // left public for python bindings
    SignalTransmitterOld(FnType&& fn) :
        fn_(std::move(fn)) {}

    inline void operator()(SignalArgs&&... args) const {
        fn_(std::forward<SignalArgs>(args)...);
    }

    // pointer-to-member-function
    template<typename R, typename Obj, typename... SlotArgs>
    [[nodiscard]] static inline
        SignalTransmitterOld* create(Obj* o, R (Obj::* method)(SlotArgs...)) {
        static_assert(sizeof...(SignalArgs) >= sizeof...(SlotArgs),
            "The slot signature cannot be longer than the signal signature.");
        static_assert(std::is_same_v<std::tuple<SlotArgs...>, SubPackAsTuple<0, sizeof...(SlotArgs), SignalArgs...>>,
            "The slot signature does not match the signal signature.");
        return new SignalTransmitterOld(
            [=](SignalArgs&&... args) {
                applyPartial<1 + sizeof...(SlotArgs)>(method, std::forward_as_tuple(o, std::forward<SignalArgs>(args)...));
            });
    }

    // free functions and callables
    template<typename FreeHandler>
    [[nodiscard]] static inline
        SignalTransmitterOld* create(FreeHandler&& f) {
        using HTraits = SimpleCallableTraits<FreeHandler>;
        // optimization: when the target has already the desired signature
        if constexpr (std::is_same_v<CallArgsTuple, typename HTraits::ArgsTuple>) {
            return new SignalTransmitterOld(std::forward<FreeHandler>(f));
        }
        return new SignalTransmitterOld(
            [=](SignalArgs&&... args) {
                applyPartial<HTraits::arity>(f, std::forward_as_tuple(std::forward<SignalArgs>(args)...));
            });
    }

private:
    FnType fn_;
};


template<typename CommonArgsTuple>
struct SlotFunctorBuilder_;
template<typename... CommonArgs>
struct SlotFunctorBuilder_<std::tuple<CommonArgs...>> {

    using FnType = std::function<void(CommonArgs&&...)>;

    // XXX merge both into one, with BindArgs for instance ?

    template<typename SimpleCallable>
    [[nodiscard]] static FnType create(SimpleCallable&& f) {
        static_assert(std::is_invocable_v<SimpleCallable, CommonArgs&&...>,
            "Slot does not match the signal signature, even with truncation and implicit conversions.");
        return FnType([=](CommonArgs&&... args) {
            f(std::forward<CommonArgs>(args)...);
        });
    }

    template<typename R, typename Obj, typename... SlotArgs>
    [[nodiscard]] static FnType create(Obj* o, R (Obj::* method)(SlotArgs...)) {
        static_assert(std::is_invocable_v<decltype(method), Obj*, CommonArgs&&...>,
            "Slot does not match the signal signature, even with truncation and implicit conversions.");
        return FnType([=](CommonArgs&&... args) {
            (o->*method)(std::forward<CommonArgs>(args)...);
        });
    }
};

class SignalTransmitter {
protected:
    template<typename... Args>
    SignalTransmitter(std::function<void(Args...)> fn) :
        slotFn_(std::move(fn)), slotArity_(sizeof...(Args)) {}

public:
    // pointer-to-member-function
    template<typename SignalArgsTuple, typename R, typename Obj, typename... SlotArgs>
    [[nodiscard]] static inline SignalTransmitter* create(Obj* o, R (Obj::* method)(SlotArgs...)) {
        constexpr size_t slotArity = sizeof...(SlotArgs);
        using CommonArgsTuple = SubTuple<0, sizeof...(SlotArgs), SignalArgsTuple>;
        static_assert(std::tuple_size_v<SignalArgsTuple> >= slotArity,
            "The slot signature cannot be longer than the signal signature.");
        return new SignalTransmitter(
            SlotFunctorBuilder_<CommonArgsTuple>::create(o, method));
    }

    // simple callables
    template<typename SignalArgsTuple, typename SimpleCallable>
    [[nodiscard]] static inline SignalTransmitter* create(SimpleCallable&& f) {
        using Traits = SimpleCallableTraits<SimpleCallable>;
        constexpr size_t slotArity = std::tuple_size_v<typename Traits::ArgsTuple>;
        using CommonArgsTuple = SubTuple<0, slotArity, SignalArgsTuple>;
        static_assert(std::tuple_size_v<SignalArgsTuple> >= slotArity,
            "The slot signature cannot be longer than the signal signature.");
        return new SignalTransmitter(
            SlotFunctorBuilder_<CommonArgsTuple>::create(std::forward<SimpleCallable>(f)));
    }

    // Limited to 7 args atm.
    template<typename... SignalArgs>
    void transmit(SignalArgs&&... args) {
        using SignalArgsTuple = std::tuple<SignalArgs...>;
        switch (slotArity_) {
        // XXX use macro
        case 1: if constexpr (sizeof...(SignalArgs) >= 1) {
            using FnType = typename SlotFunctorBuilder_<SubTuple<0, 1, SignalArgsTuple>>::FnType;
            auto* f = std::any_cast<FnType>(&slotFn_);
            applyPartial<1>(*f, std::forward_as_tuple(std::forward<SignalArgs>(args)...));
        }

        }
    }

    const UInt16 slotArity() const {
        return slotArity_;
    }

private:
    const std::any slotFn_;
    UInt16 slotArity_;
};


// Its member functions are static because we have to operate from the context of its owner Object.
class SignalHub {
private:
    struct Connection_ {
        template<typename To>
        Connection_(std::unique_ptr<SignalTransmitter>&& f, ConnectionHandle h, SignalMethodId from, To&& to) :
            f(std::move(f)), h(h), from(from), to(std::forward<To>(to)) {}

        std::unique_ptr<SignalTransmitter> f;
        ConnectionHandle h;
        SignalMethodId from;
        SlotId to;
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

    static ConnectionHandle connect(Object* sender, SignalMethodId from, std::unique_ptr<SignalTransmitter>&& transmitter, const SlotId& to) {
        auto& hub = access(sender);
        auto h = genConnectionHandle();
        hub.connections_.emplaceLast(std::move(transmitter), h, from, to);
        return h;
    }

    // XXX add to public interface of Object
    static bool disconnect(Object* sender, ConnectionHandle h) {
        return removeConnectionIf_(sender, [=](const Connection_& c) {
            return c.h.id == h.id; 
        });
    }

    static bool disconnect(Object* sender, SignalMethodId from) {
        return removeConnectionIf_(sender, [=](const Connection_& c) {
            return c.from == from;
        });
    }

    static bool disconnect(Object* sender, SignalMethodId from, ConnectionHandle h) {
        return removeConnectionIf_(sender, [=](const Connection_& c) {
            return c.from == from && c.h.id == h.id;
        });
    }

    static bool disconnect(Object* sender, SignalMethodId from, const SlotId& to) {
        return removeConnectionIf_(sender, [=](const Connection_& c) {
            return c.from == from && c.to == to;
        });
    }

    // DANGEROUS.
    //
    template<typename... Args>
    static void emit_(Object* sender, SignalMethodId from, Args&&... args) {
        auto& hub = access(sender);
        for (auto& c : hub.connections_) {
            if (c.from == from) {
                const auto* t = c.f.get();
                //t->transmit<Args...>(std::forward<Args>(args)...);
            }
        }
    }

protected:

    static void disconnect_(Object* sender, Object* receiver) {
        auto& hub = access(sender);
        auto it = std::remove_if(hub.connections_.begin(), hub.connections_.end(), [=](const Connection_& c) {
                return std::holds_alternative<BoundObjectMethodId>(c.to) && std::get<BoundObjectMethodId>(c.to).first == receiver; 
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
                    if (std::holds_alternative<BoundObjectMethodId>(c.to)) {
                        // Decrement numInboundConnections in the receiver's info about sender.
                        const auto& bsid = std::get<BoundObjectMethodId>(c.to);
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


template<typename ObjectMethodTag, typename SlotMethodT>
class SlotRef {
public:
    using SlotMethod = SlotMethodT;
    using SlotMethodTraits = MethodTraits<SlotMethod>;
    using Obj = typename SlotMethodTraits::Obj;
    using ArgsTuple = typename SlotMethodTraits::ArgsTuple;

    static_assert(isObject<Obj>, "Slots must be declared only in Objects");

    SlotRef(const Obj* object, SlotMethod m) :
        object_(const_cast<Obj*>(object)), m_(m) {}

    SlotRef(const SlotRef&) = delete;
    SlotRef& operator=(const SlotRef&) = delete;

    static constexpr ObjectMethodId id() {
        return ObjectMethodIdSingleton_<ObjectMethodTag>::get();
    }

    constexpr Obj* object() const {
        return object_;
    }

    constexpr const SlotMethod& method() const {
        return m_;
    }

private:
    Obj* const object_;
    SlotMethod const m_;
};

// It does not define emit(..) and is intended to be locally subclassed in
// the signal getter.
//
template<typename ObjectMethodTag, typename ObjT, typename... Args>
class SignalRef {
public:
    using Obj = ObjT;
    using ArgsTuple = std::tuple<Args...>;

    static_assert(isObject<Obj>, "Signals must be declared only in Objects");

    SignalRef(const Obj* object) :
        object_(const_cast<Obj*>(object)) {}

    SignalRef(const SignalRef&) = delete;
    SignalRef& operator=(const SignalRef&) = delete;

    static constexpr SignalMethodId id() {
        return ObjectMethodIdSingleton_<ObjectMethodTag>::get();;
    }

    constexpr Obj* object() const {
        return object_;
    }

    template<typename ObjectMethodTag, typename SlotMethodT>
    ConnectionHandle connect(SlotRef<ObjectMethodTag, SlotMethodT>&& slotRef) const {
        // XXX make owner listen on receiver destroy to automatically disconnect signals
        std::unique_ptr<SignalTransmitter> transmitter(
            SignalTransmitter::create<ArgsTuple>(slotRef.object(), slotRef.method()));
        return SignalHub::connect(object_, id(), std::move(transmitter), BoundObjectMethodId(slotRef.object(), slotRef.id()));
    }

    template<typename R, typename... FnArgs>
    ConnectionHandle connect(R (*callback)(FnArgs...)) const {
        std::unique_ptr<SignalTransmitter> transmitter(
            SignalTransmitter::create<ArgsTuple>(callback));
        return SignalHub::connect(object_, id(), std::move(transmitter), FreeFuncId(callback));
    }

    template<typename FunctionObject, std::enable_if_t<SimpleCallableTraits<FunctionObject>::isCallOperator, int> = 0>
    ConnectionHandle connect(FunctionObject&& funcObj) const {
        std::unique_ptr<SignalTransmitter> transmitter(
            SignalTransmitter::create<ArgsTuple>(funcObj));
        return SignalHub::connect(object_, id(), std::move(transmitter), std::monostate{});
    }

    bool disconnect() const {
        return SignalHub::disconnect(object_, id());
    }

    bool disconnect(ConnectionHandle h) const {
        return SignalHub::disconnect(object_, id(), h);
    }

    template<typename ObjectMethodTag, typename SlotMethodT>
    bool disconnect(SlotRef<ObjectMethodTag, SlotMethodT>&& slotRef) const {
        return SignalHub::disconnect(object_, id(), BoundObjectMethodId(slotRef.object(), slotRef.id()));
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
        using SignalRefBase = ::vgc::core::internal::SignalRef<                                     \
            Tag, VGC_PARAMS_TYPE_((MyClass,), __VA_ARGS__)>;                                            \
        class SignalRef : public SignalRefBase {                                                        \
        public:                                                                                         \
            using SignalRefBase::SignalRefBase;                                                         \
            ::vgc::core::internal::EmitCheck                                                            \
            emit(VGC_PARAMS_(__VA_ARGS__)) const {                                                      \
                SignalHub::emit_<VGC_PARAMS_TYPE_(__VA_ARGS__)>(                                        \
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
        using SlotRefBase = ::vgc::core::internal::SlotRef<                                             \
            Tag, decltype(&MyClass::name_)>;                                                        \
        class SlotRef : public SlotRefBase {                                                            \
        public:                                                                                         \
            using SlotRefBase::SlotRefBase;                                                             \
        };                                                                                              \
        return SlotRef(this, &MyClass::name_);                                                          \
    }

#define VGC_SLOT_ALIAS(name_, funcName_)                                                                \
    auto name_() {                                                                                      \
        struct Tag {};                                                                                  \
        using MyClass = std::remove_pointer_t<decltype(this)>;                                          \
        using SlotRefBase = ::vgc::core::internal::SlotRef<                                             \
            Tag, decltype(&MyClass::funcName_)>;                                                        \
        class SlotRef : public SlotRefBase {                                                            \
        public:                                                                                         \
            using SlotRefBase::SlotRefBase;                                                             \
        };                                                                                              \
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
        using HTraits = SimpleCallableTraits<Handler>;
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
