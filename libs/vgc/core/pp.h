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

#ifndef VGC_CORE_PP_H
#define VGC_CORE_PP_H

#define VGC_STR(x) #x
#define VGC_XSTR(x) VGC_STR(x)

#define VGC_CONCAT(x, y) x##y
#define VGC_XCONCAT(x, y) VGC_CONCAT(x,y)

#define VGC_EXPAND(x) x
#define VGC_EXPAND2(x) VGC_EXPAND(x)

#define VGC_FIRST_(a, ...) a
#define VGC_SUBLIST_1_END_(_0,...) __VA_ARGS__
#define VGC_SUBLIST_2_END_(_0,_1,...) __VA_ARGS__

#define VGC_TVE_0_(_)
#define VGC_TVE_1_(x, _)   x
#define VGC_TVE_2_(x, ...) x, VGC_EXPAND(VGC_TVE_1_(__VA_ARGS__))
#define VGC_TVE_3_(x, ...) x, VGC_EXPAND(VGC_TVE_2_(__VA_ARGS__))
#define VGC_TVE_4_(x, ...) x, VGC_EXPAND(VGC_TVE_3_(__VA_ARGS__))
#define VGC_TVE_5_(x, ...) x, VGC_EXPAND(VGC_TVE_4_(__VA_ARGS__))
#define VGC_TVE_6_(x, ...) x, VGC_EXPAND(VGC_TVE_5_(__VA_ARGS__))
#define VGC_TVE_7_(x, ...) x, VGC_EXPAND(VGC_TVE_6_(__VA_ARGS__))
#define VGC_TVE_8_(x, ...) x, VGC_EXPAND(VGC_TVE_7_(__VA_ARGS__))
#define VGC_TVE_9_(x, ...) x, VGC_EXPAND(VGC_TVE_8_(__VA_ARGS__))
#define VGC_TVE_DISPATCH_(_0,_1,_2,_3,_4,_5,_6,_7,_8,_9,S,...) S
// ... must end with VaEnd
#define VGC_TRIM_VAEND_(...)                                    \
  VGC_EXPAND(VGC_TVE_DISPATCH_(__VA_ARGS__,                     \
    VGC_TVE_9_,VGC_TVE_8_,VGC_TVE_7_,VGC_TVE_6_,VGC_TVE_5_,     \
    VGC_TVE_4_,VGC_TVE_3_,VGC_TVE_2_,VGC_TVE_1_,VGC_TVE_0_      \
  )(__VA_ARGS__))

// XXX check that the VGC_EXPAND fix for msvc works with all F
#define VGC_TF_0_(F, _)      VaEnd
#define VGC_TF_1_(F, x, ...) F(x), VaEnd
#define VGC_TF_2_(F, x, ...) F(x), VGC_EXPAND(VGC_TF_1_(F, __VA_ARGS__))
#define VGC_TF_3_(F, x, ...) F(x), VGC_EXPAND(VGC_TF_2_(F, __VA_ARGS__))
#define VGC_TF_4_(F, x, ...) F(x), VGC_EXPAND(VGC_TF_3_(F, __VA_ARGS__))
#define VGC_TF_5_(F, x, ...) F(x), VGC_EXPAND(VGC_TF_4_(F, __VA_ARGS__))
#define VGC_TF_6_(F, x, ...) F(x), VGC_EXPAND(VGC_TF_5_(F, __VA_ARGS__))
#define VGC_TF_7_(F, x, ...) F(x), VGC_EXPAND(VGC_TF_6_(F, __VA_ARGS__))
#define VGC_TF_8_(F, x, ...) F(x), VGC_EXPAND(VGC_TF_7_(F, __VA_ARGS__))
#define VGC_TF_9_(F, x, ...) F(x), VGC_EXPAND(VGC_TF_8_(F, __VA_ARGS__))
#define VGC_TF_DISPATCH_(_0,_1,_2,_3,_4,_5,_6,_7,_8,_9,S,...) S
// ... must end with VaEnd
#define VGC_TRANSFORM_(F, ...)                              \
  VGC_EXPAND(VGC_TF_DISPATCH_(__VA_ARGS__,                  \
    VGC_TF_9_,VGC_TF_8_,VGC_TF_7_,VGC_TF_6_,VGC_TF_5_,      \
    VGC_TF_4_,VGC_TF_3_,VGC_TF_2_,VGC_TF_1_,VGC_TF_0_       \
  )(F, __VA_ARGS__))

#endif // VGC_CORE_PP_H
