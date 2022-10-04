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

#ifndef VGC_CORE_PREPROCESSOR_H
#define VGC_CORE_PREPROCESSOR_H

// clang-format off

/// Converts the given argument to a string literal, after performing macro
/// expansion.
///
#define VGC_PP_STR(x) VGC_PP_STR_(x)
#define VGC_PP_STR_(x) #x

/// Concatenates the two given arguments after performing macro expansion.
///
#define VGC_PP_CAT(x, y) VGC_PP_CAT_(x, y)
#define VGC_PP_CAT_(x, y) x ## y

/// Expands the given argument before further macro processing.
/// 
/// Such expansion is required in some compilers (e.g., MSVC) whenever
/// you need to call a macro whose arguments are themselves macros or __VA_ARGS__.
/// 
/// Example (on MSVC 2019): 
///
/// ```cpp
/// #define ADD(x, y) x + y
/// #define APPLY_OP_BUG(op, ...) op(__VA_ARGS__)
/// #define APPLY_OP_FIX(op, ...) VGC_PP_EXPAND(op(__VA_ARGS__))
/// 
/// int x = APPLY_OP_BUG(ADD, 1, 2); // Expands to `1, 2 +` => compile error
/// int y = APPLY_OP_FIX(ADD, 1, 2); // Expands to `1 + 2`  => ok
/// ```
/// 
/// Note: the similar Boost macro BOOST_PP_EXPAND performs double-expansion, while
/// our macro VGC_PP_EXPAND only performs single-expansion, which seems enough
/// in all our use cases (compiling with MSVC, GCC, and Clang). If double-expansion
/// is required, you can use VGC_PP_EXPAND_TWICE.
/// 
#define VGC_PP_EXPAND(x) x

/// Expands the given argument twice before further macro processing.
///
#define VGC_PP_EXPAND_TWICE(x) VGC_PP_EXPAND(x)

/// Returns the first argument of the two given arguments.
//
#define VGC_PP_PAIR_FIRST(x, y) x

/// Returns the second argument of the two given arguments.
//
#define VGC_PP_PAIR_SECOND(x, y) y

/// Returns both arguments of a pair, separated by a whitespace.
//
#define VGC_PP_PAIR_BOTH(x, y) x y

/// Returns the number of arguments of a macro.
///
#define VGC_PP_NUM_ARGS(...)                        \
    VGC_PP_EXPAND(VGC_PP_NUM_ARGS_DISPATCH(         \
        __VA_ARGS__, 100,                           \
        99, 98, 97, 96, 95, 94, 93, 92, 91, 90,     \
        89, 88, 87, 86, 85, 84, 83, 82, 81, 80,     \
        79, 78, 77, 76, 75, 74, 73, 72, 71, 70,     \
        69, 68, 67, 66, 65, 64, 63, 62, 61, 60,     \
        59, 58, 57, 56, 55, 54, 53, 52, 51, 50,     \
        49, 48, 47, 46, 45, 44, 43, 42, 41, 40,     \
        39, 38, 37, 36, 35, 34, 33, 32, 31, 30,     \
        29, 28, 27, 26, 25, 24, 23, 22, 21, 20,     \
        19, 18, 17, 16, 15, 14, 13, 12, 11, 10,     \
        9, 8, 7, 6, 5, 4, 3, 2, 1, 0))

#define VGC_PP_NUM_ARGS_DISPATCH(                      \
    _00, _01, _02, _03, _04, _05, _06, _07, _08, _09,  \
    _10, _11, _12, _13, _14, _15, _16, _17, _18, _19,  \
    _20, _21, _22, _23, _24, _25, _26, _27, _28, _29,  \
    _30, _31, _32, _33, _34, _35, _36, _37, _38, _39,  \
    _40, _41, _42, _43, _44, _45, _46, _47, _48, _49,  \
    _50, _51, _52, _53, _54, _55, _56, _57, _58, _59,  \
    _60, _61, _62, _63, _64, _65, _66, _67, _68, _69,  \
    _70, _71, _72, _73, _74, _75, _76, _77, _78, _79,  \
    _80, _81, _82, _83, _84, _85, _86, _87, _88, _89,  \
    _90, _91, _92, _93, _94, _95, _96, _97, _98, _99,  \
    S, ...) S

/// Allows to define macro overloads based on the number of arguments.
///
/// ```cpp
/// #define FOO_1(x) doSomething(x)
/// #define FOO_2(x, y) doSomethingElse(x, y)
/// #define FOO(...) VGC_PP_EXPAND(VGC_PP_OVERLOAD(FOO_,__VA_ARGS__)(__VA_ARGS__))
///
/// // => Now you can use either FOO(x) or FOO(x, y)
/// ```
///
#define VGC_PP_OVERLOAD(prefix, ...) VGC_PP_CAT(prefix, VGC_PP_NUM_ARGS(__VA_ARGS__))

/// The macro `VGC_PP_FOREACH(F, x, t1, t2, ...)` expands to `F(x, t1) F(x, t2) ...`.
///
/// Example:
///
/// ```cpp
/// #define PRINT(x, t) std::cout << t << x;
/// VGC_PP_FOREACH(PRINT, std::endl, "Hello", "World")
/// ```
///
/// Preprocessor output:
///
/// ```cpp
/// std::cout << "Hello" << std::endl; std::cout << "World" << std::endl;
/// ```
///
#define VGC_PP_FOREACH_X(...) VGC_PP_EXPAND(VGC_PP_OVERLOAD(VGC_PP_FOREACH_X_,__VA_ARGS__)(__VA_ARGS__))
#define VGC_PP_FOREACH(...) VGC_PP_EXPAND(VGC_PP_FOREACH_X(__VA_ARGS__, ~))

