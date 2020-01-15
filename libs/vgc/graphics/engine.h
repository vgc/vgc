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

#ifndef VGC_GRAPHICS_ENGINE_H
#define VGC_GRAPHICS_ENGINE_H

#include <vgc/core/color.h>
#include <vgc/core/innercore.h>
#include <vgc/graphics/api.h>

namespace vgc {
namespace graphics {

VGC_CORE_DECLARE_PTRS(Engine);

/// \class vgc::graphics::Engine
/// \brief Abstract interface for graphics rendering.
///
/// This class is an abstract base class defining a common shared API for
/// graphics rendering. Implementations of this abstraction may provide
/// backends such as OpenGL, Vulkan, Direct3D, Metal, or software rendering.
///
class VGC_GRAPHICS_API Engine : public core::Object
{
    VGC_CORE_OBJECT(Engine)

protected:
    /// Constructs an Engine. This constructor is an implementation detail only
    /// available to derived classes.
    ///
    Engine();

public:
    /// Clears the whole render area with the given color.
    ///
    virtual void clear(const core::Color& color) = 0;
};

} // namespace graphics
} // namespace vgc

#endif // VGC_GRAPHICS_ENGINE_H
