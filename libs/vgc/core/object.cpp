// Copyright 2018 The VGC Developers
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

namespace vgc {
namespace core {

/* static */
ObjectSharedPtr Object::create()
{
    // Provide access to protected constructor. See:
    // - https://stackoverflow.com/a/11344174/1951907
    // - https://groups.google.com/a/isocpp.org/forum/#!topic/std-proposals/YXyGFUq7Wa4
    struct A : std::allocator<Object> {
        void construct(void* p) { ::new(p) Object(); }
        void destroy(Object* p) { p->~Object(); }
    };
    return std::allocate_shared<Object>(A());
}

} // namespace core
} // namespace vgc