#define VGC_PP_FOREACH_X_1(_)
#define VGC_PP_FOREACH_X_2(F, _)
#define VGC_PP_FOREACH_X_3(F, x, _)
#define VGC_PP_FOREACH_X_4(F, x, t, ...)   F(x, t) VGC_PP_EXPAND(VGC_PP_FOREACH_X_3(F, x, __VA_ARGS__))
#define VGC_PP_FOREACH_X_5(F, x, t, ...)   F(x, t) VGC_PP_EXPAND(VGC_PP_FOREACH_X_4(F, x, __VA_ARGS__))
#define VGC_PP_FOREACH_X_6(F, x, t, ...)   F(x, t) VGC_PP_EXPAND(VGC_PP_FOREACH_X_5(F, x, __VA_ARGS__))
#define VGC_PP_FOREACH_X_7(F, x, t, ...)   F(x, t) VGC_PP_EXPAND(VGC_PP_FOREACH_X_6(F, x, __VA_ARGS__))
#define VGC_PP_FOREACH_X_8(F, x, t, ...)   F(x, t) VGC_PP_EXPAND(VGC_PP_FOREACH_X_7(F, x, __VA_ARGS__))
#define VGC_PP_FOREACH_X_9(F, x, t, ...)   F(x, t) VGC_PP_EXPAND(VGC_PP_FOREACH_X_8(F, x, __VA_ARGS__))
#define VGC_PP_FOREACH_X_10(F, x, t, ...)  F(x, t) VGC_PP_EXPAND(VGC_PP_FOREACH_X_9(F, x, __VA_ARGS__))
#define VGC_PP_FOREACH_X_11(F, x, t, ...)  F(x, t) VGC_PP_EXPAND(VGC_PP_FOREACH_X_10(F, x, __VA_ARGS__))
#define VGC_PP_FOREACH_X_12(F, x, t, ...)  F(x, t) VGC_PP_EXPAND(VGC_PP_FOREACH_X_11(F, x, __VA_ARGS__))
#define VGC_PP_FOREACH_X_13(F, x, t, ...)  F(x, t) VGC_PP_EXPAND(VGC_PP_FOREACH_X_12(F, x, __VA_ARGS__))
#define VGC_PP_FOREACH_X_14(F, x, t, ...)  F(x, t) VGC_PP_EXPAND(VGC_PP_FOREACH_X_13(F, x, __VA_ARGS__))
#define VGC_PP_FOREACH_X_15(F, x, t, ...)  F(x, t) VGC_PP_EXPAND(VGC_PP_FOREACH_X_14(F, x, __VA_ARGS__))
#define VGC_PP_FOREACH_X_16(F, x, t, ...)  F(x, t) VGC_PP_EXPAND(VGC_PP_FOREACH_X_15(F, x, __VA_ARGS__))
#define VGC_PP_FOREACH_X_17(F, x, t, ...)  F(x, t) VGC_PP_EXPAND(VGC_PP_FOREACH_X_16(F, x, __VA_ARGS__))
#define VGC_PP_FOREACH_X_18(F, x, t, ...)  F(x, t) VGC_PP_EXPAND(VGC_PP_FOREACH_X_17(F, x, __VA_ARGS__))
#define VGC_PP_FOREACH_X_19(F, x, t, ...)  F(x, t) VGC_PP_EXPAND(VGC_PP_FOREACH_X_18(F, x, __VA_ARGS__))
#define VGC_PP_FOREACH_X_20(F, x, t, ...)  F(x, t) VGC_PP_EXPAND(VGC_PP_FOREACH_X_19(F, x, __VA_ARGS__))
#define VGC_PP_FOREACH_X_21(F, x, t, ...)  F(x, t) VGC_PP_EXPAND(VGC_PP_FOREACH_X_20(F, x, __VA_ARGS__))
#define VGC_PP_FOREACH_X_22(F, x, t, ...)  F(x, t) VGC_PP_EXPAND(VGC_PP_FOREACH_X_21(F, x, __VA_ARGS__))
#define VGC_PP_FOREACH_X_23(F, x, t, ...)  F(x, t) VGC_PP_EXPAND(VGC_PP_FOREACH_X_22(F, x, __VA_ARGS__))
#define VGC_PP_FOREACH_X_24(F, x, t, ...)  F(x, t) VGC_PP_EXPAND(VGC_PP_FOREACH_X_23(F, x, __VA_ARGS__))
#define VGC_PP_FOREACH_X_25(F, x, t, ...)  F(x, t) VGC_PP_EXPAND(VGC_PP_FOREACH_X_24(F, x, __VA_ARGS__))
#define VGC_PP_FOREACH_X_26(F, x, t, ...)  F(x, t) VGC_PP_EXPAND(VGC_PP_FOREACH_X_25(F, x, __VA_ARGS__))
#define VGC_PP_FOREACH_X_27(F, x, t, ...)  F(x, t) VGC_PP_EXPAND(VGC_PP_FOREACH_X_26(F, x, __VA_ARGS__))
#define VGC_PP_FOREACH_X_28(F, x, t, ...)  F(x, t) VGC_PP_EXPAND(VGC_PP_FOREACH_X_27(F, x, __VA_ARGS__))
#define VGC_PP_FOREACH_X_29(F, x, t, ...)  F(x, t) VGC_PP_EXPAND(VGC_PP_FOREACH_X_28(F, x, __VA_ARGS__))
#define VGC_PP_FOREACH_X_30(F, x, t, ...)  F(x, t) VGC_PP_EXPAND(VGC_PP_FOREACH_X_29(F, x, __VA_ARGS__))
#define VGC_PP_FOREACH_X_31(F, x, t, ...)  F(x, t) VGC_PP_EXPAND(VGC_PP_FOREACH_X_30(F, x, __VA_ARGS__))
#define VGC_PP_FOREACH_X_32(F, x, t, ...)  F(x, t) VGC_PP_EXPAND(VGC_PP_FOREACH_X_31(F, x, __VA_ARGS__))
#define VGC_PP_FOREACH_X_33(F, x, t, ...)  F(x, t) VGC_PP_EXPAND(VGC_PP_FOREACH_X_32(F, x, __VA_ARGS__))
#define VGC_PP_FOREACH_X_34(F, x, t, ...)  F(x, t) VGC_PP_EXPAND(VGC_PP_FOREACH_X_33(F, x, __VA_ARGS__))
#define VGC_PP_FOREACH_X_35(F, x, t, ...)  F(x, t) VGC_PP_EXPAND(VGC_PP_FOREACH_X_34(F, x, __VA_ARGS__))
#define VGC_PP_FOREACH_X_36(F, x, t, ...)  F(x, t) VGC_PP_EXPAND(VGC_PP_FOREACH_X_35(F, x, __VA_ARGS__))
#define VGC_PP_FOREACH_X_37(F, x, t, ...)  F(x, t) VGC_PP_EXPAND(VGC_PP_FOREACH_X_36(F, x, __VA_ARGS__))
#define VGC_PP_FOREACH_X_38(F, x, t, ...)  F(x, t) VGC_PP_EXPAND(VGC_PP_FOREACH_X_37(F, x, __VA_ARGS__))
#define VGC_PP_FOREACH_X_39(F, x, t, ...)  F(x, t) VGC_PP_EXPAND(VGC_PP_FOREACH_X_38(F, x, __VA_ARGS__))
#define VGC_PP_FOREACH_X_40(F, x, t, ...)  F(x, t) VGC_PP_EXPAND(VGC_PP_FOREACH_X_39(F, x, __VA_ARGS__))
#define VGC_PP_FOREACH_X_41(F, x, t, ...)  F(x, t) VGC_PP_EXPAND(VGC_PP_FOREACH_X_40(F, x, __VA_ARGS__))
#define VGC_PP_FOREACH_X_42(F, x, t, ...)  F(x, t) VGC_PP_EXPAND(VGC_PP_FOREACH_X_41(F, x, __VA_ARGS__))
#define VGC_PP_FOREACH_X_43(F, x, t, ...)  F(x, t) VGC_PP_EXPAND(VGC_PP_FOREACH_X_42(F, x, __VA_ARGS__))
#define VGC_PP_FOREACH_X_44(F, x, t, ...)  F(x, t) VGC_PP_EXPAND(VGC_PP_FOREACH_X_43(F, x, __VA_ARGS__))
#define VGC_PP_FOREACH_X_45(F, x, t, ...)  F(x, t) VGC_PP_EXPAND(VGC_PP_FOREACH_X_44(F, x, __VA_ARGS__))
#define VGC_PP_FOREACH_X_46(F, x, t, ...)  F(x, t) VGC_PP_EXPAND(VGC_PP_FOREACH_X_45(F, x, __VA_ARGS__))
#define VGC_PP_FOREACH_X_47(F, x, t, ...)  F(x, t) VGC_PP_EXPAND(VGC_PP_FOREACH_X_46(F, x, __VA_ARGS__))
#define VGC_PP_FOREACH_X_48(F, x, t, ...)  F(x, t) VGC_PP_EXPAND(VGC_PP_FOREACH_X_47(F, x, __VA_ARGS__))
#define VGC_PP_FOREACH_X_49(F, x, t, ...)  F(x, t) VGC_PP_EXPAND(VGC_PP_FOREACH_X_48(F, x, __VA_ARGS__))
#define VGC_PP_FOREACH_X_50(F, x, t, ...)  F(x, t) VGC_PP_EXPAND(VGC_PP_FOREACH_X_49(F, x, __VA_ARGS__))
#define VGC_PP_FOREACH_X_51(F, x, t, ...)  F(x, t) VGC_PP_EXPAND(VGC_PP_FOREACH_X_50(F, x, __VA_ARGS__))
#define VGC_PP_FOREACH_X_52(F, x, t, ...)  F(x, t) VGC_PP_EXPAND(VGC_PP_FOREACH_X_51(F, x, __VA_ARGS__))
#define VGC_PP_FOREACH_X_53(F, x, t, ...)  F(x, t) VGC_PP_EXPAND(VGC_PP_FOREACH_X_52(F, x, __VA_ARGS__))
#define VGC_PP_FOREACH_X_54(F, x, t, ...)  F(x, t) VGC_PP_EXPAND(VGC_PP_FOREACH_X_53(F, x, __VA_ARGS__))
#define VGC_PP_FOREACH_X_55(F, x, t, ...)  F(x, t) VGC_PP_EXPAND(VGC_PP_FOREACH_X_54(F, x, __VA_ARGS__))
#define VGC_PP_FOREACH_X_56(F, x, t, ...)  F(x, t) VGC_PP_EXPAND(VGC_PP_FOREACH_X_55(F, x, __VA_ARGS__))
#define VGC_PP_FOREACH_X_57(F, x, t, ...)  F(x, t) VGC_PP_EXPAND(VGC_PP_FOREACH_X_56(F, x, __VA_ARGS__))
#define VGC_PP_FOREACH_X_58(F, x, t, ...)  F(x, t) VGC_PP_EXPAND(VGC_PP_FOREACH_X_57(F, x, __VA_ARGS__))
#define VGC_PP_FOREACH_X_59(F, x, t, ...)  F(x, t) VGC_PP_EXPAND(VGC_PP_FOREACH_X_58(F, x, __VA_ARGS__))
#define VGC_PP_FOREACH_X_60(F, x, t, ...)  F(x, t) VGC_PP_EXPAND(VGC_PP_FOREACH_X_49(F, x, __VA_ARGS__))
#define VGC_PP_FOREACH_X_61(F, x, t, ...)  F(x, t) VGC_PP_EXPAND(VGC_PP_FOREACH_X_60(F, x, __VA_ARGS__))
#define VGC_PP_FOREACH_X_62(F, x, t, ...)  F(x, t) VGC_PP_EXPAND(VGC_PP_FOREACH_X_61(F, x, __VA_ARGS__))
#define VGC_PP_FOREACH_X_63(F, x, t, ...)  F(x, t) VGC_PP_EXPAND(VGC_PP_FOREACH_X_62(F, x, __VA_ARGS__))
#define VGC_PP_FOREACH_X_64(F, x, t, ...)  F(x, t) VGC_PP_EXPAND(VGC_PP_FOREACH_X_63(F, x, __VA_ARGS__))
#define VGC_PP_FOREACH_X_65(F, x, t, ...)  F(x, t) VGC_PP_EXPAND(VGC_PP_FOREACH_X_64(F, x, __VA_ARGS__))
#define VGC_PP_FOREACH_X_66(F, x, t, ...)  F(x, t) VGC_PP_EXPAND(VGC_PP_FOREACH_X_65(F, x, __VA_ARGS__))
#define VGC_PP_FOREACH_X_67(F, x, t, ...)  F(x, t) VGC_PP_EXPAND(VGC_PP_FOREACH_X_66(F, x, __VA_ARGS__))
#define VGC_PP_FOREACH_X_68(F, x, t, ...)  F(x, t) VGC_PP_EXPAND(VGC_PP_FOREACH_X_67(F, x, __VA_ARGS__))
#define VGC_PP_FOREACH_X_69(F, x, t, ...)  F(x, t) VGC_PP_EXPAND(VGC_PP_FOREACH_X_68(F, x, __VA_ARGS__))
#define VGC_PP_FOREACH_X_70(F, x, t, ...)  F(x, t) VGC_PP_EXPAND(VGC_PP_FOREACH_X_69(F, x, __VA_ARGS__))
#define VGC_PP_FOREACH_X_71(F, x, t, ...)  F(x, t) VGC_PP_EXPAND(VGC_PP_FOREACH_X_70(F, x, __VA_ARGS__))
#define VGC_PP_FOREACH_X_72(F, x, t, ...)  F(x, t) VGC_PP_EXPAND(VGC_PP_FOREACH_X_71(F, x, __VA_ARGS__))
#define VGC_PP_FOREACH_X_73(F, x, t, ...)  F(x, t) VGC_PP_EXPAND(VGC_PP_FOREACH_X_72(F, x, __VA_ARGS__))
#define VGC_PP_FOREACH_X_74(F, x, t, ...)  F(x, t) VGC_PP_EXPAND(VGC_PP_FOREACH_X_73(F, x, __VA_ARGS__))
#define VGC_PP_FOREACH_X_75(F, x, t, ...)  F(x, t) VGC_PP_EXPAND(VGC_PP_FOREACH_X_74(F, x, __VA_ARGS__))
#define VGC_PP_FOREACH_X_76(F, x, t, ...)  F(x, t) VGC_PP_EXPAND(VGC_PP_FOREACH_X_75(F, x, __VA_ARGS__))
#define VGC_PP_FOREACH_X_77(F, x, t, ...)  F(x, t) VGC_PP_EXPAND(VGC_PP_FOREACH_X_76(F, x, __VA_ARGS__))
#define VGC_PP_FOREACH_X_78(F, x, t, ...)  F(x, t) VGC_PP_EXPAND(VGC_PP_FOREACH_X_77(F, x, __VA_ARGS__))
#define VGC_PP_FOREACH_X_79(F, x, t, ...)  F(x, t) VGC_PP_EXPAND(VGC_PP_FOREACH_X_78(F, x, __VA_ARGS__))
#define VGC_PP_FOREACH_X_80(F, x, t, ...)  F(x, t) VGC_PP_EXPAND(VGC_PP_FOREACH_X_79(F, x, __VA_ARGS__))
#define VGC_PP_FOREACH_X_81(F, x, t, ...)  F(x, t) VGC_PP_EXPAND(VGC_PP_FOREACH_X_80(F, x, __VA_ARGS__))
#define VGC_PP_FOREACH_X_82(F, x, t, ...)  F(x, t) VGC_PP_EXPAND(VGC_PP_FOREACH_X_81(F, x, __VA_ARGS__))
#define VGC_PP_FOREACH_X_83(F, x, t, ...)  F(x, t) VGC_PP_EXPAND(VGC_PP_FOREACH_X_82(F, x, __VA_ARGS__))
#define VGC_PP_FOREACH_X_84(F, x, t, ...)  F(x, t) VGC_PP_EXPAND(VGC_PP_FOREACH_X_83(F, x, __VA_ARGS__))
#define VGC_PP_FOREACH_X_85(F, x, t, ...)  F(x, t) VGC_PP_EXPAND(VGC_PP_FOREACH_X_84(F, x, __VA_ARGS__))
#define VGC_PP_FOREACH_X_86(F, x, t, ...)  F(x, t) VGC_PP_EXPAND(VGC_PP_FOREACH_X_85(F, x, __VA_ARGS__))
#define VGC_PP_FOREACH_X_87(F, x, t, ...)  F(x, t) VGC_PP_EXPAND(VGC_PP_FOREACH_X_86(F, x, __VA_ARGS__))
#define VGC_PP_FOREACH_X_88(F, x, t, ...)  F(x, t) VGC_PP_EXPAND(VGC_PP_FOREACH_X_87(F, x, __VA_ARGS__))
#define VGC_PP_FOREACH_X_89(F, x, t, ...)  F(x, t) VGC_PP_EXPAND(VGC_PP_FOREACH_X_88(F, x, __VA_ARGS__))
#define VGC_PP_FOREACH_X_90(F, x, t, ...)  F(x, t) VGC_PP_EXPAND(VGC_PP_FOREACH_X_89(F, x, __VA_ARGS__))
#define VGC_PP_FOREACH_X_91(F, x, t, ...)  F(x, t) VGC_PP_EXPAND(VGC_PP_FOREACH_X_90(F, x, __VA_ARGS__))
#define VGC_PP_FOREACH_X_92(F, x, t, ...)  F(x, t) VGC_PP_EXPAND(VGC_PP_FOREACH_X_91(F, x, __VA_ARGS__))
#define VGC_PP_FOREACH_X_93(F, x, t, ...)  F(x, t) VGC_PP_EXPAND(VGC_PP_FOREACH_X_92(F, x, __VA_ARGS__))
#define VGC_PP_FOREACH_X_94(F, x, t, ...)  F(x, t) VGC_PP_EXPAND(VGC_PP_FOREACH_X_93(F, x, __VA_ARGS__))
#define VGC_PP_FOREACH_X_95(F, x, t, ...)  F(x, t) VGC_PP_EXPAND(VGC_PP_FOREACH_X_94(F, x, __VA_ARGS__))
#define VGC_PP_FOREACH_X_96(F, x, t, ...)  F(x, t) VGC_PP_EXPAND(VGC_PP_FOREACH_X_95(F, x, __VA_ARGS__))
#define VGC_PP_FOREACH_X_97(F, x, t, ...)  F(x, t) VGC_PP_EXPAND(VGC_PP_FOREACH_X_96(F, x, __VA_ARGS__))
#define VGC_PP_FOREACH_X_98(F, x, t, ...)  F(x, t) VGC_PP_EXPAND(VGC_PP_FOREACH_X_97(F, x, __VA_ARGS__))
#define VGC_PP_FOREACH_X_99(F, x, t, ...)  F(x, t) VGC_PP_EXPAND(VGC_PP_FOREACH_X_98(F, x, __VA_ARGS__))
#define VGC_PP_FOREACH_X_100(F, x, t, ...) F(x, t) VGC_PP_EXPAND(VGC_PP_FOREACH_X_99(F, x, __VA_ARGS__))

