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

#include <vgc/core/object.h>

#ifdef VGC_CORE_OBJECT_DEBUG
    #include <iostream>
    #include <vgc/core/format.h>
#endif

namespace vgc {
namespace core {

namespace {
void printDebugInfo_(Object* obj, const char* s)
{
#ifdef VGC_CORE_OBJECT_DEBUG
    std::string info;
    info.reserve();
    info.append("Object ");
    info.append(toAddressString(obj));
    info.append(" ");
    info.append(s);
    info.append(" \n");
    std::cout << info;
#endif
}
} // namespace

Object::Object(const ConstructorKey&)
{
    printDebugInfo_(this, "constructed");
}

Object::~Object()
{
    printDebugInfo_(this, "constructed");
}

/* static */
ObjectSharedPtr Object::create()
{
    return std::make_shared<Object>(ConstructorKey());
}

} // namespace core
} // namespace vgc
