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
/// ```cpp
/// #define ADD(x, y) x + y
///
/// const char* a = "ADD(1, 2)";           // == "ADD(1, 2)"
/// const char* b = VGC_PP_STR(ADD(1, 2)); // == "1 + 2"
/// ```
///
#define VGC_PP_STR(x) VGC_PP_STR_(x)
#define VGC_PP_STR_(x) #x

/// Concatenates the two given arguments after performing macro expansion.
///
/// ```cpp
/// #define ANSWER() 42
/// #define CAT_ANSWER_1(x) x ## ANSWER()
/// #define CAT_ANSWER_2(x) VGC_PP_CAT(x, ANSWER())
///
/// const char* a = CAT_ANSWER_1(1);  // Expands to `1ANSWER()` => compile error
/// const char* b = CAT_ANSWER_2(1);  // Expands to `142`       => ok
/// ```
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
/// #define APPLY_OP_1(op, ...) op(__VA_ARGS__)
/// #define APPLY_OP_2(op, ...) VGC_PP_EXPAND(op(__VA_ARGS__))
///
/// int a = APPLY_OP_1(ADD, 1, 2); // Expands to `1, 2 +` => compile error
/// int b = APPLY_OP_2(ADD, 1, 2); // Expands to `1 + 2`  => ok
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

/// Returns the number of arguments of a macro. This only works up to 125
/// arguments, due to compiler limits. Also, calling this function with zero
/// arguments is undefined (in most compilers, it would return 1)
///
/// Note that you must not call this function, since variadic macros with
/// zero arguments are not allowed (until C++20).
///
#define VGC_PP_NUM_ARGS(...)                                                             \
    VGC_PP_EXPAND(VGC_PP_NUM_ARGS_DISPATCH(                                              \
        __VA_ARGS__,        125, 124, 123, 122, 121, 120,                                \
        119, 118, 117, 116, 115, 114, 113, 112, 111, 110,                                \
        109, 108, 107, 106, 105, 104, 103, 102, 101, 100,                                \
         99,  98,  97,  96,  95,  94,  93,  92,  91,  90,                                \
         89,  88,  87,  86,  85,  84,  83,  82,  81,  80,                                \
         79,  78,  77,  76,  75,  74,  73,  72,  71,  70,                                \
         69,  68,  67,  66,  65,  64,  63,  62,  61,  60,                                \
         59,  58,  57,  56,  55,  54,  53,  52,  51,  50,                                \
         49,  48,  47,  46,  45,  44,  43,  42,  41,  40,                                \
         39,  38,  37,  36,  35,  34,  33,  32,  31,  30,                                \
         29,  28,  27,  26,  25,  24,  23,  22,  21,  20,                                \
         19,  18,  17,  16,  15,  14,  13,  12,  11,  10,                                \
          9,   8,   7,   6,   5,   4,   3,   2,   1))

// Note: MSVC only supports up to 127 macro arguments, so this macro can't be
// any bigger. Note that "..." counts as 1 argument for this compiler limit.
//
#define VGC_PP_NUM_ARGS_DISPATCH(                                                        \
    _000, _001, _002, _003, _004, _005, _006, _007, _008, _009,                          \
    _010, _011, _012, _013, _014, _015, _016, _017, _018, _019,                          \
    _020, _021, _022, _023, _024, _025, _026, _027, _028, _029,                          \
    _030, _031, _032, _033, _034, _035, _036, _037, _038, _039,                          \
    _040, _041, _042, _043, _044, _045, _046, _047, _048, _049,                          \
    _050, _051, _052, _053, _054, _055, _056, _057, _058, _059,                          \
    _060, _061, _062, _063, _064, _065, _066, _067, _068, _069,                          \
    _070, _071, _072, _073, _074, _075, _076, _077, _078, _079,                          \
    _080, _081, _082, _083, _084, _085, _086, _087, _088, _089,                          \
    _090, _091, _092, _093, _094, _095, _096, _097, _098, _099,                          \
    _100, _101, _102, _103, _104, _105, _106, _107, _108, _109,                          \
    _110, _111, _112, _113, _114, _115, _116, _117, _118, _119,                          \
    _120, _121, _122, _123, _124, S, ...) S