/// The macro `VGC_PP_TRANSFORM(F, x, t1, t2, ...)` expands to `F(x, t1), F(x, t2) ...`.
///
#define VGC_PP_TRANSFORM_X(...) VGC_PP_EXPAND(VGC_PP_OVERLOAD(VGC_PP_TRANSFORM_X_,__VA_ARGS__)(__VA_ARGS__))
#define VGC_PP_TRANSFORM(...) VGC_PP_EXPAND(VGC_PP_TRANSFORM_X(__VA_ARGS__, ~))

#define VGC_PP_TRANSFORM_X_1(_)
#define VGC_PP_TRANSFORM_X_2(F, _)
#define VGC_PP_TRANSFORM_X_3(F, x, _)
#define VGC_PP_TRANSFORM_X_4(F, x, t, ...)   F(x, t)
#define VGC_PP_TRANSFORM_X_5(F, x, t, ...)   F(x, t), VGC_PP_EXPAND(VGC_PP_TRANSFORM_X_4(F, x, __VA_ARGS__))
#define VGC_PP_TRANSFORM_X_6(F, x, t, ...)   F(x, t), VGC_PP_EXPAND(VGC_PP_TRANSFORM_X_5(F, x, __VA_ARGS__))
#define VGC_PP_TRANSFORM_X_7(F, x, t, ...)   F(x, t), VGC_PP_EXPAND(VGC_PP_TRANSFORM_X_6(F, x, __VA_ARGS__))
#define VGC_PP_TRANSFORM_X_8(F, x, t, ...)   F(x, t), VGC_PP_EXPAND(VGC_PP_TRANSFORM_X_7(F, x, __VA_ARGS__))
#define VGC_PP_TRANSFORM_X_9(F, x, t, ...)   F(x, t), VGC_PP_EXPAND(VGC_PP_TRANSFORM_X_8(F, x, __VA_ARGS__))
#define VGC_PP_TRANSFORM_X_10(F, x, t, ...)  F(x, t), VGC_PP_EXPAND(VGC_PP_TRANSFORM_X_9(F, x, __VA_ARGS__))
#define VGC_PP_TRANSFORM_X_11(F, x, t, ...)  F(x, t), VGC_PP_EXPAND(VGC_PP_TRANSFORM_X_10(F, x, __VA_ARGS__))
#define VGC_PP_TRANSFORM_X_12(F, x, t, ...)  F(x, t), VGC_PP_EXPAND(VGC_PP_TRANSFORM_X_11(F, x, __VA_ARGS__))
#define VGC_PP_TRANSFORM_X_13(F, x, t, ...)  F(x, t), VGC_PP_EXPAND(VGC_PP_TRANSFORM_X_12(F, x, __VA_ARGS__))
#define VGC_PP_TRANSFORM_X_14(F, x, t, ...)  F(x, t), VGC_PP_EXPAND(VGC_PP_TRANSFORM_X_13(F, x, __VA_ARGS__))
#define VGC_PP_TRANSFORM_X_15(F, x, t, ...)  F(x, t), VGC_PP_EXPAND(VGC_PP_TRANSFORM_X_14(F, x, __VA_ARGS__))
#define VGC_PP_TRANSFORM_X_16(F, x, t, ...)  F(x, t), VGC_PP_EXPAND(VGC_PP_TRANSFORM_X_15(F, x, __VA_ARGS__))
#define VGC_PP_TRANSFORM_X_17(F, x, t, ...)  F(x, t), VGC_PP_EXPAND(VGC_PP_TRANSFORM_X_16(F, x, __VA_ARGS__))
#define VGC_PP_TRANSFORM_X_18(F, x, t, ...)  F(x, t), VGC_PP_EXPAND(VGC_PP_TRANSFORM_X_17(F, x, __VA_ARGS__))
#define VGC_PP_TRANSFORM_X_19(F, x, t, ...)  F(x, t), VGC_PP_EXPAND(VGC_PP_TRANSFORM_X_18(F, x, __VA_ARGS__))
#define VGC_PP_TRANSFORM_X_20(F, x, t, ...)  F(x, t), VGC_PP_EXPAND(VGC_PP_TRANSFORM_X_19(F, x, __VA_ARGS__))
#define VGC_PP_TRANSFORM_X_21(F, x, t, ...)  F(x, t), VGC_PP_EXPAND(VGC_PP_TRANSFORM_X_20(F, x, __VA_ARGS__))
#define VGC_PP_TRANSFORM_X_22(F, x, t, ...)  F(x, t), VGC_PP_EXPAND(VGC_PP_TRANSFORM_X_21(F, x, __VA_ARGS__))
#define VGC_PP_TRANSFORM_X_23(F, x, t, ...)  F(x, t), VGC_PP_EXPAND(VGC_PP_TRANSFORM_X_22(F, x, __VA_ARGS__))
#define VGC_PP_TRANSFORM_X_24(F, x, t, ...)  F(x, t), VGC_PP_EXPAND(VGC_PP_TRANSFORM_X_23(F, x, __VA_ARGS__))
#define VGC_PP_TRANSFORM_X_25(F, x, t, ...)  F(x, t), VGC_PP_EXPAND(VGC_PP_TRANSFORM_X_24(F, x, __VA_ARGS__))
#define VGC_PP_TRANSFORM_X_26(F, x, t, ...)  F(x, t), VGC_PP_EXPAND(VGC_PP_TRANSFORM_X_25(F, x, __VA_ARGS__))
#define VGC_PP_TRANSFORM_X_27(F, x, t, ...)  F(x, t), VGC_PP_EXPAND(VGC_PP_TRANSFORM_X_26(F, x, __VA_ARGS__))
#define VGC_PP_TRANSFORM_X_28(F, x, t, ...)  F(x, t), VGC_PP_EXPAND(VGC_PP_TRANSFORM_X_27(F, x, __VA_ARGS__))
#define VGC_PP_TRANSFORM_X_29(F, x, t, ...)  F(x, t), VGC_PP_EXPAND(VGC_PP_TRANSFORM_X_28(F, x, __VA_ARGS__))
#define VGC_PP_TRANSFORM_X_30(F, x, t, ...)  F(x, t), VGC_PP_EXPAND(VGC_PP_TRANSFORM_X_29(F, x, __VA_ARGS__))
#define VGC_PP_TRANSFORM_X_31(F, x, t, ...)  F(x, t), VGC_PP_EXPAND(VGC_PP_TRANSFORM_X_30(F, x, __VA_ARGS__))
#define VGC_PP_TRANSFORM_X_32(F, x, t, ...)  F(x, t), VGC_PP_EXPAND(VGC_PP_TRANSFORM_X_31(F, x, __VA_ARGS__))
#define VGC_PP_TRANSFORM_X_33(F, x, t, ...)  F(x, t), VGC_PP_EXPAND(VGC_PP_TRANSFORM_X_32(F, x, __VA_ARGS__))
#define VGC_PP_TRANSFORM_X_34(F, x, t, ...)  F(x, t), VGC_PP_EXPAND(VGC_PP_TRANSFORM_X_33(F, x, __VA_ARGS__))
#define VGC_PP_TRANSFORM_X_35(F, x, t, ...)  F(x, t), VGC_PP_EXPAND(VGC_PP_TRANSFORM_X_34(F, x, __VA_ARGS__))
#define VGC_PP_TRANSFORM_X_36(F, x, t, ...)  F(x, t), VGC_PP_EXPAND(VGC_PP_TRANSFORM_X_35(F, x, __VA_ARGS__))
#define VGC_PP_TRANSFORM_X_37(F, x, t, ...)  F(x, t), VGC_PP_EXPAND(VGC_PP_TRANSFORM_X_36(F, x, __VA_ARGS__))
#define VGC_PP_TRANSFORM_X_38(F, x, t, ...)  F(x, t), VGC_PP_EXPAND(VGC_PP_TRANSFORM_X_37(F, x, __VA_ARGS__))
#define VGC_PP_TRANSFORM_X_39(F, x, t, ...)  F(x, t), VGC_PP_EXPAND(VGC_PP_TRANSFORM_X_38(F, x, __VA_ARGS__))
#define VGC_PP_TRANSFORM_X_40(F, x, t, ...)  F(x, t), VGC_PP_EXPAND(VGC_PP_TRANSFORM_X_39(F, x, __VA_ARGS__))
#define VGC_PP_TRANSFORM_X_41(F, x, t, ...)  F(x, t), VGC_PP_EXPAND(VGC_PP_TRANSFORM_X_40(F, x, __VA_ARGS__))
#define VGC_PP_TRANSFORM_X_42(F, x, t, ...)  F(x, t), VGC_PP_EXPAND(VGC_PP_TRANSFORM_X_41(F, x, __VA_ARGS__))
#define VGC_PP_TRANSFORM_X_43(F, x, t, ...)  F(x, t), VGC_PP_EXPAND(VGC_PP_TRANSFORM_X_42(F, x, __VA_ARGS__))
#define VGC_PP_TRANSFORM_X_44(F, x, t, ...)  F(x, t), VGC_PP_EXPAND(VGC_PP_TRANSFORM_X_43(F, x, __VA_ARGS__))
#define VGC_PP_TRANSFORM_X_45(F, x, t, ...)  F(x, t), VGC_PP_EXPAND(VGC_PP_TRANSFORM_X_44(F, x, __VA_ARGS__))
#define VGC_PP_TRANSFORM_X_46(F, x, t, ...)  F(x, t), VGC_PP_EXPAND(VGC_PP_TRANSFORM_X_45(F, x, __VA_ARGS__))
#define VGC_PP_TRANSFORM_X_47(F, x, t, ...)  F(x, t), VGC_PP_EXPAND(VGC_PP_TRANSFORM_X_46(F, x, __VA_ARGS__))
#define VGC_PP_TRANSFORM_X_48(F, x, t, ...)  F(x, t), VGC_PP_EXPAND(VGC_PP_TRANSFORM_X_47(F, x, __VA_ARGS__))
#define VGC_PP_TRANSFORM_X_49(F, x, t, ...)  F(x, t), VGC_PP_EXPAND(VGC_PP_TRANSFORM_X_48(F, x, __VA_ARGS__))
#define VGC_PP_TRANSFORM_X_50(F, x, t, ...)  F(x, t), VGC_PP_EXPAND(VGC_PP_TRANSFORM_X_49(F, x, __VA_ARGS__))
#define VGC_PP_TRANSFORM_X_51(F, x, t, ...)  F(x, t), VGC_PP_EXPAND(VGC_PP_TRANSFORM_X_50(F, x, __VA_ARGS__))
#define VGC_PP_TRANSFORM_X_52(F, x, t, ...)  F(x, t), VGC_PP_EXPAND(VGC_PP_TRANSFORM_X_51(F, x, __VA_ARGS__))
#define VGC_PP_TRANSFORM_X_53(F, x, t, ...)  F(x, t), VGC_PP_EXPAND(VGC_PP_TRANSFORM_X_52(F, x, __VA_ARGS__))
#define VGC_PP_TRANSFORM_X_54(F, x, t, ...)  F(x, t), VGC_PP_EXPAND(VGC_PP_TRANSFORM_X_53(F, x, __VA_ARGS__))
#define VGC_PP_TRANSFORM_X_55(F, x, t, ...)  F(x, t), VGC_PP_EXPAND(VGC_PP_TRANSFORM_X_54(F, x, __VA_ARGS__))
#define VGC_PP_TRANSFORM_X_56(F, x, t, ...)  F(x, t), VGC_PP_EXPAND(VGC_PP_TRANSFORM_X_55(F, x, __VA_ARGS__))
#define VGC_PP_TRANSFORM_X_57(F, x, t, ...)  F(x, t), VGC_PP_EXPAND(VGC_PP_TRANSFORM_X_56(F, x, __VA_ARGS__))
#define VGC_PP_TRANSFORM_X_58(F, x, t, ...)  F(x, t), VGC_PP_EXPAND(VGC_PP_TRANSFORM_X_57(F, x, __VA_ARGS__))
#define VGC_PP_TRANSFORM_X_59(F, x, t, ...)  F(x, t), VGC_PP_EXPAND(VGC_PP_TRANSFORM_X_58(F, x, __VA_ARGS__))
#define VGC_PP_TRANSFORM_X_60(F, x, t, ...)  F(x, t), VGC_PP_EXPAND(VGC_PP_TRANSFORM_X_49(F, x, __VA_ARGS__))
#define VGC_PP_TRANSFORM_X_61(F, x, t, ...)  F(x, t), VGC_PP_EXPAND(VGC_PP_TRANSFORM_X_60(F, x, __VA_ARGS__))
#define VGC_PP_TRANSFORM_X_62(F, x, t, ...)  F(x, t), VGC_PP_EXPAND(VGC_PP_TRANSFORM_X_61(F, x, __VA_ARGS__))
#define VGC_PP_TRANSFORM_X_63(F, x, t, ...)  F(x, t), VGC_PP_EXPAND(VGC_PP_TRANSFORM_X_62(F, x, __VA_ARGS__))
#define VGC_PP_TRANSFORM_X_64(F, x, t, ...)  F(x, t), VGC_PP_EXPAND(VGC_PP_TRANSFORM_X_63(F, x, __VA_ARGS__))
#define VGC_PP_TRANSFORM_X_65(F, x, t, ...)  F(x, t), VGC_PP_EXPAND(VGC_PP_TRANSFORM_X_64(F, x, __VA_ARGS__))
#define VGC_PP_TRANSFORM_X_66(F, x, t, ...)  F(x, t), VGC_PP_EXPAND(VGC_PP_TRANSFORM_X_65(F, x, __VA_ARGS__))
#define VGC_PP_TRANSFORM_X_67(F, x, t, ...)  F(x, t), VGC_PP_EXPAND(VGC_PP_TRANSFORM_X_66(F, x, __VA_ARGS__))
#define VGC_PP_TRANSFORM_X_68(F, x, t, ...)  F(x, t), VGC_PP_EXPAND(VGC_PP_TRANSFORM_X_67(F, x, __VA_ARGS__))
#define VGC_PP_TRANSFORM_X_69(F, x, t, ...)  F(x, t), VGC_PP_EXPAND(VGC_PP_TRANSFORM_X_68(F, x, __VA_ARGS__))
#define VGC_PP_TRANSFORM_X_70(F, x, t, ...)  F(x, t), VGC_PP_EXPAND(VGC_PP_TRANSFORM_X_69(F, x, __VA_ARGS__))
#define VGC_PP_TRANSFORM_X_71(F, x, t, ...)  F(x, t), VGC_PP_EXPAND(VGC_PP_TRANSFORM_X_70(F, x, __VA_ARGS__))
#define VGC_PP_TRANSFORM_X_72(F, x, t, ...)  F(x, t), VGC_PP_EXPAND(VGC_PP_TRANSFORM_X_71(F, x, __VA_ARGS__))
#define VGC_PP_TRANSFORM_X_73(F, x, t, ...)  F(x, t), VGC_PP_EXPAND(VGC_PP_TRANSFORM_X_72(F, x, __VA_ARGS__))
#define VGC_PP_TRANSFORM_X_74(F, x, t, ...)  F(x, t), VGC_PP_EXPAND(VGC_PP_TRANSFORM_X_73(F, x, __VA_ARGS__))
#define VGC_PP_TRANSFORM_X_75(F, x, t, ...)  F(x, t), VGC_PP_EXPAND(VGC_PP_TRANSFORM_X_74(F, x, __VA_ARGS__))
#define VGC_PP_TRANSFORM_X_76(F, x, t, ...)  F(x, t), VGC_PP_EXPAND(VGC_PP_TRANSFORM_X_75(F, x, __VA_ARGS__))
#define VGC_PP_TRANSFORM_X_77(F, x, t, ...)  F(x, t), VGC_PP_EXPAND(VGC_PP_TRANSFORM_X_76(F, x, __VA_ARGS__))
#define VGC_PP_TRANSFORM_X_78(F, x, t, ...)  F(x, t), VGC_PP_EXPAND(VGC_PP_TRANSFORM_X_77(F, x, __VA_ARGS__))
#define VGC_PP_TRANSFORM_X_79(F, x, t, ...)  F(x, t), VGC_PP_EXPAND(VGC_PP_TRANSFORM_X_78(F, x, __VA_ARGS__))
#define VGC_PP_TRANSFORM_X_80(F, x, t, ...)  F(x, t), VGC_PP_EXPAND(VGC_PP_TRANSFORM_X_79(F, x, __VA_ARGS__))
#define VGC_PP_TRANSFORM_X_81(F, x, t, ...)  F(x, t), VGC_PP_EXPAND(VGC_PP_TRANSFORM_X_80(F, x, __VA_ARGS__))
#define VGC_PP_TRANSFORM_X_82(F, x, t, ...)  F(x, t), VGC_PP_EXPAND(VGC_PP_TRANSFORM_X_81(F, x, __VA_ARGS__))
#define VGC_PP_TRANSFORM_X_83(F, x, t, ...)  F(x, t), VGC_PP_EXPAND(VGC_PP_TRANSFORM_X_82(F, x, __VA_ARGS__))
#define VGC_PP_TRANSFORM_X_84(F, x, t, ...)  F(x, t), VGC_PP_EXPAND(VGC_PP_TRANSFORM_X_83(F, x, __VA_ARGS__))
#define VGC_PP_TRANSFORM_X_85(F, x, t, ...)  F(x, t), VGC_PP_EXPAND(VGC_PP_TRANSFORM_X_84(F, x, __VA_ARGS__))
#define VGC_PP_TRANSFORM_X_86(F, x, t, ...)  F(x, t), VGC_PP_EXPAND(VGC_PP_TRANSFORM_X_85(F, x, __VA_ARGS__))
#define VGC_PP_TRANSFORM_X_87(F, x, t, ...)  F(x, t), VGC_PP_EXPAND(VGC_PP_TRANSFORM_X_86(F, x, __VA_ARGS__))
#define VGC_PP_TRANSFORM_X_88(F, x, t, ...)  F(x, t), VGC_PP_EXPAND(VGC_PP_TRANSFORM_X_87(F, x, __VA_ARGS__))
#define VGC_PP_TRANSFORM_X_89(F, x, t, ...)  F(x, t), VGC_PP_EXPAND(VGC_PP_TRANSFORM_X_88(F, x, __VA_ARGS__))
#define VGC_PP_TRANSFORM_X_90(F, x, t, ...)  F(x, t), VGC_PP_EXPAND(VGC_PP_TRANSFORM_X_89(F, x, __VA_ARGS__))
#define VGC_PP_TRANSFORM_X_91(F, x, t, ...)  F(x, t), VGC_PP_EXPAND(VGC_PP_TRANSFORM_X_90(F, x, __VA_ARGS__))
#define VGC_PP_TRANSFORM_X_92(F, x, t, ...)  F(x, t), VGC_PP_EXPAND(VGC_PP_TRANSFORM_X_91(F, x, __VA_ARGS__))
#define VGC_PP_TRANSFORM_X_93(F, x, t, ...)  F(x, t), VGC_PP_EXPAND(VGC_PP_TRANSFORM_X_92(F, x, __VA_ARGS__))
#define VGC_PP_TRANSFORM_X_94(F, x, t, ...)  F(x, t), VGC_PP_EXPAND(VGC_PP_TRANSFORM_X_93(F, x, __VA_ARGS__))
#define VGC_PP_TRANSFORM_X_95(F, x, t, ...)  F(x, t), VGC_PP_EXPAND(VGC_PP_TRANSFORM_X_94(F, x, __VA_ARGS__))
#define VGC_PP_TRANSFORM_X_96(F, x, t, ...)  F(x, t), VGC_PP_EXPAND(VGC_PP_TRANSFORM_X_95(F, x, __VA_ARGS__))
#define VGC_PP_TRANSFORM_X_97(F, x, t, ...)  F(x, t), VGC_PP_EXPAND(VGC_PP_TRANSFORM_X_96(F, x, __VA_ARGS__))
#define VGC_PP_TRANSFORM_X_98(F, x, t, ...)  F(x, t), VGC_PP_EXPAND(VGC_PP_TRANSFORM_X_97(F, x, __VA_ARGS__))
#define VGC_PP_TRANSFORM_X_99(F, x, t, ...)  F(x, t), VGC_PP_EXPAND(VGC_PP_TRANSFORM_X_98(F, x, __VA_ARGS__))
#define VGC_PP_TRANSFORM_X_100(F, x, t, ...) F(x, t), VGC_PP_EXPAND(VGC_PP_TRANSFORM_X_99(F, x, __VA_ARGS__))

