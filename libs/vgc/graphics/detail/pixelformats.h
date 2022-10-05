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

// This header has no header guards and is meant to be included after
// defining VGC_PIXEL_FORMAT_MACRO_(Enumerator, ElemSizeInBytes, DXGIFormat, OpenGLInternalFormat, OpenGLPixelType, OpenGLPixelFormat).
// It does undefine VGC_PIXEL_FORMAT_MACRO_ at the end of the table.

// clang-format off

#ifndef VGC_PIXEL_FORMAT_MACRO_
#define VGC_PIXEL_FORMAT_MACRO_(...)
#endif

// This table defines PixelFormat enumerators starting at 0 with additional constants.
//
//                       Enumerator,   ElemSizeInBytes, DXGIFormat,                         OpenGLInternalFormat,    OpenGLPixelType,                    OpenGLPixelFormat
VGC_PIXEL_FORMAT_MACRO_( Undefined,                  0, DXGI_FORMAT_UNKNOWN,                0,                       0,                                  0                   )  // Undefined
// Depth
VGC_PIXEL_FORMAT_MACRO_( D_16_UNORM,                 2, DXGI_FORMAT_D16_UNORM,              GL_DEPTH_COMPONENT16,    GL_UNSIGNED_SHORT,                  GL_DEPTH_COMPONENT  )  // D_16_UNORM
VGC_PIXEL_FORMAT_MACRO_( D_32_FLOAT,                 4, DXGI_FORMAT_D32_FLOAT,              GL_DEPTH_COMPONENT32F,   GL_FLOAT,                           GL_DEPTH_COMPONENT  )  // D_32_FLOAT
// Depth + Stencil
VGC_PIXEL_FORMAT_MACRO_( DS_24_UNORM_8_UINT,         4, DXGI_FORMAT_D24_UNORM_S8_UINT,      GL_DEPTH24_STENCIL8,     GL_UNSIGNED_INT_24_8,               GL_DEPTH_STENCIL    )  // DS_24_UNORM_8_UINT
VGC_PIXEL_FORMAT_MACRO_( DS_32_FLOAT_8_UINT_24_X,    8, DXGI_FORMAT_D32_FLOAT_S8X24_UINT,   GL_DEPTH32F_STENCIL8,    GL_FLOAT_32_UNSIGNED_INT_24_8_REV,  GL_DEPTH_STENCIL    )  // DS_32_FLOAT_8_UINT_24_X
// Red
VGC_PIXEL_FORMAT_MACRO_( R_8_UNORM,                  1, DXGI_FORMAT_R8_UNORM,               GL_R8,                   GL_UNSIGNED_BYTE,                   GL_RED              )  // R_8_UNORM
VGC_PIXEL_FORMAT_MACRO_( R_8_SNORM,                  1, DXGI_FORMAT_R8_SNORM,               GL_R8_SNORM,             GL_BYTE,                            GL_RED              )  // R_8_SNORM
VGC_PIXEL_FORMAT_MACRO_( R_8_UINT,                   1, DXGI_FORMAT_R8_UINT,                GL_R8UI,                 GL_UNSIGNED_BYTE,                   GL_RED_INTEGER      )  // R_8_UINT
VGC_PIXEL_FORMAT_MACRO_( R_8_SINT,                   1, DXGI_FORMAT_R8_SINT,                GL_R8I,                  GL_BYTE,                            GL_RED_INTEGER      )  // R_8_SINT
VGC_PIXEL_FORMAT_MACRO_( R_16_UNORM,                 2, DXGI_FORMAT_R16_UNORM,              GL_R16,                  GL_UNSIGNED_SHORT,                  GL_RED              )  // R_16_UNORM
VGC_PIXEL_FORMAT_MACRO_( R_16_SNORM,                 2, DXGI_FORMAT_R16_SNORM,              GL_R16_SNORM,            GL_SHORT,                           GL_RED              )  // R_16_SNORM
VGC_PIXEL_FORMAT_MACRO_( R_16_UINT,                  2, DXGI_FORMAT_R16_UINT,               GL_R16UI,                GL_UNSIGNED_SHORT,                  GL_RED_INTEGER      )  // R_16_UINT
VGC_PIXEL_FORMAT_MACRO_( R_16_SINT,                  2, DXGI_FORMAT_R16_SINT,               GL_R16I,                 GL_SHORT,                           GL_RED_INTEGER      )  // R_16_SINT
VGC_PIXEL_FORMAT_MACRO_( R_16_FLOAT,                 2, DXGI_FORMAT_R16_FLOAT,              GL_R16F,                 GL_HALF_FLOAT,                      GL_RED              )  // R_16_FLOAT
VGC_PIXEL_FORMAT_MACRO_( R_32_UINT,                  4, DXGI_FORMAT_R32_UINT,               GL_R32UI,                GL_UNSIGNED_INT,                    GL_RED_INTEGER      )  // R_32_UINT
VGC_PIXEL_FORMAT_MACRO_( R_32_SINT,                  4, DXGI_FORMAT_R32_SINT,               GL_R32I,                 GL_INT,                             GL_RED_INTEGER      )  // R_32_SINT
VGC_PIXEL_FORMAT_MACRO_( R_32_FLOAT,                 4, DXGI_FORMAT_R32_FLOAT,              GL_R32F,                 GL_FLOAT,                           GL_RED              )  // R_32_FLOAT
// RG
VGC_PIXEL_FORMAT_MACRO_( RG_8_UNORM,                 2, DXGI_FORMAT_R8G8_UNORM,             GL_RG8,                  GL_UNSIGNED_BYTE,                   GL_RG               )  // RG_8_UNORM
VGC_PIXEL_FORMAT_MACRO_( RG_8_SNORM,                 2, DXGI_FORMAT_R8G8_SNORM,             GL_RG8_SNORM,            GL_BYTE,                            GL_RG               )  // RG_8_SNORM
VGC_PIXEL_FORMAT_MACRO_( RG_8_UINT,                  2, DXGI_FORMAT_R8G8_UINT,              GL_RG8UI,                GL_UNSIGNED_BYTE,                   GL_RG_INTEGER       )  // RG_8_UINT
VGC_PIXEL_FORMAT_MACRO_( RG_8_SINT,                  2, DXGI_FORMAT_R8G8_SINT,              GL_RG8I,                 GL_BYTE,                            GL_RG_INTEGER       )  // RG_8_SINT
VGC_PIXEL_FORMAT_MACRO_( RG_16_UNORM,                4, DXGI_FORMAT_R16G16_UNORM,           GL_RG16,                 GL_UNSIGNED_SHORT,                  GL_RG               )  // RG_16_UNORM
VGC_PIXEL_FORMAT_MACRO_( RG_16_SNORM,                4, DXGI_FORMAT_R16G16_SNORM,           GL_RG16_SNORM,           GL_SHORT,                           GL_RG               )  // RG_16_SNORM
VGC_PIXEL_FORMAT_MACRO_( RG_16_UINT,                 4, DXGI_FORMAT_R16G16_UINT,            GL_RG16UI,               GL_UNSIGNED_SHORT,                  GL_RG_INTEGER       )  // RG_16_UINT
VGC_PIXEL_FORMAT_MACRO_( RG_16_SINT,                 4, DXGI_FORMAT_R16G16_SINT,            GL_RG16I,                GL_SHORT,                           GL_RG_INTEGER       )  // RG_16_SINT
VGC_PIXEL_FORMAT_MACRO_( RG_16_FLOAT,                4, DXGI_FORMAT_R16G16_FLOAT,           GL_RG16F,                GL_HALF_FLOAT,                      GL_RG               )  // RG_16_FLOAT
VGC_PIXEL_FORMAT_MACRO_( RG_32_UINT,                 8, DXGI_FORMAT_R32G32_UINT,            GL_RG32UI,               GL_UNSIGNED_INT,                    GL_RG_INTEGER       )  // RG_32_UINT
VGC_PIXEL_FORMAT_MACRO_( RG_32_SINT,                 8, DXGI_FORMAT_R32G32_SINT,            GL_RG32I,                GL_INT,                             GL_RG_INTEGER       )  // RG_32_SINT
VGC_PIXEL_FORMAT_MACRO_( RG_32_FLOAT,                8, DXGI_FORMAT_R32G32_FLOAT,           GL_RG32F,                GL_FLOAT,                           GL_RG               )  // RG_32_FLOAT
// RGB
VGC_PIXEL_FORMAT_MACRO_( RGB_11_11_10_FLOAT,         4, DXGI_FORMAT_R11G11B10_FLOAT,        GL_R11F_G11F_B10F,       GL_UNSIGNED_INT_10F_11F_11F_REV,    GL_RGB              )  // RGB_11_11_10_FLOAT
VGC_PIXEL_FORMAT_MACRO_( RGB_32_UINT,               12, DXGI_FORMAT_R32G32B32_UINT,         GL_RGB32UI,              GL_UNSIGNED_INT,                    GL_RGB_INTEGER      )  // RGB_32_UINT
VGC_PIXEL_FORMAT_MACRO_( RGB_32_SINT,               12, DXGI_FORMAT_R32G32B32_SINT,         GL_RGB32I,               GL_INT,                             GL_RGB_INTEGER      )  // RGB_32_SINT
VGC_PIXEL_FORMAT_MACRO_( RGB_32_FLOAT,              12, DXGI_FORMAT_R32G32B32_FLOAT,        GL_RGB32F,               GL_FLOAT,                           GL_RGB              )  // RGB_32_FLOAT
// RGBA
VGC_PIXEL_FORMAT_MACRO_( RGBA_8_UNORM,               4, DXGI_FORMAT_R8G8B8A8_UNORM,         GL_RGBA8,                GL_UNSIGNED_BYTE,                   GL_RGBA             )  // RGBA_8_UNORM
VGC_PIXEL_FORMAT_MACRO_( RGBA_8_UNORM_SRGB,          4, DXGI_FORMAT_R8G8B8A8_UNORM_SRGB,    GL_SRGB8_ALPHA8,         GL_UNSIGNED_BYTE,                   GL_RGBA             )  // RGBA_8_UNORM_SRGB
VGC_PIXEL_FORMAT_MACRO_( RGBA_8_SNORM,               4, DXGI_FORMAT_R8G8B8A8_SNORM,         GL_RGBA8_SNORM,          GL_BYTE,                            GL_RGBA             )  // RGBA_8_SNORM
VGC_PIXEL_FORMAT_MACRO_( RGBA_8_UINT,                4, DXGI_FORMAT_R8G8B8A8_UINT,          GL_RGBA8UI,              GL_UNSIGNED_BYTE,                   GL_RGBA_INTEGER     )  // RGBA_8_UINT
VGC_PIXEL_FORMAT_MACRO_( RGBA_8_SINT,                4, DXGI_FORMAT_R8G8B8A8_SINT,          GL_RGBA8I,               GL_BYTE,                            GL_RGBA_INTEGER     )  // RGBA_8_SINT
VGC_PIXEL_FORMAT_MACRO_( RGBA_10_10_10_2_UNORM,      4, DXGI_FORMAT_R10G10B10A2_UNORM,      GL_RGB10_A2,             GL_UNSIGNED_INT_10_10_10_2,         GL_RGBA             )  // RGBA_10_10_10_2_UNORM
VGC_PIXEL_FORMAT_MACRO_( RGBA_10_10_10_2_UINT,       4, DXGI_FORMAT_R10G10B10A2_UINT,       GL_RGB10_A2UI,           GL_UNSIGNED_INT_10_10_10_2,         GL_RGBA_INTEGER     )  // RGBA_10_10_10_2_UINT
VGC_PIXEL_FORMAT_MACRO_( RGBA_16_UNORM,              8, DXGI_FORMAT_R16G16B16A16_UNORM,     GL_RGBA16,               GL_UNSIGNED_SHORT,                  GL_RGBA             )  // RGBA_16_UNORM
VGC_PIXEL_FORMAT_MACRO_( RGBA_16_UINT,               8, DXGI_FORMAT_R16G16B16A16_UINT,      GL_RGBA16UI,             GL_UNSIGNED_SHORT,                  GL_RGBA_INTEGER     )  // RGBA_16_UINT
VGC_PIXEL_FORMAT_MACRO_( RGBA_16_SINT,               8, DXGI_FORMAT_R16G16B16A16_SINT,      GL_RGBA16I,              GL_SHORT,                           GL_RGBA_INTEGER     )  // RGBA_16_SINT
VGC_PIXEL_FORMAT_MACRO_( RGBA_16_FLOAT,              8, DXGI_FORMAT_R16G16B16A16_FLOAT,     GL_RGBA16F,              GL_HALF_FLOAT,                      GL_RGBA             )  // RGBA_16_FLOAT
VGC_PIXEL_FORMAT_MACRO_( RGBA_32_UINT,              16, DXGI_FORMAT_R32G32B32A32_UINT,      GL_RGBA32UI,             GL_UNSIGNED_INT,                    GL_RGBA_INTEGER     )  // RGBA_32_UINT
VGC_PIXEL_FORMAT_MACRO_( RGBA_32_SINT,              16, DXGI_FORMAT_R32G32B32A32_SINT,      GL_RGBA32I,              GL_INT,                             GL_RGBA_INTEGER     )  // RGBA_32_SINT
VGC_PIXEL_FORMAT_MACRO_( RGBA_32_FLOAT,             16, DXGI_FORMAT_R32G32B32A32_FLOAT,     GL_RGBA32F,              GL_FLOAT,                           GL_RGBA             )  // RGBA_32_FLOAT

// clang-format on

#undef VGC_PIXEL_FORMAT_MACRO_

