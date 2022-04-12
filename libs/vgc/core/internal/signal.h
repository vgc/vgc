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

#ifndef VGC_CORE_INTERNAL_SIGNAL_H
#define VGC_CORE_INTERNAL_SIGNAL_H

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
    std::is_same<std::remove_const_t<T>, typename T::ThisClass> {};

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

    constexpr ConnectionHandle(Int64 id)
        : id(id) {}

    static const ConnectionHandle invalid;
};

inline constexpr ConnectionHandle ConnectionHandle::invalid = {-1};

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
        using HTraits = CallableTraits<FreeHandler>;
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


template<typename TruncatedSignalArgsTuple>
struct SignalTransmitterFnType_;
template<typename... TruncatedSignalArgs>
struct SignalTransmitterFnType_<std::tuple<TruncatedSignalArgs...>> {
    using type = std::function<void(TruncatedSignalArgs&&...)>;
};
template<typename TruncatedSignalArgsTuple>
using SignalTransmitterFnType = typename SignalTransmitterFnType_<TruncatedSignalArgsTuple>::type;

class SignalTransmitter {
protected:
    template<typename... Args>
    SignalTransmitter(std::function<void(Args...)>&& fn) :
        fn_(std::move(fn)), arity_(sizeof...(Args)) {}

public:
    template<typename SignalArgsTuple, typename Callable, typename... OptionalObj>
    [[nodiscard]] static std::enable_if_t<isCallable<RemoveCRef<Callable>>, SignalTransmitter>
    build(Callable&& c, OptionalObj... methodObj) {
        using Traits = CallableTraits<Callable>;

        static_assert(std::tuple_size_v<SignalArgsTuple> >= Traits::arity,
            "The slot signature cannot be longer than the signal signature.");
        static_assert(std::tuple_size_v<SignalArgsTuple> < 8,
            "Signals and Slots are limited to 7 parameters.");

        using TruncatedSignalArgsTuple = SubTuple<0, Traits::arity, SignalArgsTuple>;
        
        if constexpr (isMethod<RemoveCRef<Callable>>) {
            static_assert(sizeof...(OptionalObj) == 1,
                "Expecting one methodObj to bind the given method.");
            using Obj = std::remove_reference_t<
                std::tuple_element_t<0, std::tuple<OptionalObj&&...>> >;
            static_assert(std::is_pointer_v<Obj>,
                "Requires methodObj to be passed by pointer.");
            static_assert(std::is_convertible_v<Obj, typename Traits::This>,
                "The given methodObj cannot be used to bind the given method.");
        }
        else {
            static_assert(sizeof...(OptionalObj) == 0,
                "Expecting methodObj only for methods, the given callable is not a method.");
        }

        auto fn = createFn(
            std::forward<Callable>(c),
            static_cast<TruncatedSignalArgsTuple*>(nullptr),
            methodObj...);

        return SignalTransmitter(std::move(fn));
    }

    // Limited to 7 args atm.
    template<typename SignalArgsTuple, typename... SignalArgs>
    void transmit(SignalArgs&&... args) const {

        // XXX add check size of SignalArgsTuple is sizeof...(SignalArgs)

        try {
            switch (arity_) {
            case 0: {
                std::any_cast<std::function<void()>>(fn_)();
                break;
            }

    #define VGC_SIGNALTRANSMITTER_TRANSMIT_CASE(i)                                                  \
            case i: if constexpr (sizeof...(SignalArgs) >= i) {                                     \
                using TruncatedSignalArgsTuple = SubTuple<0, i, SignalArgsTuple>;                   \
                auto f = getFn<TruncatedSignalArgsTuple>();                                 \
                applyPartial<i>(f, std::forward_as_tuple(std::forward<SignalArgs>(args)...));       \
                break;                                                                              \
            }                                                                                       \
            [[fallthrough]]

            VGC_SIGNALTRANSMITTER_TRANSMIT_CASE(1);
            VGC_SIGNALTRANSMITTER_TRANSMIT_CASE(2);
            VGC_SIGNALTRANSMITTER_TRANSMIT_CASE(3);
            VGC_SIGNALTRANSMITTER_TRANSMIT_CASE(4);
            VGC_SIGNALTRANSMITTER_TRANSMIT_CASE(5);
            VGC_SIGNALTRANSMITTER_TRANSMIT_CASE(6);
            VGC_SIGNALTRANSMITTER_TRANSMIT_CASE(7);

    #undef VGC_SIGNALTRANSMITTER_TRANSMIT_CASE

            default:
                throw core::LogicError("Less signal arguments than expected in SignalTransmitter call operator.");
                break;
            }
        }
        catch (std::bad_any_cast& e) {
            throw core::LogicError("Invalid arguments given to SignalTransmitter call operator.");
        }
    }