/// Allows to define macro overloads based on the number of arguments.
///
/// ```cpp
/// #define MIN_1(x) x
/// #define MIN_2(x, y) ((x) < (y) ? (x) : (y))
/// #define MIN_3(x, y, z) ((x) < (y) ? MIN_2(x, z) : MIN_2(y, z))
/// #define MIN(...) VGC_PP_EXPAND(VGC_PP_OVERLOAD(MIN_, __VA_ARGS__)(__VA_ARGS__))
///
/// int a = MIN(42);         // 42
/// int a = MIN(42, 10);     // 10
/// int a = MIN(42, 10, 25); // 10
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
/// Note that due to compiler limits, this works only up to 122 variadic
/// arguments (t1, ..., t122).
///
#define VGC_PP_FOREACH_X(...) VGC_PP_EXPAND(VGC_PP_OVERLOAD(VGC_PP_FOREACH_X_, __VA_ARGS__)(__VA_ARGS__))
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
#define VGC_PP_FOREACH_X_60(F, x, t, ...)  F(x, t) VGC_PP_EXPAND(VGC_PP_FOREACH_X_59(F, x, __VA_ARGS__))
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
#define VGC_PP_FOREACH_X_101(F, x, t, ...)  F(x, t) VGC_PP_EXPAND(VGC_PP_FOREACH_X_100(F, x, __VA_ARGS__))
#define VGC_PP_FOREACH_X_102(F, x, t, ...)  F(x, t) VGC_PP_EXPAND(VGC_PP_FOREACH_X_101(F, x, __VA_ARGS__))
#define VGC_PP_FOREACH_X_103(F, x, t, ...)  F(x, t) VGC_PP_EXPAND(VGC_PP_FOREACH_X_102(F, x, __VA_ARGS__))
#define VGC_PP_FOREACH_X_104(F, x, t, ...)  F(x, t) VGC_PP_EXPAND(VGC_PP_FOREACH_X_103(F, x, __VA_ARGS__))
#define VGC_PP_FOREACH_X_105(F, x, t, ...)  F(x, t) VGC_PP_EXPAND(VGC_PP_FOREACH_X_104(F, x, __VA_ARGS__))
#define VGC_PP_FOREACH_X_106(F, x, t, ...)  F(x, t) VGC_PP_EXPAND(VGC_PP_FOREACH_X_105(F, x, __VA_ARGS__))
#define VGC_PP_FOREACH_X_107(F, x, t, ...)  F(x, t) VGC_PP_EXPAND(VGC_PP_FOREACH_X_106(F, x, __VA_ARGS__))
#define VGC_PP_FOREACH_X_108(F, x, t, ...)  F(x, t) VGC_PP_EXPAND(VGC_PP_FOREACH_X_107(F, x, __VA_ARGS__))
#define VGC_PP_FOREACH_X_109(F, x, t, ...)  F(x, t) VGC_PP_EXPAND(VGC_PP_FOREACH_X_108(F, x, __VA_ARGS__))
#define VGC_PP_FOREACH_X_110(F, x, t, ...)  F(x, t) VGC_PP_EXPAND(VGC_PP_FOREACH_X_109(F, x, __VA_ARGS__))
#define VGC_PP_FOREACH_X_111(F, x, t, ...)  F(x, t) VGC_PP_EXPAND(VGC_PP_FOREACH_X_110(F, x, __VA_ARGS__))
#define VGC_PP_FOREACH_X_112(F, x, t, ...)  F(x, t) VGC_PP_EXPAND(VGC_PP_FOREACH_X_111(F, x, __VA_ARGS__))
#define VGC_PP_FOREACH_X_113(F, x, t, ...)  F(x, t) VGC_PP_EXPAND(VGC_PP_FOREACH_X_112(F, x, __VA_ARGS__))
#define VGC_PP_FOREACH_X_114(F, x, t, ...)  F(x, t) VGC_PP_EXPAND(VGC_PP_FOREACH_X_113(F, x, __VA_ARGS__))
#define VGC_PP_FOREACH_X_115(F, x, t, ...)  F(x, t) VGC_PP_EXPAND(VGC_PP_FOREACH_X_114(F, x, __VA_ARGS__))
#define VGC_PP_FOREACH_X_116(F, x, t, ...)  F(x, t) VGC_PP_EXPAND(VGC_PP_FOREACH_X_115(F, x, __VA_ARGS__))
#define VGC_PP_FOREACH_X_117(F, x, t, ...)  F(x, t) VGC_PP_EXPAND(VGC_PP_FOREACH_X_116(F, x, __VA_ARGS__))
#define VGC_PP_FOREACH_X_118(F, x, t, ...)  F(x, t) VGC_PP_EXPAND(VGC_PP_FOREACH_X_117(F, x, __VA_ARGS__))
#define VGC_PP_FOREACH_X_119(F, x, t, ...)  F(x, t) VGC_PP_EXPAND(VGC_PP_FOREACH_X_118(F, x, __VA_ARGS__))
#define VGC_PP_FOREACH_X_120(F, x, t, ...)  F(x, t) VGC_PP_EXPAND(VGC_PP_FOREACH_X_119(F, x, __VA_ARGS__))
#define VGC_PP_FOREACH_X_121(F, x, t, ...)  F(x, t) VGC_PP_EXPAND(VGC_PP_FOREACH_X_120(F, x, __VA_ARGS__))
#define VGC_PP_FOREACH_X_122(F, x, t, ...)  F(x, t) VGC_PP_EXPAND(VGC_PP_FOREACH_X_121(F, x, __VA_ARGS__))
#define VGC_PP_FOREACH_X_123(F, x, t, ...)  F(x, t) VGC_PP_EXPAND(VGC_PP_FOREACH_X_122(F, x, __VA_ARGS__))
#define VGC_PP_FOREACH_X_124(F, x, t, ...)  F(x, t) VGC_PP_EXPAND(VGC_PP_FOREACH_X_123(F, x, __VA_ARGS__))
#define VGC_PP_FOREACH_X_125(F, x, t, ...)  F(x, t) VGC_PP_EXPAND(VGC_PP_FOREACH_X_124(F, x, __VA_ARGS__))

