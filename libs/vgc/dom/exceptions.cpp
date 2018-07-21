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

#include <vgc/dom/exceptions.h>

#include <vgc/core/stringutil.h>
#include <vgc/dom/node.h>

namespace vgc {
namespace dom {

namespace {
std::string what_(const Node* node)
{
    std::string res;
    res.reserve(36); // = size("Node 0x1234567812345678 is not alive")
    res.append("Node ");
    res.append(core::toString(node));
    res.append(" is not alive");
    return res;
}
} // namespace

NotAliveException::NotAliveException(const Node* node) :
    core::Exception(what_(node))
{

}

} // namespace dom
} // namespace vgc
