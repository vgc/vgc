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

#ifndef VGC_CORE_TEMPLATEUTIL_H
#define VGC_CORE_TEMPLATEUTIL_H

#include <functional>
#include <tuple>
#include <type_traits>

namespace vgc::core {

namespace internal {

// Used to inline sfinae-based tests on type ArgType.
// see VGC_CONSTEXPR_IS_ID_ADDRESSABLE_IN_CLASS_ for an example.
//
template<typename ArgType>
struct LambdaSfinae {
    static constexpr bool check(...) { return false; }
    template <class Lambda>
    static constexpr auto check(Lambda lambda)
        -> decltype(lambda(std::declval<ArgType>()), bool{}) {
        return true;
    }
};

} // namespace internal

// compile-time evaluates to true only if &cls::id is a valid expression.
#define VGC_CONSTEXPR_IS_ID_ADDRESSABLE_IN_CLASS(cls, id)                           \
    ::vgc::core::internal::LambdaSfinae<cls*>::check(                               \
        [](auto* v)                                                                 \
            -> std::void_t<decltype(&std::remove_pointer_t<decltype(v)>::id)> {     \
        })

#define VGC_CONSTEXPR_IS_TYPE_DECLARED_IN_CLASS(cls, tname)                         \
    ::vgc::core::internal::LambdaSfinae<cls*>::check(                               \
        [](auto* v)                                                                 \
            -> std::void_t<typename std::remove_pointer_t<decltype(v)>::tname> {    \
        })

namespace internal {

template<typename U>
struct TypeIdentity_ {
    using type = U;
};

} // namespace internal

// Similar as std::type_identity available in C++20.
// Used to establish non-deduced contexts in template argument deduction.
// e.g. throwLengthError's IntType is deduced from first argument only,
// then second argument must be convertible to it.
// todo: move it to some common header or adopt c++20.
template<typename U>
using TypeIdentity = typename internal::TypeIdentity_<U>::type;

template<typename T>
using RemoveCRef = std::remove_const_t<std::remove_reference_t<T>>;

namespace internal {

template<std::size_t I, typename... T, std::size_t... Is>
constexpr std::tuple<std::tuple_element_t<I + Is, std::tuple<T...>>...>
SubPackAsTuple_(std::index_sequence<Is...>);

} // namespace internal

template<size_t I, size_t N, typename... T>
using SubPackAsTuple = decltype(internal::SubPackAsTuple_<I, T...>(std::make_index_sequence<N>{}));

namespace internal {

template<std::size_t I, typename Tuple, std::size_t... Is>
constexpr std::tuple<std::tuple_element_t<I + Is, Tuple>...>
SubTuple_(std::index_sequence<Is...>);

} // namespace internal

template<size_t I, size_t N, typename Tuple>
using SubTuple = decltype(internal::SubTuple_<I, Tuple>(std::make_index_sequence<N>{}));

namespace internal {

template<class F, class ArgsTuple, std::size_t... I>
void applyPartial_(F&& f, ArgsTuple&& t, std::index_sequence<I...>) {
    std::invoke(
        std::forward<F>(f),
        std::get<I>(std::forward<ArgsTuple>(t))...);
}

} // namespace internal

template<size_t N, class F, class ArgsTuple>
void applyPartial(F&& f, ArgsTuple&& t) {
    internal::applyPartial_(
        std::forward<F>(f),
        std::forward<ArgsTuple>(t),
        std::make_index_sequence<N>{});
}

template<typename T, typename Traits_>
struct WithTraits : T {
    using T::T;
    using UnderlyingType = T;
    using Traits = Traits_;
};

// Call Sig Traits

// CallSig of must be of the form R(Args...)
template<typename CallSig>
struct CallSigTraits;

template<typename R, typename... Args>
struct CallSigTraits<R(Args...)> {
    using CallSig = R(Args...);
    using ReturnType = R;
    using ArgsTuple = std::tuple<Args...>;
    static constexpr size_t arity = sizeof...(Args);
};

// Method Traits (non-rvalue-reference-qualified)

namespace internal {

template<typename T>
struct MethodTraits_;

template<typename Obj_, typename This_, bool IsConst, typename R, typename... Args>
struct MethodTraitsDef_ : CallSigTraits<R(Args...)> {
    using Obj = Obj_;
    using This = This_;
    static constexpr bool isConst = IsConst;
};

template<typename R, typename C, typename... Args> struct MethodTraits_<R (C::*)(Args...)         > :
    MethodTraitsDef_<C, C*, false, R, Args...> {};                                

template<typename R, typename C, typename... Args> struct MethodTraits_<R (C::*)(Args...) &       > :
    MethodTraitsDef_<C, C*, false, R, Args...> {};                              

template<typename R, typename C, typename... Args> struct MethodTraits_<R (C::*)(Args...) const   > :
    MethodTraitsDef_<C, const C*, true, R, Args...> {};                                 

template<typename R, typename C, typename... Args> struct MethodTraits_<R (C::*)(Args...) const&  > :
    MethodTraitsDef_<C, const C*, true, R, Args...> {};                               

} // namespace internal

template<typename T>
using MethodTraits = internal::MethodTraits_<RemoveCRef<T>>;

// Functor Traits

namespace internal {

template<typename T, typename Enable = void>
struct FunctorTraits_;

template<typename Obj_, typename R, typename... Args>
struct FunctorTraitsDef_ : CallSigTraits<R(Args...)> {
    using Obj = Obj_;
};

template<typename T>
struct FunctorTraits_<T, std::void_t<decltype(&T::operator())>> : 
    CallSigTraits<typename MethodTraits<decltype(&T::operator())>::CallSig> {

    using CallOpType = decltype(&T::operator());
    using CallOpTraits = MethodTraits<CallOpType>;
    using Obj = typename CallOpTraits::Obj;
};

} // namespace internal

template<typename T>
using FunctorTraits = internal::FunctorTraits_<RemoveCRef<T>>;

// Free-Function Traits

namespace internal {

template<typename T>
struct FreeFunctionTraits_;

template<typename R, typename... Args>
struct FreeFunctionTraitsDef_ : CallSigTraits<R(Args...)> {
    // nothing more atm..
};

template<typename R, typename... Args> struct FreeFunctionTraits_<R(*)(Args...)> :
    FreeFunctionTraitsDef_<R, Args...> {}; 

template<typename R, typename... Args> struct FreeFunctionTraits_<R   (Args...)> :
    FreeFunctionTraitsDef_<R, Args...> {};

} // namespace internal

template<typename T>
using FreeFunctionTraits = internal::FreeFunctionTraits_<RemoveCRef<T>>;

// Callable Traits

enum class CallableKind {
    FreeFunction,
    Method,
    Functor
};

namespace internal {

template<typename T, typename Enable = void>
struct CallableTraits_;

// Cannot use std::void_t here because it fails on MacOS.
// Probably a compiler bug similar to https://bugs.llvm.org/show_bug.cgi?id=46791

template<typename T> struct CallableTraits_<T, std::enable_if_t<FreeFunctionTraits_<T>::arity >= 0>> : FreeFunctionTraits<T> {
    static constexpr CallableKind kind = CallableKind::FreeFunction;
};

template<typename T> struct CallableTraits_<T, std::enable_if_t<MethodTraits_<T>::arity >= 0>> : MethodTraits<T> {
    static constexpr CallableKind kind = CallableKind::Method;
};

template<typename T> struct CallableTraits_<T, std::enable_if_t<FunctorTraits_<T>::arity >= 0>> : FunctorTraits<T> {
    static constexpr CallableKind kind = CallableKind::Functor;
};

} // namespace internal

template<typename T>
using CallableTraits = internal::CallableTraits_<RemoveCRef<T>>;

namespace internal {

template<typename T, typename Enable = void>
struct isFreeFunction_ : std::false_type {};
template<typename T>
struct isFreeFunction_<T, std::enable_if_t<CallableTraits_<T>::kind == CallableKind::FreeFunction, void>> : std::true_type {};

} // namespace internal

template<typename T>
inline constexpr bool isFreeFunction = internal::isFreeFunction_<T>::value;

namespace internal {

template<typename T, typename Enable = void>
struct isMethod_ : std::false_type {};
template<typename T>
struct isMethod_<T, std::enable_if_t<CallableTraits_<T>::kind == CallableKind::Method, void>> : std::true_type {};

} // namespace internal

// "Method" = non-rvalue-reference-qualified non-static member function.
// i.e.: For "struct Foo { void foo() &&; };", isMethod<&Foo::foo> is false.
//
template<typename T>
inline constexpr bool isMethod = internal::isMethod_<T>::value;

namespace internal {

template<typename T, typename Enable = void>
struct isFunctor_ : std::false_type {};
template<typename T>
struct isFunctor_<T, std::enable_if_t<CallableTraits_<T>::kind == CallableKind::Functor, void>> : std::true_type {};

} // namespace internal

template<typename T>
inline constexpr bool isFunctor = internal::isFunctor_<T>::value;

namespace internal {

template<typename T, typename Enable = void>
struct isCallable_ : std::false_type {};
template<typename T>
struct isCallable_<T, std::void_t<typename CallableTraits_<T>::CallSig>> : std::true_type {};

} // namespace internal

// Does not include pointers to data members.
template<typename T>
inline constexpr bool isCallable = internal::isCallable_<T>::value;


// XXX to allow slots for pairs of ref-qualified member functions via a select func:
// valid: decltype(getLValueRefQualifiedOverload_(&Foo::foo))
template<typename R, typename C, typename... Args>
auto getLValueRefQualifiedMethodOverload_(R(C::*x)(Args...)) { return x; }
template<typename R, typename C, typename... Args>
auto getLValueRefQualifiedMethodOverload_(R(C::*x)(Args...)&) { return x; }


namespace internal {

template<template<typename...> typename Base, typename... Ts>
std::true_type  testIsConvertibleToTplBasePointer(const volatile Base<Ts...>*);
template<template<typename...> typename Base>
std::false_type testIsConvertibleToTplBasePointer(const volatile void*);

template<template<typename...> typename Base, typename Derived>
auto testIsTplBaseOf(int) -> decltype(testIsConvertibleToTplBasePointer<Base>(static_cast<Derived*>(nullptr)));
template<template<typename...> typename Base, typename Derived>
auto testIsTplBaseOf(...) -> std::true_type; // inaccessible base

template<template<typename...> typename Base, typename Derived>
struct IsTplBaseOf_ : std::bool_constant<
    std::is_class_v<Derived> && decltype(testIsTplBaseOf<Base, Derived>(0))::value> {};

} // namespace internal

template<template<typename...> typename Base, typename Derived>
inline constexpr bool IsTplBaseOf = internal::IsTplBaseOf_<Base, Derived>::value;

} // namespace vgc::core

#endif // VGC_CORE_TEMPLATEUTIL_H
