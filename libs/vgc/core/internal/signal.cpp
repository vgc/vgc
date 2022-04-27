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

#include <vgc/core/object.h>

namespace vgc::core::internal {

ConnectionHandle ConnectionHandle::generate() {
    static ConnectionHandle s = {0};
    // XXX make this thread-safe ?
    return {++s.id_};
}

VGC_CORE_API
FunctionId genFunctionId() {
    static FunctionId s = 0;
    // XXX make this thread-safe ?
    return ++s;
}

// Checking that AnySignalArg::IsMakeableFrom is working as expected.
// The goal is for AnySignalArg to not accept temporaries created on construction.
//
struct A_ {};
struct B_ : A_ {};
// upcasts are allowed
static_assert( AnySignalArg::isMakeableFrom<      A_  , B_&>);
static_assert( AnySignalArg::isMakeableFrom<      A_& , B_&>);
static_assert( AnySignalArg::isMakeableFrom<const A_& , B_&>);
static_assert(!AnySignalArg::isMakeableFrom<      A_  , const B_&>);
static_assert(!AnySignalArg::isMakeableFrom<      A_& , const B_&>);
static_assert( AnySignalArg::isMakeableFrom<const A_& , const B_&>);
static_assert(!AnySignalArg::isMakeableFrom<      A_  , B_&&>);
static_assert(!AnySignalArg::isMakeableFrom<      A_& , B_&&>);
static_assert(!AnySignalArg::isMakeableFrom<const A_& , B_&&>);
static_assert(!AnySignalArg::isMakeableFrom<      A_  , const B_&&>);
static_assert(!AnySignalArg::isMakeableFrom<      A_& , const B_&&>);
static_assert(!AnySignalArg::isMakeableFrom<const A_& , const B_&&>);
// copies are not allowed
static_assert(!AnySignalArg::isMakeableFrom<      int  , float&>);
static_assert(!AnySignalArg::isMakeableFrom<      int& , float&>);
static_assert(!AnySignalArg::isMakeableFrom<const int& , float&>);
static_assert(!AnySignalArg::isMakeableFrom<      int  , const float&>);
static_assert(!AnySignalArg::isMakeableFrom<      int& , const float&>);
static_assert(!AnySignalArg::isMakeableFrom<const int& , const float&>);
static_assert(!AnySignalArg::isMakeableFrom<      int  , float&&>);
static_assert(!AnySignalArg::isMakeableFrom<      int& , float&&>);
static_assert(!AnySignalArg::isMakeableFrom<const int& , float&&>);
static_assert(!AnySignalArg::isMakeableFrom<      int  , const float&&>);
static_assert(!AnySignalArg::isMakeableFrom<      int& , const float&&>);
static_assert(!AnySignalArg::isMakeableFrom<const int& , const float&&>);

} // namespace vgc::core::internal
