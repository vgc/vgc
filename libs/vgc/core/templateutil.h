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
using RemoveCVRef = std::remove_cv_t<std::remove_reference_t<T>>;

// Helper to create SFINAE-based template overloads/specializations.
// 
// ```
// template<typename Union, Requires<std::is_union_v<Union>> = true>
// void foo(Union p) { ... }
// ```
//
template<bool B>
using Requires = std::enable_if_t<B, bool>;

namespace internal {

template<typename... Ts>
struct MakeBool { 
    using type = bool;
};

} // namespace internal

// Helper to create SFINAE-based template overloads/specializations.
// It maps a sequence of types Ts to bool.
//
template<typename... Ts>
using RequiresValid = typename internal::MakeBool<Ts...>::type;

// Alias of std::enable_if_t
template<bool B, typename T = void>
using EnableIf = std::enable_if_t<B, T>;

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

template<class F, class ArgsTuple, std::size_t... Is>
void applyPartial_(F&& f, ArgsTuple&& t, std::index_sequence<Is...>) {
    std::invoke(
        std::forward<F>(f),
        std::get<Is>(std::forward<ArgsTuple>(t))...);
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

template<typename TObj, typename TThis, bool IsConst, typename R, typename... Args>
struct MethodTraitsDef_ : CallSigTraits<R(Args...)> {
    using Obj = TObj;
    using This = TThis;
    static constexpr bool isConst = IsConst;
};

} // namespace internal

template<typename T>
struct MethodTraits;

template<typename R, typename C, typename... Args> struct MethodTraits<R (C::*)(Args...)         > :
    internal::MethodTraitsDef_<C, C*, false, R, Args...> {};                                

template<typename R, typename C, typename... Args> struct MethodTraits<R (C::*)(Args...) &       > :
    internal::MethodTraitsDef_<C, C*, false, R, Args...> {};                              

template<typename R, typename C, typename... Args> struct MethodTraits<R (C::*)(Args...) const   > :
    internal::MethodTraitsDef_<C, const C*, true, R, Args...> {};                                 

template<typename R, typename C, typename... Args> struct MethodTraits<R (C::*)(Args...) const&  > :
    internal::MethodTraitsDef_<C, const C*, true, R, Args...> {};                               

// Functor Traits

namespace internal {

template<typename TCallOp>
struct FunctorTraitsDef_ :
    CallSigTraits<typename MethodTraits<TCallOp>::CallSig> {
    
    using CallOp = TCallOp;
    using CallOpTraits = MethodTraits<CallOp>;
    using Obj = typename CallOpTraits::Obj;
};

} // namespace internal

template<typename T, typename Enable = bool>
struct FunctorTraits;

template<typename T>
struct FunctorTraits<T, RequiresValid<decltype(&T::operator())>> : 
    internal::FunctorTraitsDef_<decltype(&T::operator())> {};

// Free-Function Traits

namespace internal {

template<typename R, typename... Args>
struct FreeFunctionTraitsDef_ : CallSigTraits<R(Args...)> {
    // nothing more atm..
};

} // namespace internal

template<typename T>
struct FreeFunctionTraits;

template<typename R, typename... Args> struct FreeFunctionTraits<R(*)(Args...)> :
    internal::FreeFunctionTraitsDef_<R, Args...> {}; 

template<typename R, typename... Args> struct FreeFunctionTraits<R   (Args...)> :
    internal::FreeFunctionTraitsDef_<R, Args...> {};

// Callable Traits

enum class CallableKind {
    FreeFunction,
    Method,
    Functor
};

template<typename T, typename Enable = bool>
struct CallableTraits;

template<typename T> struct CallableTraits<T, RequiresValid<typename FreeFunctionTraits<T>::ReturnType>> : FreeFunctionTraits<T> {
    static constexpr CallableKind kind = CallableKind::FreeFunction;
};

template<typename T> struct CallableTraits<T, RequiresValid<typename MethodTraits<T>::ReturnType>> : MethodTraits<T> {
    static constexpr CallableKind kind = CallableKind::Method;
};

template<typename T> struct CallableTraits<T, RequiresValid<typename FunctorTraits<T>::ReturnType>> : FunctorTraits<T> {
    static constexpr CallableKind kind = CallableKind::Functor;
};


template<typename T, typename Enable = void>
struct IsFreeFunction : std::false_type {};
template<typename T>
struct IsFreeFunction<T, std::enable_if_t<CallableTraits<T>::kind == CallableKind::FreeFunction, void>> : std::true_type {};

template<typename T>
inline constexpr bool isFreeFunction = IsFreeFunction<T>::value;


// "Method" = non-rvalue-reference-qualified non-static member function.
// i.e.: For "struct Foo { void foo() &&; };", IsMethod<&Foo::foo> is std::false_type.
//
template<typename T, typename Enable = void>
struct IsMethod : std::false_type {};
template<typename T>
struct IsMethod<T, std::enable_if_t<CallableTraits<T>::kind == CallableKind::Method, void>> : std::true_type {};

// "Method" = non-rvalue-reference-qualified non-static member function.
// i.e.: For "struct Foo { void foo() &&; };", isMethod<&Foo::foo> is false.
//
template<typename T>
inline constexpr bool isMethod = IsMethod<T>::value;


template<typename T, typename Enable = void>
struct IsFunctor : std::false_type {};
template<typename T>
struct IsFunctor<T, std::enable_if_t<CallableTraits<T>::kind == CallableKind::Functor, void>> : std::true_type {};

template<typename T>
inline constexpr bool isFunctor = IsFunctor<T>::value;


template<typename T, typename Enable = bool>
struct IsCallable : std::false_type {};
template<typename T>
struct IsCallable<T, RequiresValid<typename CallableTraits<T>::CallSig>> : std::true_type {};

// Does not include pointers to data members.
template<typename T>
inline constexpr bool isCallable = IsCallable<T>::value;


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

} // namespace internal

template<template<typename...> typename Base, typename Derived>
struct IsTplBaseOf : std::bool_constant<
    std::is_class_v<Derived> && decltype(internal::testIsTplBaseOf<Base, Derived>(0))::value> {};

template<template<typename...> typename Base, typename Derived>
inline constexpr bool isTplBaseOf = IsTplBaseOf<Base, Derived>::value;

namespace internal {

template<template<Int> typename Case, std::size_t... Is>
auto tplFnTable_(std::index_sequence<Is...>) {
    static constexpr std::array<decltype(&Case<0>::execute), sizeof...(Is)> s = {Case<Is>::execute...};
    return s;
};

} // namespace internal

// Experimental
template<template<Int> typename Case, Int N>
auto tplFnTable() {
    return internal::tplFnTable_<Case>(std::make_index_sequence<N>{});
};


} // namespace vgc::core

#endif // VGC_CORE_TEMPLATEUTIL_H
