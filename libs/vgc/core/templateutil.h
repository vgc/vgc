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
#include <iterator>
#include <tuple>
#include <type_traits>
#include <utility>

// clang-format off

namespace vgc::core {

namespace detail {

// Allows to inline SFINAE-based tests on the given type `ArgType`.
// See VGC_CONSTEXPR_IS_ID_ADDRESSABLE_IN_CLASS_ for an example.
//
template<typename ArgType>
struct LambdaSfinae {

    static constexpr bool check(...) {
        return false;
    }

    template<typename Lambda>
    static constexpr auto check(Lambda lambda)
        -> decltype(lambda(std::declval<ArgType>()), bool{}) {

        return true;
    }
};

// Note: we write our own implementation of `std::void_t` because it doesn't
// properly SFINAE on some versions of Clang and produces redefinition errors.
//
template<typename... Ts>
struct MakeVoid_ {
    using type = void;
};

template<typename... Ts>
struct MakeInt_ {
    using type = int;
};

} // namespace detail

template<typename... Ts>
using DependentTrue = std::true_type;

template<typename T>
inline constexpr bool dependentTrue = DependentTrue<T>::value;

template<typename... Ts>
using DependentFalse = std::false_type;

template<typename T>
inline constexpr bool dependentFalse = DependentFalse<T>::value;

/// Same as `std::void_t` without the bugs in some versions of Clang.
///
template<typename... Ts>
using MakeVoid = typename detail::MakeVoid_<Ts...>::type;

/// Similar as `std::void_t` but resolves to the type `int`.
///
template<typename... Ts>
using MakeInt = typename detail::MakeInt_<Ts...>::type;

/// Evaluates at compile-time whether `&cls::id` is a valid expression.
///
#define VGC_CONSTEXPR_IS_ID_ADDRESSABLE_IN_CLASS(cls, id)                                \
    ::vgc::core::detail::LambdaSfinae<cls*>::check(                                      \
        [](auto* v) ->                                                                   \
            ::vgc::core::MakeVoid<decltype(&std::remove_pointer_t<decltype(v)>::id)> {   \
        })

/// Evaluates at compile-time whether `cls::tname` is a valid type.
///
#define VGC_CONSTEXPR_IS_TYPE_DECLARED_IN_CLASS(cls, tname)                              \
    ::vgc::core::detail::LambdaSfinae<cls*>::check(                                      \
        [](auto* v) ->                                                                   \
            ::vgc::core::MakeVoid<typename std::remove_pointer_t<decltype(v)>::tname> {  \
        })

namespace detail {

template<typename U>
struct TypeIdentity_ {
    using type = U;
};

} // namespace detail

/// Allows to specify that a given function parameter should not be used for
/// template argument deduction.
///
/// This is equivalent to `std::type_identity_t` (C++20).
///
/// ```cpp
/// template<T>
/// T add(T x, TypeIdentity<T> y) {
///     return x + y;
/// }
///
/// int main() {
///     std::cout << add(0.5, 2); // would be an error without TypeIdentity
/// }
/// ```
///
/// In the above example, the template argument `T` is forced to be deduced
/// only based on the first function parameter (`0.5`). Once `T` is deduced to
/// be `double`, the second parameter `2`, which is an `int`, is implicitly
/// converted to a `double`.
///
/// Without `TypeIdentity`, the code wouldn't compile because it would be
/// ambiguous whether `add(int, int)` or `add(double, double)` should be
/// called, due to the fact that `int` is convertible to `double` and `double`
/// is convertible to `int`.
///
template<typename U>
using TypeIdentity = typename detail::TypeIdentity_<U>::type;

/// Casts a class enum value to its underlying type.
/// Equivalent to `return static_cast<std::underlying_type_t<Enum>>(e);`.
///
template<typename Enum>
constexpr std::underlying_type_t<Enum> toUnderlying(Enum e) noexcept {
    return static_cast<std::underlying_type_t<Enum>>(e);
}

/// Removes cv-qualifiers (const and volatile) and ref-qualifiers (lvalue and
/// rvalue references) of the given type.
///
/// This is equivalent to `std::remove_cvref_t` (C++20).
///
/// ```cpp
/// RemoveCVRef<int>;                 // => int
/// RemoveCVRef<const int>;           // => int
/// RemoveCVRef<int&>;                // => int
/// RemoveCVRef<const int&>;          // => int
/// RemoveCVRef<int&&>;               // => int
/// RemoveCVRef<volatile int>;        // => int
/// RemoveCVRef<const volatile int>;  // => int
///
/// RemoveCVRef<int*>;                // => int*
/// RemoveCVRef<const int*>;          // => const int*
/// ```
///
template<typename T>
using RemoveCVRef = std::remove_cv_t<std::remove_reference_t<T>>;

/// If the given expression `B` evaluates to `true`, then `Requires<B>`
/// is an alias for `void`. Otherwise, `Requires<B>` is ill-formed.
///
/// This can be used to define SFINAE-based requirements in template
/// specializations.
///
/// `Requires<B>` is equivalent to `std::enable_if_t<B>`.
///
/// # Example 1: class template specialization (no default implementation)
///
/// ```cpp
/// template<typename T, typename SFINAE = void>
/// struct Foo; // by default, Foo<T> is undefined
///
/// template<typename T>
/// struct Foo<T, Requires<std::is_integral_v<T>>> {
///     // define Foo<T> for integral types
/// };
/// ```
///
/// # Example 2: class template specialization (with default implementation)
///
/// ```cpp
/// template<typename T, typename SFINAE = void>
/// struct Foo {
///     // default implementation
/// };
///
/// template<typename T>
/// struct Foo<T, Requires<std::is_integral_v<T>>> {
///     // specialize Foo<T> for integral types
/// };
/// ```
///
/// # Example 3: defining type traits
///
/// ```cpp
/// template<typename T, typename SFINAE = void>
/// struct IsSignedInteger : std::false_type {};
///
/// template<typename T>
/// struct IsSignedInteger<T, Requires<
///         std::is_integral_v<T> &&
///         std::is_signed_v<T>>> :
///     std::true_type {};
///
/// template<typename T>
/// inline constexpr bool isSignedInteger = IsSignedInteger<T>::value;
/// ```
///
/// \sa `RequiresValid<...>`.
///
template<bool B>
using Requires = std::enable_if_t<B>;

/// Defines SFINAE-based requirements for function template overloads.
///
/// ```cpp
/// template<typename T, VGC_REQUIRES(std::is_integral_v<T>)>
/// void foo(T x) {
///     // function overload only defined for integral types
/// }
/// ```
///
#define VGC_REQUIRES(...) std::enable_if_t<(__VA_ARGS__), int> = 0
#define VGC_FORWARDED_REQUIRES(...) std::enable_if_t<(__VA_ARGS__), int>

/// Defines SFINAE-based requirements for function template overloads.
///
/// ```cpp
/// template<typename T, VGC_REQUIRES_VALID(T::iterator, T::value_type)>
/// void foo(T x) {
///     // function overload only defined for types with iterators
/// }
/// ```
///
#define VGC_REQUIRES_VALID(...) ::vgc::core::MakeInt<__VA_ARGS__> = 0

/// If any of the given template arguments are ill-formed, then `RequiresValid<...>`
/// is also ill-formed. Otherwise, `RequiresValid<...>` is an alias for `void`.
///
/// This can be used to define SFINAE-based requirements in template
/// overloads/specializations.
///
/// `RequiresValid<...>` is equivalent to `std::void_t<...>`.
///
template<typename... Ts>
using RequiresValid = MakeVoid<Ts...>;

namespace detail {

template<std::size_t I, typename... T, std::size_t... Is>
constexpr std::tuple<std::tuple_element_t<I + Is, std::tuple<T...>>...>
SubPackAsTuple_(std::index_sequence<Is...>);

} // namespace detail

/// Alias template for `std::tuple<Ti, ..., Tj>`, where `Ti`, ..., `Tj` are the
/// `N` consecutives types starting at index `I` from the given parameter pack
/// `T...`.
///
/// ```cpp
/// using A = SubPackAsTuple<1, 3, int, bool, double, char, float>;
/// // => A is an alias for std::tuple<bool, double, char>
/// ```
///
/// \sa `SubTuple<I, N, Tuple>`
///
template<size_t I, size_t N, typename... T>
using SubPackAsTuple =
    decltype(detail::SubPackAsTuple_<I, T...>(std::make_index_sequence<N>{}));

namespace detail {

template<std::size_t I, typename Tuple, std::size_t... Is>
constexpr std::tuple<std::tuple_element_t<I + Is, Tuple>...>
SubTuple_(std::index_sequence<Is...>);

} // namespace detail

/// Alias template for `std::tuple<Ti, ..., Tj>`, where `Ti`, ..., `Tj` are the
/// `N` consecutives types starting at index `I` from the given tuple
/// `std::tuple<T0, ...., Tn-1>`.
///
/// ```cpp
/// using A = SubTuple<1, 3, std::tuple<int, bool, double, char, float>>;
/// // => A is an alias for std::tuple<bool, double, char>
/// ```
///
/// \sa `SubPackAsTuple<I, N, T...>`
///
template<size_t I, size_t N, typename Tuple>
using SubTuple = decltype(detail::SubTuple_<I, Tuple>(std::make_index_sequence<N>{}));

/// Type trait that checks whether the type `T` is among `Us...`.
///
/// \sa `isAmong<T, Us...>`
///
template<typename T, typename... Us>
struct IsAmong : std::disjunction<std::is_same<T, Us>...> {};

/// Checks whether the type `T` is among `Us...`.
///
/// ```cpp
/// constexpr bool a = isAmong<bool, int, double, char>; // true
/// constexpr bool b = isAmong<bool, int, bool, char>;   // false
/// ```
///
/// \sa `IsAmong<T, Us...>`
///
template<typename T, typename... Us>
inline constexpr bool isAmong = IsAmong<T, Us...>::value;

namespace detail {

template<typename T>
auto testIsComplete_(int) -> std::bool_constant<sizeof(T) == sizeof(T)>;
template<typename T>
auto testIsComplete_(...) -> std::false_type;
template<typename T, typename Tag>
struct IsComplete_ : decltype(testIsComplete_<T>(0)) {};

} // namespace detail

/// Statically asserts that the given `type` is complete. If it is not complete,
/// a compile-time error is issued with `message` as diagnostic message.
///
/// ```cpp
/// struct A;
/// //VGC_ASSERT_TYPE_IS_COMPLETE(A, "A must be complete"); // assert fails
/// struct A {};
/// //VGC_ASSERT_TYPE_IS_COMPLETE(A, "A must be complete"); // assert still fails
/// struct B {};
/// VGC_ASSERT_TYPE_IS_COMPLETE(B, "B must be complete"); // assert passes
/// ```
///
/// \sa `VGC_ASSERT_TYPE_IS_COMPLETE()`
///
#define VGC_ASSERT_TYPE_IS_COMPLETE(type, message)                                       \
    do {                                                                                 \
        struct Tag;                                                                      \
        static_assert(::vgc::core::detail::IsComplete_<type, Tag>::value, message);      \
    } while (0)
    

namespace detail {

template<typename F, typename ArgsTuple, std::size_t... Is>
constexpr decltype(auto) applyPartial_(F&& f, ArgsTuple&& t, std::index_sequence<Is...>) {
    return std::invoke(
        std::forward<F>(f),
        std::get<Is>(std::forward<ArgsTuple>(t))...);
}

} // namespace detail

/// Invokes the function `f` with the first `N` arguments in the tuple `t`.
///
/// ```cpp
/// void printAddition(int a, int b) {
///    std::cout << a + b << std::endl;
/// }
///
/// int main() {
///     std::tuple<int, int, int> t = {2, 3, 4};
///     applyPartial<2>(&test::printAddition, t); // prints "5"
/// }
/// ```
///
template<size_t N, class F, class ArgsTuple>
constexpr decltype(auto) applyPartial(F&& f, ArgsTuple&& t) {
    return detail::applyPartial_(
        std::forward<F>(f),
        std::forward<ArgsTuple>(t),
        std::make_index_sequence<N>{});
}

/// Assuming `T` is a
/// [call signature](https://timsong-cpp.github.io/cppwp/n4659/func.def#def:call_signature)
/// of the form `R (Args...)`, then the struct `CallSignatureTraits<T>`
/// provides the following compile-time information:
///
/// - `CallSignature`: the call signature `R (Args...)` itself
/// - `ReturnType`: the return type `R` of the call signature
/// - `ArgsTuple`: the argument types `Args...` of the call signature
///                represented as the tuple type `std::tuple<Args...>`
/// - `arity`: the number of arguments of the call signature
///
/// ```cpp
/// template<typename T>
/// void print(T x) {
///     std::cout << std::boolalpha << x << std::endl;
/// }
///
/// int add(int a, int b) {
///     return a + b;
/// }
///
/// int main() {
///     using C = CallSignatureTraits<int (bool, double)>;
///     print(std::is_same_v<C::CallSignature, int (bool, double)>);   // true
///     print(std::is_same_v<C::ReturnType, int>);                     // true
///     print(std::is_same_v<C::ArgsTuple, std::tuple<bool, double>>); // true
///     print(C::arity);                                               // 2
///
///     using D = CallSignatureTraits<decltype(add)>;
///     print(std::is_same_v<D::CallSignature, int (int, int)>);  // true
///     print(std::is_same_v<D::ReturnType, int>);                // true
///     print(std::is_same_v<D::ArgsTuple, std::tuple<int, int>); // true
///     print(D::arity);                                          // 2
/// }
/// ```
///
template<typename T>
struct CallSignatureTraits;

template<typename R, typename... Args>
struct CallSignatureTraits<R (Args...)> {
    using CallSignature = R (Args...);
    using ReturnType = R;
    using ArgsTuple = std::tuple<Args...>;
    static constexpr size_t arity = sizeof...(Args);
};

namespace detail {

template<typename R, typename... Args>
struct FreeFunctionTraitsDef : CallSignatureTraits<R (Args...)> {};

} // namespace detail

/// Assuming `T` is a free function type of the form:
///
/// - `R (*)(Args...)` (pointer to function), or
/// - `R (Args...)` (call signature)
///
/// then the struct `FreeFunctionTraits<T>` provides the following compile-time
/// information:
///
/// - `CallSignature`: the call signature `R (Args...)` of the function
/// - `ReturnType`: the return type `R` of the function
/// - `ArgsTuple`: the argument types `Args...` of the function
///                represented as the tuple type `std::tuple<Args...>`
/// - `arity`: the number of arguments of the function
///
/// ```cpp
/// template<typename T>
/// void print(T x) {
///     std::cout << std::boolalpha << x << std::endl;
/// }
///
/// int add(int a, int b) {
///     return a + b;
/// }
///
/// int main() {
///     using F = FreeFunctionTraits<decltype(add)>;
///     print(std::is_same_v<F::CallSignature, int (int, int)>);   // true
///     print(std::is_same_v<F::ReturnType, int>);                 // true
///     print(std::is_same_v<F::ArgsTuple, std::tuple<int, int>>); // true
///     print(F::arity);                                           // 2
///
///     using G = FreeFunctionTraits<decltype(&add)>;
///     print(std::is_same_v<G::CallSignature, int (int, int)>);   // true
///     print(std::is_same_v<G::ReturnType, int>);                 // true
///     print(std::is_same_v<G::ArgsTuple, std::tuple<int, int>>); // true
///     print(G::arity);                                           // 2
/// }
/// ```
///
template<typename T>
struct FreeFunctionTraits;

template<typename R, typename... Args>
struct FreeFunctionTraits<R (*)(Args...)>
    : detail::FreeFunctionTraitsDef<R, Args...> {};

template<typename R, typename... Args>
struct FreeFunctionTraits<R (Args...)>
    : detail::FreeFunctionTraitsDef<R, Args...> {};

namespace detail {

template<
    typename TMethodType,
    typename TClass,
    typename TThis,
    bool IsConst,
    typename R,
    typename... Args>
struct MethodTraitsDef : CallSignatureTraits<R (Args...)> {
    using MethodType = TMethodType;
    using Class = TClass;
    using This = TThis;
    static constexpr bool isConst = IsConst;
};

} // namespace detail

/// Assuming `T` is a (pointer to member) method type that has one of the
/// four following forms:
///
/// - `R (C::*)(Args...)`
/// - `R (C::*)(Args...) &`
/// - `R (C::*)(Args...) const`
/// - `R (C::*)(Args...) const&`
///
/// then the struct `MethodTraits<T>` provides the following compile-time
/// information:
///
/// - `CallSignature`: the call signature `R (Args...)` of the method
/// - `ReturnType`: the return type `R` of the method
/// - `ArgsTuple`: the argument types `Args...` of the method
///                represented as the tuple type `std::tuple<Args...>`
/// - `arity`: the number of arguments of the method
/// - `MethodType`: the (pointer to member) type `T` of the method
/// - `Class`: the class `C` this method is a member of
/// - `This`: the type of `this` in the context of the method (i.e., `C*` or `const C*`)
/// - `isConst`: whether the method is const qualified
///
/// ```cpp
/// template<typename T>
/// void print(T x) {
///     std::cout << std::boolalpha << x << std::endl;
/// }
///
/// struct Range {
///     int min_, max_;
///     bool contains(int i) const {
///         return min_ <= i && i <= max_;
///     }
/// };
///
/// int main() {
///     using M = MethodTraits<decltype(&Range::contains)>;
///     print(std::is_same_v<M::CallSignature, bool (int)>);              // true
///     print(std::is_same_v<M::ReturnType, bool>);                       // true
///     print(std::is_same_v<M::ArgsTuple, std::tuple<int>>);             // true
///     print(M::arity);                                                  // 1
///     print(std::is_same_v<M::MethodType, bool (Range::*)(int) const>); // true
///     print(std::is_same_v<M::Class, Range>);                           // true
///     print(std::is_same_v<M::This, const Range*>);                     // true
///     print(M::isConst);                                                // true
/// }
/// ```
///
/// Note that `MethodTraits<T>` intentionally disallows any
/// rvalue-reference-qualified function type as template parameter `T`. For
/// example, the member function `Foo::foo` defined as:
///
/// ```cpp
/// struct Foo {
///     void foo() && {}
/// };
/// ```
///
/// is not considered a method as per our definition. In other words, we define
/// the term "method" to mean "non-rvalue-reference-qualified non-static member
/// function".
///
template<typename T>
struct MethodTraits;

template<typename R, typename C, typename... Args>
struct MethodTraits<R (C::*)(Args...)>
    : detail::MethodTraitsDef<
        R (C::*)(Args...), C, C*, false, R, Args...> {};

template<typename R, typename C, typename... Args>
struct MethodTraits<R (C::*)(Args...)&>
    : detail::MethodTraitsDef<
        R (C::*)(Args...)&, C, C*, false, R, Args...> {};

template<typename R, typename C, typename... Args>
struct MethodTraits<R (C::*)(Args...) const>
    : detail::MethodTraitsDef<
        R (C::*)(Args...) const, C, const C*, true, R, Args...> {};

template<typename R, typename C, typename... Args>
struct MethodTraits<R (C::*)(Args...) const&>
    : detail::MethodTraitsDef<
        R (C::*)(Args...) const&, C, const C*, true, R, Args...> {};

namespace detail {

template<typename TCallOperator>
struct FunctorTraitsDef : MethodTraits<TCallOperator> {};

} // namespace detail

/// Assuming `T` is a class with one (and only one) operator() method (its
/// "call operator"), then the struct `FunctorTraits<T>` provides the same
/// compile-time information as `MethodTraits<decltype(&T::operator())>`, that
/// is:
///
/// - `CallSignature`: the call signature `R (Args...)` of the call operator
/// - `ReturnType`: the return type `R` of the call operator
/// - `ArgsTuple`: the argument types `Args...` of the call operator
///                represented as the tuple type `std::tuple<Args...>`
/// - `arity`: the number of arguments of the call operator
/// - `MethodType`: the pointer to member type of the call operator
/// - `Class`: the functor class `T`
/// - `This`: the type of `this` in the context of the call operator (i.e., `T*` or `const T*`)
/// - `isConst`: whether the call operator is const qualified
///
/// ```cpp
/// template<typename T>
/// void print(T x) {
///     std::cout << std::boolalpha << x << std::endl;
/// }
///
/// struct Functor {
///     int min_, max_;
///     bool operator()(int i) const {
///         return min_ <= i && i <= max_;
///     }
/// };
///
/// int main() {
///     using F = MethodTraits<Functor>;
///     print(std::is_same_v<F::CallSignature, bool (int)>);                // true
///     print(std::is_same_v<F::ReturnType, bool>);                         // true
///     print(std::is_same_v<F::ArgsTuple, std::tuple<int>>);               // true
///     print(F::arity);                                                    // 1
///     print(std::is_same_v<F::MethodType, bool (Functor::*)(int) const>); // true
///     print(std::is_same_v<F::Class, Functor>);                           // true
///     print(std::is_same_v<F::This, const Functor*>);                     // true
///     print(M::isConst);                                                  // true
/// }
/// ```
///
template<typename T, typename SFINAE = void>
struct FunctorTraits;

template<typename T>
struct FunctorTraits<T, RequiresValid<decltype(&T::operator())>>
    : MethodTraits<decltype(&T::operator())> {};

/// Enumeration of the different kinds of callable.
///
/// \sa `CallableTraits<T>`.
///
enum class CallableKind {
    FreeFunction,
    Method,
    Functor
};

/// Assuming `T` is a callable type (that is, a free function, a method, or a
/// functor), then `CallableTraits<T>` provides the following compile-time
/// information:
///
/// - `kind`: the `CallableKind` of the callable
/// - `CallSignature`: the call signature `R (Args...)` of the callable
/// - `ReturnType`: the return type `R` of the callable
/// - `ArgsTuple`: the argument types `Args...` of the callable
///                represented as the tuple type `std::tuple<Args...>`
/// - `arity`: the number of arguments of the callable
///
/// If `kind` is either `CallableKind::Method` or `CallableKind::Functor`, then
/// `CallableTraits<T>` also provides the following, where "method" refers to
/// `&T::operator()` in the case of a functor:
///
/// - `MethodType`: the pointer to member type of the method
/// - `Class`:  the class this method is a member of
/// - `This`: the type of `this` in the context of the method
/// - `isConst`: whether the method is const qualified
///
/// \sa `FreeFunctionTraits<T>`, `MethodTraits<T>`, and `FunctorTraits<T>`.
///
template<typename T, typename SFINAE = void>
struct CallableTraits;

template<typename T>
struct CallableTraits<T, RequiresValid<typename FreeFunctionTraits<T>::ReturnType>>
    : FreeFunctionTraits<T> {
    static constexpr CallableKind kind = CallableKind::FreeFunction;
};

template<typename T>
struct CallableTraits<T, RequiresValid<typename MethodTraits<T>::ReturnType>>
    : MethodTraits<T> {
    static constexpr CallableKind kind = CallableKind::Method;
};

template<typename T>
struct CallableTraits<T, RequiresValid<typename FunctorTraits<T>::ReturnType>>
    : FunctorTraits<T> {
    static constexpr CallableKind kind = CallableKind::Functor;
};

/// Type trait for `isFreeFunction<T>`.
///
template<typename T, typename SFINAE = void>
struct IsFreeFunction
    : std::false_type {};

template<typename T>
struct IsFreeFunction<T, Requires<CallableTraits<T>::kind == CallableKind::FreeFunction>>
    : std::true_type {};

/// Checks whether the given type `T` is a free function
/// pointer type or a call signature.
///
/// \sa `IsFreeFunction<T>`, `FreeFunctionTraits<T>`.
///
template<typename T>
inline constexpr bool isFreeFunction = IsFreeFunction<T>::value;

/// Type trait for `isMethod<T>`.
///
template<typename T, typename SFINAE = void>
struct IsMethod : std::false_type {};

template<typename T>
struct IsMethod<T, Requires<CallableTraits<T>::kind == CallableKind::Method>>
    : std::true_type {};

/// Checks whether the given type `T` is a pointer to member
/// method.
///
/// Note that by "method", we mean "non-rvalue-reference-qualified non-static
/// member function". See `MethodTraits<T>` for more details.
///
/// \sa `IsMethod<T>`, `MethodTraits<T>`.
///
template<typename T>
inline constexpr bool isMethod = IsMethod<T>::value;

/// Type trait for `isFunctor<T>`.
///
template<typename T, typename SFINAE = void>
struct IsFunctor : std::false_type {};

template<typename T>
struct IsFunctor<T, Requires<CallableTraits<T>::kind == CallableKind::Functor>>
    : std::true_type {};

/// Checks whether the given type `T` is a functor.
///
/// Note that by "functor", we mean a class that has one and only one
/// operator() method. See `FunctorTraits<T>` for more details.
///
/// \sa `IsFunctor<T>`, `FunctorTraits<T>`.
///
template<typename T>
inline constexpr bool isFunctor = IsFunctor<T>::value;

/// Type trait for `isCallable<T>`.
///
template<typename T, typename SFINAE = void>
struct IsCallable : std::false_type {};

template<typename T>
struct IsCallable<T, RequiresValid<typename CallableTraits<T>::CallSignature>>
    : std::true_type {};

/// Checks whether the given type `T` is callable, that is, whether it is a
/// free function, a method, or a functor.
///
/// \sa `IsCallable<T>`, `isFreeFunction<T>`, `isMethod<T>`, `isFunctor<T>`.
///
template<typename T>
inline constexpr bool isCallable = IsCallable<T>::value;

namespace detail {

template<template<typename...> typename Base, typename... Ts>
std::true_type testIsConvertibleToTemplateBasePointer(const volatile Base<Ts...>*);

template<template<typename...> typename Base>
std::false_type testIsConvertibleToTemplateBasePointer(const volatile void*);

template<template<typename...> typename Base, typename Derived>
auto testIsTemplateBaseOf(int) -> decltype(testIsConvertibleToTemplateBasePointer<Base>(
    static_cast<Derived*>(nullptr)));

template<template<typename...> typename Base, typename Derived>
auto testIsTemplateBaseOf(...) -> std::true_type; // inaccessible base

} // namespace detail

/// Type trait for `isTemplateBaseOf<Base, Derived>`.
///
template<template<typename...> typename Base, typename Derived>
struct IsTemplateBaseOf : std::bool_constant<
    std::is_class_v<Derived>
    && decltype(detail::testIsTemplateBaseOf<Base, Derived>(0))::value> {};

/// Checks whether `Base` is a class template and `Derived` is a subclass of
/// `Based<Args...>` for some `Args...`.
///
/// ```cpp
/// template<typename T>
/// struct Base {};
///
/// struct Derived : Base<int> {};
///
/// int main() {
///     std::cout << isTemplateBaseOf<Base, Derived> << '\n'; // true
/// }
/// ```
///
/// \sa `IsTemplateBaseOf<T>`
///
template<template<typename...> typename Base, typename Derived>
inline constexpr bool isTemplateBaseOf = IsTemplateBaseOf<Base, Derived>::value;

/// Type trait for `isUnsignedInteger<T>`.
///
template<typename T, typename SFINAE = void>
struct IsUnsignedInteger : std::false_type {};

template<typename T>
struct IsUnsignedInteger<T, Requires<std::is_integral_v<T> && std::is_unsigned_v<T>>>
    : std::true_type {};

/// Checks whether `T` is an unsigned integer type.
///
/// This is equivalent to `std::is_integral_v<T> && std::is_unsigned_v<T>`.
///
/// \sa `IsUnsignedInteger<T>`
///
template<typename T>
inline constexpr bool isUnsignedInteger = IsUnsignedInteger<T>::value;

/// Type trait for `isSignedInteger<T>`.
///
template<typename T, typename SFINAE = void>
struct IsSignedInteger : std::false_type {};

template<typename T>
struct IsSignedInteger<T, Requires<std::is_integral_v<T> && std::is_signed_v<T>>>
    : std::true_type {};

/// Checks whether `T` is a signed integer type.
///
/// This is equivalent to `std::is_integral_v<T> && std::is_signed_v<T>`.
///
/// \sa `IsSignedInteger<T>`
///
template<typename T>
inline constexpr bool isSignedInteger = IsSignedInteger<T>::value;

/// Type trait for `isInputIterator<T>`.
///
template<typename T, typename SFINAE = void>
struct IsInputIterator : std::false_type {};

template<typename T>
struct IsInputIterator<T, Requires<
    std::is_convertible_v<
        typename std::iterator_traits<T>::iterator_category,
        std::input_iterator_tag>>>
    : std::true_type {};

/// Checks whether `T` satisfies the concept of input iterator.
///
/// \sa `IsInputIterator<T>`, `isForwardIterator<T>`.
///
template<typename T>
inline constexpr bool isInputIterator = IsInputIterator<T>::value;

/// Type trait for `isForwardIterator<T>`.
///
template<typename T, typename SFINAE = void>
struct IsForwardIterator : std::false_type {};

template<typename T>
struct IsForwardIterator<T, Requires<
    std::is_convertible_v<
        typename std::iterator_traits<T>::iterator_category,
        std::forward_iterator_tag>>>
    : std::true_type {};

/// Checks whether `T` satisfies the concept of forward iterator.
///
/// \sa `IsForwardIterator<T>`, `isInputIterator<T>`, `isRange<T>`.
///
template<typename T>
inline constexpr bool isForwardIterator = IsForwardIterator<T>::value;

/// Type trait for `isRange<T>`.
///
template<typename T, typename SFINAE = void>
struct IsRange : std::false_type {};

template<typename T>
struct IsRange<T, Requires<
        isForwardIterator<decltype(std::declval<T>().begin())>
        && isForwardIterator<decltype(std::declval<T>().end())>>>
    : std::true_type {};

/// Checks whether `T` satisfies the concept of range, that is, whether it has
/// `T::begin()` and `T::end()` and they return forward iterators.
///
/// \sa `IsRange<T>`, `isForwardIterator<T>`.
///
template<typename T>
inline constexpr bool isRange = IsRange<T>::value;

} // namespace vgc::core

// clang-format on

#endif // VGC_CORE_TEMPLATEUTIL_H
