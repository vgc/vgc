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

#include <windows.h>

#include <any>
#include <array>
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
/// ```cpp
/// void printInt(int i) { std::cout << i << std::endl; }
/// int main() {
///     vgc::core::Signal<int> s;
///     s.connect(&printInt);
///     s(42); // print 42
/// }
/// ```
///
/// Example 2:
///
/// ```cpp
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
/// ```
///

// XXX ORPHANED COMMENT
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

// Configuration
namespace vgc::core::internal {
static constexpr Int maxSignalArgs = 20;
} // namespace vgc::core::internal


namespace vgc::core {

class Object;

namespace internal {

template<typename T, typename Enable = bool>
struct HasObjectTypedefs : std::false_type {};
template<typename T>
struct HasObjectTypedefs<T, RequiresValid<typename T::ThisClass, typename T::SuperClass>> :
    std::is_same<std::remove_const_t<T>, typename T::ThisClass> {};

} // namespace internal

// If T is an Object, it also checks that VGC_OBJECT has been used in T.
// 
template<typename T, typename Enable = bool>
struct IsObject : std::false_type {};
template<>
struct IsObject<Object, bool> : std::true_type {};
template<typename T>
struct IsObject<T, Requires<std::is_base_of_v<Object, T>>> : std::true_type {
    static_assert(internal::HasObjectTypedefs<T>::value,
        "Missing VGC_OBJECT(..) in T.");
};

template <typename T>
inline constexpr bool isObject = IsObject<T>::value;

// Checks that Type inherits from Object.
// Also checks that VGC_OBJECT(..) has been used in Type.
#define CHECK_TYPE_IS_VGC_OBJECT(Type) \
    static_assert(::vgc::core::internal::isObject<Type>, #Type " should inherit from vgc::core::Object.");


namespace internal {

using FunctionId = Int;
using ObjectSlotId = std::pair<Object*, FunctionId>;
using FreeFuncId = void*;
using SlotId = std::variant<std::monostate, ObjectSlotId, FreeFuncId>;
using SignalId = FunctionId;

VGC_CORE_API FunctionId genFunctionId();

/// A handle to a Signal/Slot connection.
/// It is returned by Signal's connect functions and can be used in disconnect functions.
class ConnectionHandle {
public:
    static const ConnectionHandle invalid;

protected:
    constexpr ConnectionHandle(Int64 id)
        : id_(id) {}

public:
    static VGC_CORE_API ConnectionHandle generate();

    friend bool operator==(const ConnectionHandle& h1, const ConnectionHandle& h2) {
        return h1.id_ == h2.id_;
    }

    friend bool operator!=(const ConnectionHandle& h1, const ConnectionHandle& h2) {
        return h1.id_ == h2.id_;
    }

private:
    Int64 id_;
};

inline constexpr ConnectionHandle ConnectionHandle::invalid = {-1};


template<typename Tag>
class FunctionIdSingleton {
public:
    static FunctionId get() {
        static FunctionId s = genFunctionId();
        return s;
    }

private:
    FunctionIdSingleton() = delete;
};


// Helper to check that signal/slot arguments don't contain
// rvalue reference types (that are not allowed).
template<typename TupleT> struct HasRValueReferences;
template<typename... Ts>  struct HasRValueReferences<std::tuple<Ts...>> :
    std::disjunction<std::is_rvalue_reference<Ts>...> {};

template<typename TupleT>
static constexpr bool hasRValueReferences = HasRValueReferences<TupleT>::value;


// Arguments of signals are meant to be forwarded to multiple slots.
// Thus, perfect forwarding is not an option.
// However, since we don't allow rvalues as signal/slot arguments, we can
// forward arguments by reference or const reference !
// 
// SignalArg    | MakeSignalArgRef<SignalArg>
// -------------|-----------------------------
//       T      | const T&
//       T&     |       T&
// const T&     | const T&
//       T&&    | not defined
// const T&&    | not defined
//
template<typename SignalArg>
using MakeSignalArgRef = std::enable_if_t<
    !std::is_rvalue_reference_v<SignalArg>,
    std::conditional_t<
        std::is_reference_v<SignalArg>,
        std::remove_reference_t<SignalArg>&,
        const SignalArg&>>;

template<typename T>
struct IsSignalArgRef : std::is_lvalue_reference<T> {};

template<typename T>
static constexpr bool isSignalArgRef = IsSignalArgRef<T>::value;


template<typename SignalArgsTuple>
struct MakeSignalArgRefsTuple_;
template<typename... SignalArgs>
struct MakeSignalArgRefsTuple_<std::tuple<SignalArgs...>> {
    using type = std::tuple<MakeSignalArgRef<SignalArgs>...>;
};

template<typename SignalArgsTuple>
using MakeSignalArgRefsTuple = typename MakeSignalArgRefsTuple_<SignalArgsTuple>::type;

template<typename T>
struct IsSignalArgRefsTuple : std::false_type {};
template<typename... Ts>
struct IsSignalArgRefsTuple<std::tuple<Ts...>> :
    std::conjunction<IsSignalArgRef<Ts>...> {};

template<typename T>
static constexpr bool isSignalArgRefsTuple = IsSignalArgRefsTuple<T>::value;


// Container for any signal argument reference type.
// It allows for a type-agnostic transmit interface based on lambdas that
// is necessary to implement wraps.
// Unlike std::any, values are either copied in or referred to, and type-checks
// are done only in debug builds.
//
// It cannot be constructed from an rvalue.
//
class AnySignalArgRef {
    static constexpr size_t storageSize = sizeof(void*);

    // To keep CVR as part of the type.
    template<typename T>
    struct TypeSig {
        using type = T;
    };

protected:
    template<typename SignalArgRef>
    using ValueType = std::remove_reference_t<SignalArgRef>;

    template<typename SignalArgRef>
    using Pointer = ValueType<SignalArgRef>*;

public:
    AnySignalArgRef() = default;

    // Prevents making an AnySignalArgRef from a rvalue.
    template<typename SignalArgRef>
    static AnySignalArgRef make(ValueType<SignalArgRef>&& x) = delete;

    template<typename SignalArgRef>
    static AnySignalArgRef make(ValueType<SignalArgRef>& arg) {
        static_assert(isSignalArgRef<SignalArgRef>);

        AnySignalArgRef ret = {};
        if constexpr (isCopiedIn<SignalArgRef>()) {
            ::new(&ret.v_) ValueType<SignalArgRef>(arg);
        }
        else {
            ::new(&ret.v_) Pointer<SignalArgRef>(std::addressof(arg));
        }

#ifdef VGC_DEBUG
        ret.t_ = typeid(TypeSig<SignalArgRef>);
#endif
        return ret;
    }

    template<typename SignalArgRef>
    SignalArgRef get() const {
        static_assert(isSignalArgRef<SignalArgRef>);

#ifdef VGC_DEBUG
        if (t_ != typeid(TypeSig<SignalArgRef>)) {
            throw LogicError("Bad cast of AnySignalArgRef.");
        }
#endif
        Pointer<SignalArgRef> p = {};
        if constexpr (isCopiedIn<SignalArgRef>()) {
            p = std::launder(reinterpret_cast<Pointer<SignalArgRef>>(&v_));
        }
        else {
            p = *std::launder(reinterpret_cast<const Pointer<SignalArgRef>*>(&v_));
        }
        return static_cast<SignalArgRef>(*p);
    }

protected:
    template<typename SignalArgRef>
    static constexpr bool isCopiedIn() {
        using Value = ValueType<SignalArgRef>;
        return (sizeof(Value) <= storageSize) &&
            std::is_const_v<Value> &&
            std::is_trivially_copyable_v<Value>;
    }

private:
    mutable std::aligned_storage_t<storageSize, storageSize> v_;
#ifdef VGC_DEBUG
    std::type_index t_ = typeid(void);
#endif
};

// Used to forward signal arguments to slot wrappers in a single argument.
// Constructing from temporaries is not allowed.
//
template<size_t N>
class TransmitArgs_ {
public:
    static constexpr Int maxSize = N;

protected:
    TransmitArgs_() = delete;

    template<typename... SignalArgRefs, typename... Us>
    TransmitArgs_(std::tuple<SignalArgRefs...>*, Us&&... args) :
        refs_{AnySignalArgRef::make<SignalArgRefs>(std::forward<Us>(args))...},
        size_(sizeof...(SignalArgRefs)) {}

public:
    // Does not accept rvalues.
    template<typename SignalArgRefsTuple, typename... Us>
    static TransmitArgs_ make(Us&&... args) {
        static_assert(isSignalArgRefsTuple<SignalArgRefsTuple>);
        static_assert(std::tuple_size_v<SignalArgRefsTuple> == sizeof...(Us));
        return TransmitArgs_(static_cast<SignalArgRefsTuple*>(nullptr), std::forward<Us>(args)...);
    }

    // Throws IndexError in debug builds if i not in [0, maxSize).
    template<typename SignalArgRef>
    SignalArgRef get(Int i) const {
#ifdef VGC_DEBUG
        try {
            return refs_.at(i).template get<SignalArgRef>();
        }
        catch (std::out_of_range& e) {
            throw IndexError(e.what());
        }
#else

        return refs_[i].template get<SignalArgRef>();
#endif
    }

private:
    std::array<AnySignalArgRef, maxSize> refs_;
    Int size_;
};

using TransmitArgs = TransmitArgs_<maxSignalArgs>;


class SignalTransmitter {
public:
    using SlotWrapperSig = void(const TransmitArgs&);
    using SlotWrapper = std::function<SlotWrapperSig>;

    SignalTransmitter() = default;

    template<typename SlotWrapperT, Requires<isFunctor<RemoveCVRef<SlotWrapperT>>> = true>
    SignalTransmitter(SlotWrapperT&& wrapper, bool isNative = false) :
        wrapper_(std::forward<SlotWrapperT>(wrapper)),
        arity_(FunctorTraits<RemoveCVRef<SlotWrapperT>>::arity),
        isNative_(isNative) {}

    template<typename SignalArgRefsTuple, typename SlotCallable, typename... OptionalObj,
        Requires<isCallable<RemoveCVRef<SlotCallable>>> = true>
    [[nodiscard]] static SignalTransmitter build(SlotCallable&& c, OptionalObj... methodObj) {
        static_assert(isSignalArgRefsTuple<SignalArgRefsTuple>);

        using Traits = CallableTraits<RemoveCVRef<SlotCallable>>;

        static_assert(std::tuple_size_v<SignalArgRefsTuple> >= Traits::arity,
            "The slot signature cannot be longer than the signal signature.");
        static_assert(std::tuple_size_v<SignalArgRefsTuple> < maxSignalArgs,
            "Signals and Slots are limited to maxSignalArgs parameters.");

        if constexpr (isMethod<RemoveCVRef<SlotCallable>>) {
            static_assert(sizeof...(OptionalObj) == 1,
                "Expecting one methodObj to bind the given method.");

            using OptionalObj0 = std::tuple_element_t<0, std::tuple<OptionalObj&&...>>;
            using Obj = std::remove_reference_t<OptionalObj0>;
            static_assert(std::is_pointer_v<Obj>,
                "Requires methodObj to be passed by pointer.");
            static_assert(std::is_convertible_v<Obj, typename Traits::This>,
                "The given methodObj cannot be used to bind the given method.");
        }
        else {
            static_assert(sizeof...(OptionalObj) == 0,
                "Expecting methodObj only for methods, the given callable is not a method.");
        }

        using TruncatedSignalArgRefsTuple = SubTuple<0, Traits::arity, SignalArgRefsTuple>;

        auto fn = buildSlotWrapper<TruncatedSignalArgRefsTuple>(
            std::forward<SlotCallable>(c),
            std::make_index_sequence<Traits::arity>{},
            methodObj...);

        return SignalTransmitter(std::move(fn), true);
    }

    void transmit(const TransmitArgs& args) const {
        wrapper_(args);
    }

    UInt8 slotArity() const {
        return arity_;
    }

    bool isNative() const {
        return isNative_;
    }

    const SlotWrapper& getSlotWrapper() const {
        return wrapper_;
    }

protected:
    template<typename TruncatedSignalArgRefsTuple, typename SlotCallable, size_t... Is, typename... OptionalObj>
    [[nodiscard]] static auto buildSlotWrapper(SlotCallable&& c, std::index_sequence<Is...>, OptionalObj&&... methodObj) {
        static_assert(
            std::is_invocable_v<
                SlotCallable&&, OptionalObj&&...,
                std::tuple_element_t<Is, TruncatedSignalArgRefsTuple>...>,
            "The slot signature is not compatible with the signal.");

        return SlotWrapper(
            [=](const TransmitArgs& args) {
                std::invoke(c, methodObj..., args.get<std::tuple_element_t<Is, TruncatedSignalArgRefsTuple>>(Is)...);
            });
    }

private:
    SlotWrapper wrapper_;
    // Arity of the wrapped slot.
    UInt8 arity_ = 0;
    // true if made with build()
    bool isNative_ = false; 
    // todo: More reflection info..
};


template<typename TTuple, typename UTuple, typename Enable = bool>
struct IsTupleConvertible : std::false_type {};
template<typename... Ts, typename... Us>
struct IsTupleConvertible<
    std::tuple<Ts...>, std::tuple<Us...>,
    Requires<sizeof...(Ts) == sizeof...(Us)>> :
    std::conjunction<std::is_convertible<Ts, Us>...> {};

template<typename SignalArgRefsTuple, typename SignalSlotArgRefsTuple, typename Enable = bool>
struct IsCompatibleReEmit : std::false_type {};
template<typename... SignalArgRefs, typename... SignalSlotArgRefs>
struct IsCompatibleReEmit<
    std::tuple<SignalArgRefs...>, std::tuple<SignalSlotArgRefs...>,
    Requires<sizeof...(SignalArgRefs) >= sizeof...(SignalSlotArgRefs)>> :
    IsTupleConvertible<
        SubTuple<0, sizeof...(SignalSlotArgRefs), std::tuple<SignalArgRefs...>>,
        std::tuple<SignalSlotArgRefs...>> {};

template<typename SignalArgRefsTuple, typename SignalSlotArgRefsTuple>
inline constexpr bool isCompatibleReEmit = IsCompatibleReEmit<SignalArgRefsTuple, SignalSlotArgRefsTuple>::value;


// XXX doc
// Its member functions are static because we have to operate from the context of its owner Object.
class SignalHub {
private:
    struct Connection_ {
        template<typename To>
        Connection_(SignalTransmitter&& transmitter, ConnectionHandle h, SignalId from, To&& to) :
            transmitter(std::move(transmitter)), h(h), from(from), to(std::forward<To>(to)) {}

        SignalTransmitter transmitter;
        ConnectionHandle h;
        SignalId from;
        SlotId to;

        // XXX bool disabled ?
    };

    struct ListenedObjectInfo_ {
        const Object* object = nullptr;
        Int numInboundConnections = 0;
    };

    SignalHub(const SignalHub&) = delete;
    SignalHub& operator=(const SignalHub&) = delete;

public:
    SignalHub() = default;

#ifdef VGC_DEBUG
    ~SignalHub() noexcept(false) {
        if (!listenedObjectInfos_.empty()) {
            throw LogicError(
                "A SignalHub is being destroyed but is still subscribed to some Object's signals."
                "Object destruction should call disconnectSlots() explicitly."
            );
        }
        if (!connections_.empty()) {
            throw LogicError(
                "A SignalHub is being destroyed but is still connected to some Object's slots."
                "Object destruction should call disconnectSignals() explicitly."
            );
        }
    }
#endif

    // Defined in object.h
    static constexpr SignalHub& access(const Object* o);

    // Must be called during receiver destruction.
    static void disconnectSlots(const Object* receiver) {
        auto& hub = access(receiver);
        for (ListenedObjectInfo_& info : hub.listenedObjectInfos_) {
            if (info.numInboundConnections > 0) {
                Int count = SignalHub::eraseConnections_(info.object, receiver);
#ifdef VGC_DEBUG
                if (count != info.numInboundConnections) {
                    throw LogicError("Erased connections count != info.numInboundConnections");
                }
#endif
                info.numInboundConnections = 0;
            }
        }
        hub.listenedObjectInfos_.clear();
    }

    // Must be called during sender destruction.
    static void disconnectSignals(const Object* sender) {
        auto& hub = access(sender);
        // We sort connections by receiver so that we can fully disconnect each
        // only once without a seen set.
        struct {
            bool operator()(const Connection_& a, const Connection_& b) const {
                if (std::holds_alternative<ObjectSlotId>(a.to)) {
                    if (std::holds_alternative<ObjectSlotId>(b.to)) {
                        return std::get<ObjectSlotId>(a.to).first < std::get<ObjectSlotId>(b.to).first;
                    }
                    return true;
                }
                return false;
            }
        } ByReceiver;
        std::sort(hub.connections_.begin(), hub.connections_.end(), ByReceiver);

        // Now let's reset the info about this object which is stored in receiver objects.
        Object* prevDisconnectedReceiver = nullptr;
        for (const auto& c : hub.connections_) {
            if (std::holds_alternative<ObjectSlotId>(c.to)) {
                const auto& osid = std::get<ObjectSlotId>(c.to);
                if (osid.first != prevDisconnectedReceiver) {
                    prevDisconnectedReceiver = osid.first;
                    auto& info = access(osid.first).getListenedObjectInfoRef_(sender);
                    info.numInboundConnections = 0;
                }
            }
        }
        hub.connections_.clear();
    }

    static ConnectionHandle connect(const Object* sender, SignalId from, SignalTransmitter&& transmitter, const SlotId& to) {
        auto& hub = access(sender);
        auto h = ConnectionHandle::generate();

        if (std::holds_alternative<ObjectSlotId>(to)) {
            // Increment numInboundConnections in the receiver's info about sender.
            const auto& bsid = std::get<ObjectSlotId>(to);
            auto& info = access(bsid.first).findOrCreateListenedObjectInfo_(sender);
            info.numInboundConnections++;
        }

        hub.connections_.emplaceLast(std::move(transmitter), h, from, to);
        return h;
    }

    static Int numOutboundConnections(const Object* sender) {
        return access(sender).connections_.size();
    }

    static bool disconnect(const Object* sender, ConnectionHandle h) {
        return disconnectIf_(sender, [=](const Connection_& c) {
            return c.h == h;
        });
    }

    static bool disconnect(const Object* sender, SignalId from) {
        return disconnectIf_(sender, [=](const Connection_& c) {
            return c.from == from;
        });
    }

    static bool disconnect(const Object* sender, SignalId from, ConnectionHandle h) {
        return disconnectIf_(sender, [=](const Connection_& c) {
            return c.from == from && c.h == h;
        });
    }

    static bool disconnect(const Object* sender, SignalId from, const SlotId& to) {
        return disconnectIf_(sender, [=](const Connection_& c) {
            return c.from == from && c.to == to;
        });
    }

    static bool disconnect(const Object* sender, SignalId from, Object* receiver) {
        return disconnectIf_(sender, [=](const Connection_& c) {
            return c.from == from &&
                std::holds_alternative<ObjectSlotId>(c.to) &&
                std::get<ObjectSlotId>(c.to).first == receiver;
        });
    }

    static bool disconnect(const Object* sender, const Object* receiver) {
        Int count = eraseConnections_(sender, receiver);
        auto& info = access(receiver).getListenedObjectInfoRef_(sender);
#ifdef VGC_DEBUG
        if (count != info.numInboundConnections) {
            throw LogicError("Erased connections count != info.numInboundConnections");
        }
#endif
        info.numInboundConnections = 0;
        return count > 0;
    }

    template<typename SignalArgRefsTuple, typename... Us>
    static void emitFwd(Object* sender, SignalId from, Us&&... args) {
        static_assert(sizeof...(Us) == std::tuple_size_v<SignalArgRefsTuple>);
        auto argsArray = TransmitArgs::make<SignalArgRefsTuple>(std::forward<Us>(args)...);
        emit_(sender, from, argsArray);
    }

    template<typename SignalArgRefsTuple, typename... Us>
    static void emitConvert(Object* sender, SignalId from, Us&&... args) {
        constexpr size_t signalArity = std::tuple_size_v<SignalArgRefsTuple>;
        static_assert(sizeof...(Us) == signalArity);
        emitConvertImpl_<SignalArgRefsTuple>(sender, from, std::make_index_sequence<signalArity>{}, std::forward<Us>(args)...);
    }

    template<typename SignalArgRefsTuple, typename SignalSlotArgRefsTuple>
    static void reEmit(Object* sender, SignalId from, const TransmitArgs& args) {
        constexpr size_t signalSlotArity = std::tuple_size_v<SignalSlotArgRefsTuple>;
        static_assert(signalSlotArity <= std::tuple_size_v<SignalArgRefsTuple>);
        using TruncatedSignalArgRefsTuple = SubTuple<0, signalSlotArity, SignalArgRefsTuple>;
        if constexpr (std::is_same_v<TruncatedSignalArgRefsTuple, SignalSlotArgRefsTuple>) {
            emit_(sender, from, args);
        }
        else {
            static_assert(isCompatibleReEmit<TruncatedSignalArgRefsTuple, SignalSlotArgRefsTuple>);
            reEmitConvert_<TruncatedSignalArgRefsTuple, SignalSlotArgRefsTuple>(
                sender, from, args, std::make_index_sequence<signalSlotArity>{});
        }
    }

protected:
    // Used in onDestroy(), receiver is about to be destroyed.
    // This does NOT update the numInboundConnections in the sender info stored in receiver.
    //
    static Int eraseConnections_(const Object* sender, const Object* receiver) {
        auto& hub = access(sender);
        auto it = std::remove_if(hub.connections_.begin(), hub.connections_.end(), [=](const Connection_& c){
                return std::holds_alternative<ObjectSlotId>(c.to) && std::get<ObjectSlotId>(c.to).first == receiver; 
            });
        Int count = std::distance(it, hub.connections_.end());
        hub.connections_.erase(it, hub.connections_.end());
        return count;
    }

    ListenedObjectInfo_& getListenedObjectInfoRef_(const Object* object) {
        for (ListenedObjectInfo_& x : listenedObjectInfos_) {
            if (x.object == object) {
                return x;
            }
        }
        throw LogicError("Info should be present.");
    }

    ListenedObjectInfo_& findOrCreateListenedObjectInfo_(const Object* object) {
        for (ListenedObjectInfo_& x : listenedObjectInfos_) {
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

    template <typename T = void>
    static void emit_(Object* sender, SignalId from, const TransmitArgs& args) {
        auto& hub = access(sender);
        for (auto& c : hub.connections_) {
            if (c.from == from) {
                if (std::holds_alternative<ObjectSlotId>(c.to)) {
                    OutputDebugStringA(vgc::core::format(
                        "emit_: (sender={}, sender->refCount={}, from={}, to={})\n",
                        (void*)sender, sender->refCount(), from,  (void*)std::get<ObjectSlotId>(c.to).first).c_str());
                }
                c.transmitter.transmit(args);
            }
        }
    }

    template<typename SignalArgRefsTuple, size_t... Is>
    static void emitConvertImpl_(Object* sender, SignalId from, std::index_sequence<Is...>, std::tuple_element_t<Is, SignalArgRefsTuple>... args) {
        emitFwd<SignalArgRefsTuple>(sender, from, args...);
    }

    template<typename TruncatedSignalArgRefsTuple, typename SignalSlotArgRefsTuple, size_t... Is>
    static void reEmitConvert_(Object* sender, SignalId from, const TransmitArgs& args, std::index_sequence<Is...>) {
        emitConvert<SignalSlotArgRefsTuple>(sender, from, args.get<std::tuple_element_t<Is, TruncatedSignalArgRefsTuple>>(Is)...);
    }

    template<typename SignalArgRefsTuple, typename SignalSlotArgRefsTuple>
    friend SignalTransmitter buildRetransmitter(const Object* receiver, SignalId to);

private:
    // Manipulating it should be done with knowledge of the auto-disconnect mechanism.
    Array<Connection_> connections_;
    // Used to auto-disconnect on destroy.
    Array<ListenedObjectInfo_> listenedObjectInfos_;

    // Returns true if any connection is removed.
    // This does update the numInboundConnections in the sender info stored in receiver.
    //
    template<typename Pred>
    static bool disconnectIf_(const Object* sender, Pred pred) {
        auto& hub = access(sender);
        auto end = hub.connections_.end();
        auto it = std::find_if(hub.connections_.begin(), end, pred);
        auto insertIt = it;
        while (it != end) {
            const Connection_& c = *it;
            if (std::holds_alternative<ObjectSlotId>(c.to)) {
                // Decrement numInboundConnections in the receiver's info about sender.
                const auto& bsid = std::get<ObjectSlotId>(c.to);
                auto& info = access(bsid.first).getListenedObjectInfoRef_(sender);
                info.numInboundConnections--;
            }
            while (++it != end && !pred(*it)) {
                *insertIt = std::move(*it);
                ++insertIt;
            }
        }
        if (insertIt != end) {
            hub.connections_.erase(insertIt, end);
            return true;
        }
        return false;
    }
};

template<typename SignalArgRefsTuple, typename SignalSlotArgRefsTuple>
SignalTransmitter buildRetransmitter(const Object* from, SignalId id) {
    constexpr size_t signalSlotArity = std::tuple_size_v<SignalSlotArgRefsTuple>;
    static_assert(signalSlotArity <= std::tuple_size_v<SignalArgRefsTuple>,
        "The signal-slot signature cannot be longer than the signal signature.");
    using TruncatedSignalArgRefsTuple = SubTuple<0, signalSlotArity, SignalArgRefsTuple>;
    static_assert(isCompatibleReEmit<TruncatedSignalArgRefsTuple, SignalSlotArgRefsTuple>,
        "The signal-slot signature is not compatible with the signal.");
    return SignalTransmitter(
        [=](const TransmitArgs& args) {
            SignalHub::reEmit<TruncatedSignalArgRefsTuple, SignalSlotArgRefsTuple>(const_cast<Object*>(from), id, args);
        }, true);
}


// It is intended to be locally subclassed in the slot (getter).
//
template<typename TObjectMethodTag, typename TSlotMethod>
class SlotRef {
public:
    using SlotMethod = TSlotMethod;
    using SlotMethodTraits = MethodTraits<SlotMethod>;
    using Obj = typename SlotMethodTraits::Obj;
    using ArgsTuple = typename SlotMethodTraits::ArgsTuple;
    using ArgRefsTuple = MakeSignalArgRefsTuple<ArgsTuple>;

    static_assert(isObject<Obj>, "Slots can only be declared in Objects");
    static_assert(!hasRValueReferences<ArgsTuple>,
        "Slot parameters are not allowed to have a rvalue reference type.");

    static constexpr Int arity = SlotMethodTraits::arity;

protected:
    SlotRef(const Obj* object, SlotMethod m) :
        object_(const_cast<Obj*>(object)), m_(m) {}

public:
    // Non-Copyable
    SlotRef(const SlotRef&) = delete;
    SlotRef& operator=(const SlotRef&) = delete;

    static FunctionId id() {
        return FunctionIdSingleton<TObjectMethodTag>::get();
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
struct IsSlotRef : IsTplBaseOf<SlotRef, T> {};
template<typename T>
inline constexpr bool isSlotRef = IsSlotRef<T>::value;


template<typename TObjectMethodTag, typename TObj, typename TArgRefsTuple>
class SignalRef;

template<typename T>
struct IsSignalRef : IsTplBaseOf<SignalRef, T> {};
template<typename T>
inline constexpr bool isSignalRef = IsSignalRef<T>::value;

// It does not define emit(..) and is intended to be locally subclassed in
// the signal (getter).
//
template<typename TObjectMethodTag, typename TObj, typename TArgsTuple>
class SignalRef {
public:
    using Tag = TObjectMethodTag;
    using Obj = TObj;
    using ArgsTuple = TArgsTuple;
    using ArgRefsTuple = MakeSignalArgRefsTuple<ArgsTuple>;

    static_assert(isObject<Obj>, "Signals can only be declared in Objects");
    static_assert(!hasRValueReferences<ArgsTuple>,
        "Signal parameters are not allowed to have a rvalue reference type.");

    static constexpr Int arity = std::tuple_size_v<ArgRefsTuple>;

protected:
    SignalRef(const Obj* object) :
        object_(const_cast<Obj*>(object)) {}

public:
    // Non-Copyable
    SignalRef(const SignalRef&) = delete;
    SignalRef& operator=(const SignalRef&) = delete;

    // Returns a unique identifier that represents this signal.
    static SignalId id() {
        return FunctionIdSingleton<TObjectMethodTag>::get();
    }

    // Returns a pointer to the object bound to this signal.
    constexpr Obj* object() const {
        return object_;
    }

    // Connects to a method slot.
    template<typename SlotRefT, Requires<isSlotRef<SlotRefT>> = true>
    ConnectionHandle connect(const SlotRefT& slotRef) const {
        return connect_(slotRef);
    }

    // Connects to a signal-slot.
    template<typename SignalRefT, Requires<isSignalRef<SignalRefT>> = true>
    ConnectionHandle connect(const SignalRefT& signalRef) const {
        using SignalSlotArgRefsTuple = typename RemoveCVRef<SignalRefT>::ArgRefsTuple;
        constexpr size_t signalSlotArity = std::tuple_size_v<SignalSlotArgRefsTuple>;
        static_assert(signalSlotArity <= arity,
            "The signal-slot signature cannot be longer than the signal signature.");
        using TruncatedSignalArgRefsTuple = SubTuple<0, arity, ArgRefsTuple>;
        static_assert(isCompatibleReEmit<TruncatedSignalArgRefsTuple, ArgRefsTuple>,
            "The signal-slot signature is not compatible with the signal.");
        return connect_(signalRef);
    }

    // Connects to a free function.
    template<typename FreeFunction, Requires<isFreeFunction<RemoveCVRef<FreeFunction>>> = true>
    ConnectionHandle connect(FreeFunction&& callback) const {
        static_assert(!hasRValueReferences<typename FreeFunctionTraits<RemoveCVRef<FreeFunction>>::ArgsTuple>,
            "Slot parameters are not allowed to have a rvalue reference type.");
        return connect_(std::forward<FreeFunction>(callback));
    }

    // Connects to a functor.
    // Can only be disconnected using the returned handle.
    //
    template<typename Functor, Requires<isFunctor<RemoveCVRef<Functor>>> = true>
    ConnectionHandle connect(Functor&& funcObj) const {
        static_assert(!hasRValueReferences<typename FunctorTraits<RemoveCVRef<Functor>>::ArgsTuple>,
            "Slot parameters are not allowed to have a rvalue reference type.");
        SignalTransmitter transmitter = SignalTransmitter::build<ArgRefsTuple>(std::forward<Functor>(funcObj));
        return SignalHub::connect(object_, id(), std::move(transmitter), std::monostate{});
    }

    // Disconnects all slots (method and non-method).
    // Returns true if a disconnection happened.
    //
    bool disconnect() const {
        return SignalHub::disconnect(object_, id());
    }

    // Disconnects the slot identified by the given handle `h`.
    // Returns true if a disconnection happened.
    //
    bool disconnect(ConnectionHandle h) const {
        return SignalHub::disconnect(object_, id(), h);
    }

    // Disconnects the given slot `slotRef`.
    // Returns true if a disconnection happened.
    //
    template<typename SlotRefT, Requires<isSlotRef<RemoveCVRef<SlotRefT>>> = true>
    bool disconnect(SlotRefT&& slotRef) const {
        return SignalHub::disconnect(object_, id(), ObjectSlotId(slotRef.object(), slotRef.id()));
    }

    // Disconnects the given signal-slot `signalRef`.
    // Returns true if a disconnection happened.
    //
    template<typename SignalRefT, Requires<isSignalRef<RemoveCVRef<SignalRefT>>> = true>
    bool disconnect(SignalRefT&& signalRef) const {
        return SignalHub::disconnect(object_, id(), ObjectSlotId(signalRef.object(), signalRef.id()));
    }

    // Disconnects the given free function.
    // Returns true if a disconnection happened.
    //
    template<typename FreeFunction, Requires<isFreeFunction<RemoveCVRef<FreeFunction>>> = true>
    bool disconnect(FreeFunction&& callback) const {
        return disconnect_(std::forward<FreeFunction>(callback));
    }

    // Disconnects all method slots which are bound to the given `receiver`.
    // Returns true if a disconnection happened.
    //
    bool disconnect(Object* receiver) const {
        return SignalHub::disconnect(object_, id(), receiver);
    }

protected:
    template<typename TagT, typename SlotMethod>
    ConnectionHandle connect_(const SlotRef<TagT, SlotMethod>& slotRef) const {
        static_assert(isMethod<SlotMethod>);
        SignalTransmitter transmitter = SignalTransmitter::build<ArgRefsTuple>(slotRef.method(), slotRef.object());
        return SignalHub::connect(object_, id(), std::move(transmitter), ObjectSlotId(slotRef.object(), slotRef.id()));
    }

    // XXX move in connect directly .. no need for SignalRef params.
    template<typename TagT, typename ReceiverT, typename ArgTupleT>
    ConnectionHandle connect_(const SignalRef<TagT, ReceiverT, ArgTupleT>& signalSlotRef) const {
        using SlotArgRefsTuple = typename SignalRef<TagT, ReceiverT, ArgTupleT>::ArgRefsTuple;
        SignalTransmitter transmitter = buildRetransmitter<ArgRefsTuple, SlotArgRefsTuple>(signalSlotRef.object(), signalSlotRef.id());
        return SignalHub::connect(object_, id(), std::move(transmitter), ObjectSlotId(signalSlotRef.object(), signalSlotRef.id()));
    }

    template<typename R, typename... FnArgs>
    ConnectionHandle connect_(R (*callback)(FnArgs...)) const {
        SignalTransmitter transmitter = SignalTransmitter::build<ArgRefsTuple>(callback);
        return SignalHub::connect(object_, id(), std::move(transmitter), FreeFuncId(callback));
    }

    template<typename R, typename... FnArgs>
    bool disconnect_(R (*callback)(FnArgs...)) const {
        return SignalHub::disconnect(object_, id(), FreeFuncId(callback));
    }

    template<typename... Us>
    void emitFwd_(Us&&... args) const {
        static_assert(sizeof...(Us) == arity);
        ::vgc::core::internal::SignalHub::emitFwd<ArgRefsTuple>(object(), id(), std::forward<Us>(args)...);
    }

private:
    Obj* object_;
};

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

/// Macro to define VGC Object Signals.
///
/// Example:
/// 
/// ```cpp
/// struct A : vgc::core::Object {
///     VGC_SIGNAL(changed);
///     VGC_SIGNAL(changedThings, (type0, arg0), (type1, arg1));
/// };
/// ```
///
#define VGC_SIGNAL(...) VGC_PP_EXPAND(VGC_SIGNAL_(__VA_ARGS__, VaEnd))
#define VGC_SIGNAL_(name_, ...)                                                                         \
    auto name_() const {                                                                                \
        struct Tag {};                                                                                  \
        using MyClass = std::remove_const_t<std::remove_pointer_t<decltype(this)>>;                     \
        using ArgsTuple = std::tuple<VGC_PARAMS_TYPE_(__VA_ARGS__)>;                                    \
        using SignalRefT = ::vgc::core::internal::SignalRef<Tag, MyClass, ArgsTuple>;                   \
        class VGC_PP_EXPAND(VGC_NODISCARD("Did you intend to call " #name_ "().emit()?"))               \
        SignalRef : public SignalRefT {                                                                 \
        public:                                                                                         \
            SignalRef(const Obj* object) : SignalRefT(object) {}                                        \
            void emit(VGC_PARAMS_(__VA_ARGS__)) const {                                                 \
                emitFwd_(VGC_PARAMS_NAME_(__VA_ARGS__));                                                \
            }                                                                                           \
        };                                                                                              \
        return SignalRef(this);                                                                         \
    }

namespace vgc::core::internal {

template<typename T>
struct IsSignal : std::false_type {};
template<typename ObjT, typename SignalRefT>
struct IsSignal<SignalRefT (ObjT::*)() const> : std::conjunction<IsSignalRef<SignalRefT>, IsObject<ObjT>> {};
template<typename T>
inline constexpr bool isSignal = IsSignal<T>::value;

} // namespace vgc::core::internal

#define VGC_SLOT_ALIAS_(name_, funcName_)                                                               \
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

#define VGC_SLOT_DEFAULT_ALIAS_(name_) \
    VGC_PP_EXPAND(VGC_SLOT_ALIAS_(name_##Slot, name_))

#define VGC_SLOT2_(a, b) VGC_SLOT_ALIAS_(a, b)
#define VGC_SLOT1_(a) VGC_SLOT_DEFAULT_ALIAS_(a)
#define VGC_SLOT_DISPATCH_(_1, _2, _3, NAME, ...) NAME

/// Macro to define VGC Object Slots.
/// 
/// VGC_SLOT(slotName, slotMethod) -> defines slot slotName bound to slotMethod.
/// VGC_SLOT(slotMethod) -> defines slot slotMethodSlot bound to slotMethod.
/// 
/// Example:
/// 
/// ```cpp
/// struct A : vgc::core::Object {
///     void onFooChanged(int i);
/// 
///     VGC_SLOT(onFooChanged);                     // defines onFooChangedSlot()
///     VGC_SLOT(onBarChanged_);                    // defines onBarChanged_Slot()
///     VGC_SLOT(onBarChangedSlot, onBarChanged_);  // defines onBarChangedSlot()
/// 
/// private:
///     void onBarChanged_(double d);
/// };
/// ```
///
#define VGC_SLOT(...) VGC_PP_EXPAND(VGC_SLOT_DISPATCH_(__VA_ARGS__, NOOVERLOAD, VGC_SLOT2_, VGC_SLOT1_)(__VA_ARGS__))


namespace vgc::core::internal {

template<typename T>
struct IsSlot : std::false_type {};
template<typename ObjT, typename SlotRefT>
struct IsSlot<SlotRefT(ObjT::*)()> : std::conjunction<IsSlotRef<SlotRefT>, IsObject<ObjT>> {};
template<typename T>
inline constexpr bool isSlot = IsSlot<T>::value;

} // namespace vgc::core::internal


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
        removeListenerIf_(
            [=](const Listener_& l) {
                return l.h == h; 
            });
    }

    void removeListener_(void* freeFunc) const {
        removeListenerIf_(
            [=](const Listener_& l) {
                return std::holds_alternative<FreeFuncId>(l.id) && l.id == freeFunc;
            });
    }

    ConnectionHandle addListener_(FnType fn) const {
        auto h = ConnectionHandle::generate();
        listeners_.append({fn, h, std::monostate{}});
        return h;
    }

    ConnectionHandle addListener_(FnType fn, void* freeFunc) const {
        auto h = ConnectionHandle::generate();
        listeners_.append({fn, h, FreeFuncId{freeFunc}});
        return h;
    }

    // pointer-to-member-function
    template<typename Object, typename... SlotArgs>
    static inline FnType adaptHandler_(Object* o, void (Object::* mfn)(SlotArgs...)) {
        return [=](Args... args) {
            auto&& argsTuple = std::forward_as_tuple(o, std::forward<Args>(args)...);
            applyPartial<1 + sizeof...(SlotArgs)>(mfn, argsTuple);
        };
    }

    // free functions and callables
    template<typename Handler>
    static inline FnType adaptHandler_(Handler&& f) {
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
