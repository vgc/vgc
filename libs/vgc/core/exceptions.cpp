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

#include <vgc/core/exceptions.h>
#include <vgc/core/format.h>

namespace vgc::core {

namespace detail {

std::string notAliveMsg(const Object* object) {
    return core::format("Object {} is not alive", core::asAddress(object));
}

std::string notAChildMsg(const Object* object, const Object* expectedParent) {
    return core::format(
        "Object {} is not a child of {}",
        core::asAddress(object),
        core::asAddress(expectedParent));
}

} // namespace detail

VGC_CORE_EXCEPTIONS_DEFINE_ANCHOR(LogicError)
VGC_CORE_EXCEPTIONS_DEFINE_ANCHOR(NegativeIntegerError)
VGC_CORE_EXCEPTIONS_DEFINE_ANCHOR(IndexError)
VGC_CORE_EXCEPTIONS_DEFINE_ANCHOR(LengthError)
VGC_CORE_EXCEPTIONS_DEFINE_ANCHOR(NullError)
VGC_CORE_EXCEPTIONS_DEFINE_ANCHOR(NotAliveError)
VGC_CORE_EXCEPTIONS_DEFINE_ANCHOR(NotAChildError)
VGC_CORE_EXCEPTIONS_DEFINE_ANCHOR(RuntimeError)
VGC_CORE_EXCEPTIONS_DEFINE_ANCHOR(ParseError)
VGC_CORE_EXCEPTIONS_DEFINE_ANCHOR(RangeError)
VGC_CORE_EXCEPTIONS_DEFINE_ANCHOR(IntegerOverflowError)
VGC_CORE_EXCEPTIONS_DEFINE_ANCHOR(FileError)

} // namespace vgc::core
