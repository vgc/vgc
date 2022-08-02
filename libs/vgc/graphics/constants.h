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

#ifndef VGC_GRAPHICS_CONSTANTS_H
#define VGC_GRAPHICS_CONSTANTS_H

#include <vgc/core/arithmetic.h>
#include <vgc/graphics/api.h>

namespace vgc::graphics {

// see:
// D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT    (14)
// GL_MAX_FRAGMENT_UNIFORM_BLOCKS                       (12 in GL3.3)
// GL_MAX_VERTEX_UNIFORM_BLOCKS                         (12 in GL3.3)
// GL_MAX_GEOMETRY_UNIFORM_BLOCKS                       (12 in GL3.3)
inline constexpr UInt32 maxConstantBuffersPerStage = 12;

// There is no equivalent to D3D SRVs in OpenGL.
// We have to use samplers and thus share the max count (16/2).
//
// see:
// D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT (128)
// D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT        (16)
// GL_MAX_TEXTURE_IMAGE_UNITS                   (16 in GL3.3)
// GL_MAX_VERTEX_TEXTURE_IMAGE_UNITS            (16 in GL3.3)
// GL_MAX_GEOMETRY_TEXTURE_IMAGE_UNITS          (16 in GL3.3)
//
inline constexpr UInt32 maxImageViewsPerStage = 8;
inline constexpr UInt32 maxSamplersPerStage = 8;

} // namespace vgc::graphics

#endif // VGC_GRAPHICS_CONSTANTS_H
