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

#ifndef VGC_GRAPHICS_PROGRAM_H
#define VGC_GRAPHICS_PROGRAM_H

#include <vgc/graphics/api.h>
#include <vgc/graphics/enums.h>
#include <vgc/graphics/resource.h>

namespace vgc::graphics {

VGC_DECLARE_OBJECT(Engine);

/// \class vgc::graphics::Program
/// \brief Abstract graphics program (linked shaders).
///
class VGC_GRAPHICS_API Program : public Resource {
protected:
    using Resource::Resource;

    Program(ResourceRegistry* registry, BuiltinProgram builtinId)
        : Resource(registry)
        , builtinId_(builtinId) {
    }

public:
    BuiltinProgram builtinId() const noexcept {
        return builtinId_;
    }

    bool isBuiltin() const noexcept {
        return builtinId_ != BuiltinProgram::NotBuiltin;
    }

private:
    BuiltinProgram builtinId_ = BuiltinProgram::NotBuiltin;
};
using ProgramPtr = ResourcePtr<Program>;

} // namespace vgc::graphics

#endif // VGC_GRAPHICS_PROGRAM_H
