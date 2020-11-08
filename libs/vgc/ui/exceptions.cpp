// Copyright 2020 The VGC Developers
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

#include <vgc/ui/exceptions.h>

#include <vgc/core/format.h>
#include <vgc/ui/widget.h>

namespace vgc {
namespace ui {

namespace internal {

std::string childCycleMsg(const Widget* parent, const Widget* child)
{
    return core::format(
        "Widget {} cannot be a child of Widget {}"
        " because the latter is a descendant of the former",
        core::toAddressString(child),
        core::toAddressString(parent));
}

} // namespace internal

VGC_CORE_EXCEPTIONS_DEFINE_ANCHOR(LogicError)
VGC_CORE_EXCEPTIONS_DEFINE_ANCHOR(ChildCycleError)
VGC_CORE_EXCEPTIONS_DEFINE_ANCHOR(RuntimeError)

} // namespace ui
} // namespace vgc