    const UInt8 slotArity() const {
        return arity_;
    }

    template<typename TruncatedSignalArgsTuple>
    const auto& getFn() const {
        using FnType = SignalTransmitterFnType<TruncatedSignalArgsTuple>;
        return *std::any_cast<FnType>(&fn_);
    }

protected:
    template<typename Callable, typename... TruncatedSignalArgs, typename... OptionalObj>
    [[nodiscard]] static auto createFn(Callable&& c, std::tuple<TruncatedSignalArgs...>* sig, OptionalObj&&... methodObj) {
        using Traits = CallableTraits<Callable>;

        static_assert(std::is_invocable_v<Callable&&, OptionalObj&&..., TruncatedSignalArgs&&...>,
            "Slot signature is not compatible with signal.");

        return std::function<void(TruncatedSignalArgs&&...)>(
            [=](TruncatedSignalArgs&&... args) {
                std::invoke(c, methodObj..., std::forward<TruncatedSignalArgs>(args)...);
            });
    }

private:
    std::any fn_;
    UInt8 arity_;
};


// Its member functions are static because we have to operate from the context of its owner Object.
class SignalHub {
private:
    struct Connection_ {
        template<typename To>
        Connection_(SignalTransmitter&& transmitter, ConnectionHandle h, SignalMethodId from, To&& to) :
            transmitter(std::move(transmitter)), h(h), from(from), to(std::forward<To>(to)) {}

        SignalTransmitter transmitter;
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

