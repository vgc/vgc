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
inline constexpr Int maxConstantBuffersPerStage = 12;

// There is no equivalent to D3D SRVs in OpenGL.
// We have to reason in terms of textures.
//
// see:
// D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT (128)
// D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT        (16)
// GL_MAX_TEXTURE_IMAGE_UNITS                   (16 in GL3.3)
// GL_MAX_VERTEX_TEXTURE_IMAGE_UNITS            (16 in GL3.3)
// GL_MAX_GEOMETRY_TEXTURE_IMAGE_UNITS          (16 in GL3.3)
//
inline constexpr Int maxImageViewsPerStage = 16;
inline constexpr Int maxSamplersPerStage = 16;

inline constexpr Int maxColorTargets = 8;

inline constexpr Int maxImageWidth = 16384;
inline constexpr Int maxImageHeight = 16384;
//inline constexpr Int maxImageDepth = 16384;
inline constexpr Int maxImageLayers = 2048;

inline constexpr Int maxAnisotropy = 16;
inline constexpr Int maxNumSamples = 8;

} // namespace vgc::graphics

#endif // VGC_GRAPHICS_CONSTANTS_H
