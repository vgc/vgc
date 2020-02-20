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

    /// Creates a vertex buffer for storing triangles data, and returns the ID
    /// of the buffer.
    ///
    /// Once created, you can load triangles data using loadTriangles(id, ...),
    /// and you can draw the loaded triangles using drawTriangles(id).
    ///
    /// When you don't need the buffer anymore (e.g., in the
    /// vgc::ui::Widget::cleanup() function), you must destroy the buffer using
    /// destroyTriangles(id).
    ///
    // Note: in the future, we may add overloads to this function to allow
    // specifying the vertex format (i.e., XYRGB, XYZRGBA, etc.), but for now
    // the format is fixed (XYRGB).
    //
    virtual Int createTriangles() = 0;

    /// Loads the given triangles data. Unless this engine is a software
    /// renderer, this typically sends the data from RAM to the GPU. This
    /// operation is often expensive, and therefore the number of calls to this
    /// function should be minimized. It is preferable to call it once with a
    /// lot of data, than call it several times by chunking the data.
    ///
    /// The given data must be in the following format:
    ///
    /// ```
    /// [x1, y1, r1, g1, b1,     // First vertex of first triangle
    ///  x2, y2, r2, g2, b2,     // Second vertex of first triangle
    ///  x3, y3, r3, g3, b3,     // Third vertex of first triangle
    ///
    ///  x4, y4, r4, g4, b4,     // First vertex of second triangle
    ///  x5, y5, r5, g5, b5,     // Second vertex of second triangle
    ///  x6, y6, r6, g6, b6,     // Third vertex of second triangle
    ///
    ///  ...]
    /// ```
    ///
    // Note: the above format is sub-optimal for the common case where we can
    // share color data across some or all triangles, or share vertex positions
    // across several triangles (via a separate 'indices' buffer), but it is
    // generic and should be enough as first draft of the API.
    //
    virtual void loadTriangles(Int id, const float* data, Int length) = 0;

    /// Draws the given triangles.
    ///
    virtual void drawTriangles(Int id) = 0;

    /// Releases the memory taken by the given triangles buffer.
    ///
    virtual void destroyTriangles(Int id) = 0;
};

} // namespace graphics
} // namespace vgc

#endif // VGC_GRAPHICS_ENGINE_H