/// The macro `VGC_NAMESPACE(ns1, ns2, ...)` expands to `ns1::ns2::...`.
///
/// Example:
///
/// ```cpp
/// namespace VGC_NAMESPACE(vgc, ui) {
///     struct Foo {};
/// }
/// ```
///
/// Preprocessor output:
///
/// ```cpp
/// namespace vgc::ui {
///     struct Foo {};
/// }
/// ```
///
#define VGC_NAMESPACE_X(...) VGC_PP_EXPAND(VGC_PP_OVERLOAD(VGC_NAMESPACE_X_,__VA_ARGS__)(__VA_ARGS__))
#define VGC_NAMESPACE(...) VGC_NAMESPACE_X(__VA_ARGS__,~)

#define VGC_NAMESPACE_X_1(_)
#define VGC_NAMESPACE_X_2(x, ...) x
#define VGC_NAMESPACE_X_3(x, ...) x::VGC_PP_EXPAND(VGC_NAMESPACE_X_2(__VA_ARGS__))
#define VGC_NAMESPACE_X_4(x, ...) x::VGC_PP_EXPAND(VGC_NAMESPACE_X_3(__VA_ARGS__))
#define VGC_NAMESPACE_X_5(x, ...) x::VGC_PP_EXPAND(VGC_NAMESPACE_X_4(__VA_ARGS__))
#define VGC_NAMESPACE_X_6(x, ...) x::VGC_PP_EXPAND(VGC_NAMESPACE_X_5(__VA_ARGS__))
#define VGC_NAMESPACE_X_7(x, ...) x::VGC_PP_EXPAND(VGC_NAMESPACE_X_6(__VA_ARGS__))
#define VGC_NAMESPACE_X_8(x, ...) x::VGC_PP_EXPAND(VGC_NAMESPACE_X_7(__VA_ARGS__))
#define VGC_NAMESPACE_X_9(x, ...) x::VGC_PP_EXPAND(VGC_NAMESPACE_X_8(__VA_ARGS__))

// clang-format on

#endif // VGC_CORE_PREPROCESSOR_H