/// The macro `VGC_PP_TRANSFORM(F, x, t1, t2, ...)` expands to `F(x, t1), F(x, t2) ...`.
///
#define VGC_PP_TRANSFORM_X(...) VGC_PP_EXPAND(VGC_PP_OVERLOAD(VGC_PP_TRANSFORM_X_, __VA_ARGS__)(__VA_ARGS__))
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
#define VGC_PP_TRANSFORM_X_60(F, x, t, ...)  F(x, t), VGC_PP_EXPAND(VGC_PP_TRANSFORM_X_59(F, x, __VA_ARGS__))
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
#define VGC_PP_TRANSFORM_X_101(F, x, t, ...)  F(x, t) VGC_PP_EXPAND(VGC_PP_TRANSFORM_X_100(F, x, __VA_ARGS__))
#define VGC_PP_TRANSFORM_X_102(F, x, t, ...)  F(x, t) VGC_PP_EXPAND(VGC_PP_TRANSFORM_X_101(F, x, __VA_ARGS__))
#define VGC_PP_TRANSFORM_X_103(F, x, t, ...)  F(x, t) VGC_PP_EXPAND(VGC_PP_TRANSFORM_X_102(F, x, __VA_ARGS__))
#define VGC_PP_TRANSFORM_X_104(F, x, t, ...)  F(x, t) VGC_PP_EXPAND(VGC_PP_TRANSFORM_X_103(F, x, __VA_ARGS__))
#define VGC_PP_TRANSFORM_X_105(F, x, t, ...)  F(x, t) VGC_PP_EXPAND(VGC_PP_TRANSFORM_X_104(F, x, __VA_ARGS__))
#define VGC_PP_TRANSFORM_X_106(F, x, t, ...)  F(x, t) VGC_PP_EXPAND(VGC_PP_TRANSFORM_X_105(F, x, __VA_ARGS__))
#define VGC_PP_TRANSFORM_X_107(F, x, t, ...)  F(x, t) VGC_PP_EXPAND(VGC_PP_TRANSFORM_X_106(F, x, __VA_ARGS__))
#define VGC_PP_TRANSFORM_X_108(F, x, t, ...)  F(x, t) VGC_PP_EXPAND(VGC_PP_TRANSFORM_X_107(F, x, __VA_ARGS__))
#define VGC_PP_TRANSFORM_X_109(F, x, t, ...)  F(x, t) VGC_PP_EXPAND(VGC_PP_TRANSFORM_X_108(F, x, __VA_ARGS__))
#define VGC_PP_TRANSFORM_X_110(F, x, t, ...)  F(x, t) VGC_PP_EXPAND(VGC_PP_TRANSFORM_X_109(F, x, __VA_ARGS__))
#define VGC_PP_TRANSFORM_X_111(F, x, t, ...)  F(x, t) VGC_PP_EXPAND(VGC_PP_TRANSFORM_X_110(F, x, __VA_ARGS__))
#define VGC_PP_TRANSFORM_X_112(F, x, t, ...)  F(x, t) VGC_PP_EXPAND(VGC_PP_TRANSFORM_X_111(F, x, __VA_ARGS__))
#define VGC_PP_TRANSFORM_X_113(F, x, t, ...)  F(x, t) VGC_PP_EXPAND(VGC_PP_TRANSFORM_X_112(F, x, __VA_ARGS__))
#define VGC_PP_TRANSFORM_X_114(F, x, t, ...)  F(x, t) VGC_PP_EXPAND(VGC_PP_TRANSFORM_X_113(F, x, __VA_ARGS__))
#define VGC_PP_TRANSFORM_X_115(F, x, t, ...)  F(x, t) VGC_PP_EXPAND(VGC_PP_TRANSFORM_X_114(F, x, __VA_ARGS__))
#define VGC_PP_TRANSFORM_X_116(F, x, t, ...)  F(x, t) VGC_PP_EXPAND(VGC_PP_TRANSFORM_X_115(F, x, __VA_ARGS__))
#define VGC_PP_TRANSFORM_X_117(F, x, t, ...)  F(x, t) VGC_PP_EXPAND(VGC_PP_TRANSFORM_X_116(F, x, __VA_ARGS__))
#define VGC_PP_TRANSFORM_X_118(F, x, t, ...)  F(x, t) VGC_PP_EXPAND(VGC_PP_TRANSFORM_X_117(F, x, __VA_ARGS__))
#define VGC_PP_TRANSFORM_X_119(F, x, t, ...)  F(x, t) VGC_PP_EXPAND(VGC_PP_TRANSFORM_X_118(F, x, __VA_ARGS__))
#define VGC_PP_TRANSFORM_X_120(F, x, t, ...)  F(x, t) VGC_PP_EXPAND(VGC_PP_TRANSFORM_X_119(F, x, __VA_ARGS__))
#define VGC_PP_TRANSFORM_X_121(F, x, t, ...)  F(x, t) VGC_PP_EXPAND(VGC_PP_TRANSFORM_X_120(F, x, __VA_ARGS__))
#define VGC_PP_TRANSFORM_X_122(F, x, t, ...)  F(x, t) VGC_PP_EXPAND(VGC_PP_TRANSFORM_X_121(F, x, __VA_ARGS__))
#define VGC_PP_TRANSFORM_X_123(F, x, t, ...)  F(x, t) VGC_PP_EXPAND(VGC_PP_TRANSFORM_X_122(F, x, __VA_ARGS__))
#define VGC_PP_TRANSFORM_X_124(F, x, t, ...)  F(x, t) VGC_PP_EXPAND(VGC_PP_TRANSFORM_X_123(F, x, __VA_ARGS__))
#define VGC_PP_TRANSFORM_X_125(F, x, t, ...)  F(x, t) VGC_PP_EXPAND(VGC_PP_TRANSFORM_X_124(F, x, __VA_ARGS__))

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
#define VGC_NAMESPACE_X(...) VGC_PP_EXPAND(VGC_PP_OVERLOAD(VGC_NAMESPACE_X_, __VA_ARGS__)(__VA_ARGS__))
#define VGC_NAMESPACE(...) VGC_NAMESPACE_X(__VA_ARGS__, ~)

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
