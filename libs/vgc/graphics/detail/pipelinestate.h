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

#ifndef VGC_GRAPHICS_PIPELINESTATE_H
#define VGC_GRAPHICS_PIPELINESTATE_H

#include <vgc/core/arithmetic.h>
#include <vgc/geometry/mat4f.h>
#include <vgc/graphics/batch.h>
#include <vgc/graphics/buffer.h>
#include <vgc/graphics/enums.h>
#include <vgc/graphics/framebuffer.h>
#include <vgc/graphics/geometryview.h>
#include <vgc/graphics/image.h>
#include <vgc/graphics/imageview.h>
#include <vgc/graphics/logcategories.h>
#include <vgc/graphics/program.h>
#include <vgc/graphics/resource.h>

namespace vgc::graphics {

VGC_DECLARE_OBJECT(Engine);

// XXX using struct of stacks in Engine for now

struct VGC_GRAPHICS_API PipelineState {

    // I need shared pointers !

    Int viewportX_;
    Int viewportY_;
    Int viewportWidth_;
    Int viewportHeight_;
};

} // namespace vgc::graphics

#endif // VGC_GRAPHICS_PIPELINESTATE_H