    static ConnectionHandle connect(Object* sender, SignalMethodId from, SignalTransmitter&& transmitter, const SlotId& to) {
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

    template<typename SignalArgsTuple, typename... Args>
    static void emit_(Object* sender, SignalMethodId from, Args&&... args) {
        auto& hub = access(sender);
        for (auto& c : hub.connections_) {
            if (c.from == from) {
                c.transmitter.transmit<SignalArgsTuple>(std::forward<Args>(args)...);
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
        auto& info = listenedObjectInfos_.emplaceLast();
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

// It is intended to be locally subclassed in the slot (getter).
//
template<typename ObjectMethodTag, typename SlotMethodT>
class SlotRef {
public:
    using SlotMethod = SlotMethodT;
    using SlotMethodTraits = MethodTraits<SlotMethod>;
    using Obj = typename SlotMethodTraits::Obj;
    using ArgsTuple = typename SlotMethodTraits::ArgsTuple;

    static_assert(isObject<Obj>, "Slots must be declared only in Objects");

protected:
    SlotRef(const Obj* object, SlotMethod m) :
        object_(const_cast<Obj*>(object)), m_(m) {}

public:
    // Non-Copyable
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
    Obj* object_;
    SlotMethod m_;
};

template<typename T>
inline constexpr bool isSlotRef = IsTplBaseOf<SlotRef, T>;

// It does not define emit(..) and is intended to be locally subclassed in
// the signal (getter).
//
template<typename ObjectMethodTag, typename ObjT, typename... Args>
class SignalRef {
public:
    using Obj = ObjT;
    using ArgsTuple = std::tuple<Args...>;

    static_assert(isObject<Obj>, "Signals must be declared only in Objects");

protected:
    SignalRef(const Obj* object) :
        object_(const_cast<Obj*>(object)) {}

public:
    // Non-Copyable
    SignalRef(const SignalRef&) = delete;
    SignalRef& operator=(const SignalRef&) = delete;

    // XXX doc
    static constexpr SignalMethodId id() {
        return ObjectMethodIdSingleton_<ObjectMethodTag>::get();;
    }

    // XXX doc
    constexpr Obj* object() const {
        return object_;
    }

    // XXX doc
    template<typename SlotRefT, std::enable_if_t<isSlotRef<SlotRefT>, int> = 0>
    ConnectionHandle connect(SlotRefT&& slotRef) const {
        return connect_(std::forward<SlotRefT>(slotRef));
    }

    // XXX doc
    template<typename FreeFunctionT, std::enable_if_t<isFreeFunction<FreeFunctionT>, int> = 0>
    ConnectionHandle connect(FreeFunctionT&& callback) const {
        return connect_(std::forward<FreeFunctionT>(callback));
    }

    // XXX doc
    template<typename Functor, std::enable_if_t<isFunctor<Functor>, int> = 0>
    ConnectionHandle connect(Functor&& funcObj) const {
        SignalTransmitter transmitter = SignalTransmitter::build<ArgsTuple>(std::forward<Functor>(funcObj));
        return SignalHub::connect(object_, id(), std::move(transmitter), std::monostate{});
    }

    // XXX doc
    bool disconnect() const {
        return SignalHub::disconnect(object_, id());
    }

    // XXX doc
    bool disconnect(ConnectionHandle h) const {
        return SignalHub::disconnect(object_, id(), h);
    }

    // XXX add disconnects in object

    // XXX doc
    template<typename SlotRefT, std::enable_if_t<isSlotRef<SlotRefT>, int> = 0>
    bool disconnect(SlotRefT&& slotRef) const {
        return disconnect_(std::forward<SlotRefT>(slotRef));
    }

    // XXX doc
    template<typename FreeFunctionT, std::enable_if_t<isFreeFunction<FreeFunctionT>, int> = 0>
    bool disconnect(FreeFunctionT&& callback) const {
        return disconnect_(std::forward<FreeFunctionT>(callback));
    }

protected:
    template<typename TagT, typename SlotMethodT>
    ConnectionHandle connect_(SlotRef<TagT, SlotMethodT>&& slotRef) const {
        // XXX make owner listen on receiver destroy to automatically disconnect signals
        static_assert(isMethod<SlotMethodT>);
        SignalTransmitter transmitter = SignalTransmitter::build<ArgsTuple>(slotRef.method(), slotRef.object());
        return SignalHub::connect(object_, id(), std::move(transmitter), BoundObjectMethodId(slotRef.object(), slotRef.id()));
    }

    template<typename R, typename... FnArgs>
    ConnectionHandle connect_(R (*callback)(FnArgs...)) const {
        SignalTransmitter transmitter = SignalTransmitter::build<ArgsTuple>(callback);
        return SignalHub::connect(object_, id(), std::move(transmitter), FreeFuncId(callback));
    }

    template<typename TagT, typename SlotMethodT>
    bool disconnect_(SlotRef<TagT, SlotMethodT>&& slotRef) const {
        return SignalHub::disconnect(object_, id(), BoundObjectMethodId(slotRef.object(), slotRef.id()));
    }

    template<typename R, typename... FnArgs>
    bool disconnect_(R (*callback)(FnArgs...)) const {
        return SignalHub::disconnect(object_, id(), FreeFuncId(callback));
    }

    void emit_(Args&&... args) const {
        ::vgc::core::internal::SignalHub::emit_<ArgsTuple>(
            object(), id(),
            std::forward<Args>(args)...);
    }

private:
    Obj* object_;
};

template<typename T>
inline constexpr bool isSignalRef = IsTplBaseOf<SignalRef, T>;

} // namespace internal

} // namespace vgc::core


#define VGC_SIG_PTYPE2_(t, n) t
#define VGC_SIG_PNAME2_(t, n) n
#define VGC_SIG_PBOTH2_(t, n) t n
#define VGC_SIG_PTYPE_(x) VGC_SIG_PTYPE2_ x
#define VGC_SIG_PNAME_(x) VGC_SIG_PNAME2_ x
#define VGC_SIG_PBOTH_(x) VGC_SIG_PBOTH2_ x
#define VGC_SIG_FWD2_(t, n) std::forward<t>(n)
#define VGC_SIG_FWD_(x) VGC_SIG_FWD2_ x

// ... must end with VaEnd
#define VGC_PARAMS_TYPE_(...) VGC_PP_TRIM_VAEND(VGC_PP_TRANSFORM(VGC_SIG_PTYPE_, __VA_ARGS__))
// ... must end with VaEnd
#define VGC_PARAMS_NAME_(...) VGC_PP_TRIM_VAEND(VGC_PP_TRANSFORM(VGC_SIG_PNAME_, __VA_ARGS__))
// ... must end with VaEnd
#define VGC_PARAMS_(...) VGC_PP_TRIM_VAEND(VGC_PP_TRANSFORM(VGC_SIG_PBOTH_, __VA_ARGS__))
// ... must end with VaEnd
#define VGC_PARAMS_FWD_(...) VGC_PP_TRIM_VAEND(VGC_PP_TRANSFORM(VGC_SIG_FWD_, __VA_ARGS__))



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
    auto name_() const {                                                                                \
        struct Tag {};                                                                                  \
        using MyClass = std::remove_const_t<std::remove_pointer_t<decltype(this)>>;                     \
        using SignalRefT = ::vgc::core::internal::SignalRef<                                            \
            Tag, VGC_PARAMS_TYPE_((MyClass,), __VA_ARGS__)>;                                            \
        class VGC_PP_EXPAND(VGC_NODISCARD("Did you intend to call " #name_ "().emit(..)?"))             \
        SignalRef : public SignalRefT {                                                                 \
        public:                                                                                         \
            SignalRef(const Obj* object) : SignalRefT(object) {}                                        \
            void emit(VGC_PARAMS_(__VA_ARGS__)) const {                                                 \
                emit_(VGC_PARAMS_FWD_(__VA_ARGS__));                                                    \
            }                                                                                           \
        };                                                                                              \
        return SignalRef(this);                                                                         \
    }

namespace vgc::core::internal {

template<typename T>
struct isSignal_ : std::false_type {};
template<typename ObjT, typename SignalRefT>
struct isSignal_<SignalRefT (ObjT::*)() const> : std::bool_constant<isSignalRef<SignalRefT> && isObject<ObjT>> {};
template<typename T>
inline constexpr bool isSignal = isSignal_<std::remove_reference_t<T>>::value;

} // namespace vgc::core::internal


#define VGC_SLOT(name_)                                                                                 \
    auto name_##Slot() {                                                                                \
        struct Tag {};                                                                                  \
        using MyClass = std::remove_pointer_t<decltype(this)>;                                          \
        using SlotMethod = decltype(&MyClass::name_);                                                   \
        using SlotRefT = ::vgc::core::internal::SlotRef<Tag, SlotMethod>;                               \
        class VGC_PP_EXPAND(VGC_NODISCARD("Did you intend "                                             \
            VGC_PP_STR(name_##Slot) "() instead of " #name_ "()?"))                                     \
        SlotRef : public SlotRefT {                                                                     \
        public:                                                                                         \
            SlotRef(const MyClass* object, SlotMethod m) : SlotRefT(object, m) {}                       \
        };                                                                                              \
        return SlotRef(this, &MyClass::name_);                                                          \
    }

#define VGC_SLOT_ALIAS(name_, funcName_)                                                                \
    auto name_() {                                                                                      \
        struct Tag {};                                                                                  \
        using MyClass = std::remove_pointer_t<decltype(this)>;                                          \
        using SlotMethod = decltype(&MyClass::funcName_);                                               \
        using SlotRefT = ::vgc::core::internal::SlotRef<Tag, SlotMethod>;                               \
        class VGC_PP_EXPAND(VGC_NODISCARD("Did you intend " #funcName_ "() instead of " #name_ "()?"))  \
        SlotRef : public SlotRefT {                                                                     \
        public:                                                                                         \
            SlotRef(const MyClass* object, SlotMethod m) : SlotRefT(object, m) {}                       \
        };                                                                                              \
        return SlotRef(this, &MyClass::funcName_);                                                      \
    }


namespace vgc::core::internal {

template<typename T>
struct isSlot_ : std::false_type {};
template<typename ObjT, typename SlotRefT>
struct isSlot_<SlotRefT(ObjT::*)()> : std::bool_constant<isSlotRef<SlotRefT> && isObject<ObjT>> {};
template<typename T>
inline constexpr bool isSlot = isSlot_<std::remove_reference_t<T>>::value;

} // namespace vgc::core::internal


namespace vgc::core {

// XXX move namespace imports in object.h

using ConnectionHandle = internal::ConnectionHandle;

} // namespace vgc::core


////////////////////////
// DEPRECATED
////////////////////////


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
            //SlotId,
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

    /*void removeListener_(const Object* o, const StringId& slotName) const {
        removeListenerIf_([=](const Listener_& l) {
            return std::holds_alternative<BoundObjectMethodId>(l.id) &&
                std::get<BoundObjectMethodId>(l.id) == BoundObjectMethodId(o, slotName);
            });
    }*/

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

    /*ConnectionHandle addListener_(FnType fn, Object* o, StringId slotName) const {
        auto h = genConnectionHandle();
        listeners_.append({fn, h, SlotId{o, slotName}});
        return h;
    }*/

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
        using HTraits = CallableTraits<Handler>;
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


#endif // VGC_CORE_INTERNAL_SIGNAL_H
