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

#define VGC_PP_STR(x) #x
#define VGC_PP_XSTR(x) VGC_PP_STR(x)

#define VGC_PP_CONCAT(x, y) x##y
#define VGC_PP_XCONCAT(x, y) VGC_PP_CONCAT(x,y)

#define VGC_PP_EXPAND(x) x
#define VGC_PP_EXPAND2(x) VGC_PP_EXPAND(x)

#define VGC_PP_FIRST_(a, ...) a
#define VGC_PP_SUBLIST_1_END_(_0, ...) __VA_ARGS__
#define VGC_PP_SUBLIST_2_END_(_0, _1, ...) __VA_ARGS__

// Bits of VGC_PP_TRIM_VAEND.
#define VGC_PP_TVE_0_(_)
#define VGC_PP_TVE_1_(x, _) x
#define VGC_PP_TVE_2_(x, ...) x, VGC_PP_EXPAND(VGC_PP_TVE_1_(__VA_ARGS__))
#define VGC_PP_TVE_3_(x, ...) x, VGC_PP_EXPAND(VGC_PP_TVE_2_(__VA_ARGS__))
#define VGC_PP_TVE_4_(x, ...) x, VGC_PP_EXPAND(VGC_PP_TVE_3_(__VA_ARGS__))
#define VGC_PP_TVE_5_(x, ...) x, VGC_PP_EXPAND(VGC_PP_TVE_4_(__VA_ARGS__))
#define VGC_PP_TVE_6_(x, ...) x, VGC_PP_EXPAND(VGC_PP_TVE_5_(__VA_ARGS__))
#define VGC_PP_TVE_7_(x, ...) x, VGC_PP_EXPAND(VGC_PP_TVE_6_(__VA_ARGS__))
#define VGC_PP_TVE_8_(x, ...) x, VGC_PP_EXPAND(VGC_PP_TVE_7_(__VA_ARGS__))
#define VGC_PP_TVE_9_(x, ...) x, VGC_PP_EXPAND(VGC_PP_TVE_8_(__VA_ARGS__))
#define VGC_PP_TVE_DISPATCH_(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, S, ...) S

/// Helper macro to trim VaEnd from __VA_ARGS__.
/// Input list must end with VaEnd.
///
#define VGC_PP_TRIM_VAEND(...)                                                           \
    VGC_PP_EXPAND(VGC_PP_TVE_DISPATCH_(                                                  \
        __VA_ARGS__,                                                                     \
        VGC_PP_TVE_9_,                                                                   \
        VGC_PP_TVE_8_,                                                                   \
        VGC_PP_TVE_7_,                                                                   \
        VGC_PP_TVE_6_,                                                                   \
        VGC_PP_TVE_5_,                                                                   \
        VGC_PP_TVE_4_,                                                                   \
        VGC_PP_TVE_3_,                                                                   \
        VGC_PP_TVE_2_,                                                                   \
        VGC_PP_TVE_1_,                                                                   \
        VGC_PP_TVE_0_)(__VA_ARGS__))

// Bits of VGC_TRANSFORM_.
// XXX check that the VGC_EXPAND fix for msvc works with all F
//
#define VGC_PP_TF_0_(F, _) VaEnd
#define VGC_PP_TF_1_(F, x, ...) F(x), VaEnd
#define VGC_PP_TF_2_(F, x, ...) F(x), VGC_PP_EXPAND(VGC_PP_TF_1_(F, __VA_ARGS__))
#define VGC_PP_TF_3_(F, x, ...) F(x), VGC_PP_EXPAND(VGC_PP_TF_2_(F, __VA_ARGS__))
#define VGC_PP_TF_4_(F, x, ...) F(x), VGC_PP_EXPAND(VGC_PP_TF_3_(F, __VA_ARGS__))
#define VGC_PP_TF_5_(F, x, ...) F(x), VGC_PP_EXPAND(VGC_PP_TF_4_(F, __VA_ARGS__))
#define VGC_PP_TF_6_(F, x, ...) F(x), VGC_PP_EXPAND(VGC_PP_TF_5_(F, __VA_ARGS__))
#define VGC_PP_TF_7_(F, x, ...) F(x), VGC_PP_EXPAND(VGC_PP_TF_6_(F, __VA_ARGS__))
#define VGC_PP_TF_8_(F, x, ...) F(x), VGC_PP_EXPAND(VGC_PP_TF_7_(F, __VA_ARGS__))
#define VGC_PP_TF_9_(F, x, ...) F(x), VGC_PP_EXPAND(VGC_PP_TF_8_(F, __VA_ARGS__))
#define VGC_PP_TF_DISPATCH_(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, S, ...) S

/// Macro to transform a list of token. Applies F on each token except VaEnd.
/// (t0, t1, .., t3, VaEnd) -> (F(t0), F(t1), .., F(t3), VaEnd).
/// Input list must end with VaEnd.
///
#define VGC_PP_TRANSFORM(F, ...)                                                         \
    VGC_PP_EXPAND(VGC_PP_TF_DISPATCH_(                                                   \
        __VA_ARGS__,                                                                     \
        VGC_PP_TF_9_,                                                                    \
        VGC_PP_TF_8_,                                                                    \
        VGC_PP_TF_7_,                                                                    \
        VGC_PP_TF_6_,                                                                    \
        VGC_PP_TF_5_,                                                                    \
        VGC_PP_TF_4_,                                                                    \
        VGC_PP_TF_3_,                                                                    \
        VGC_PP_TF_2_,                                                                    \
        VGC_PP_TF_1_,                                                                    \
        VGC_PP_TF_0_)(F, __VA_ARGS__))

/// Returns the number of arguments of a macro.
///
#define VGC_PP_NUM_ARGS(...)                                                             \
    VGC_PP_EXPAND(VGC_PP_NUM_ARGS_DISPATCH(__VA_ARGS__, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1))
#define VGC_PP_NUM_ARGS_DISPATCH(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, S, ...) S

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
#define VGC_PP_OVERLOAD(prefix, ...) VGC_PP_XCONCAT(prefix, VGC_PP_NUM_ARGS(__VA_ARGS__))

#endif // VGC_CORE_PREPROCESSOR_H
