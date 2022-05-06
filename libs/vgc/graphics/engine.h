// Copyright 2021 The VGC Developers
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

#include <memory>

#include <vgc/core/color.h>
#include <vgc/core/innercore.h>
#include <vgc/core/mat4f.h>
#include <vgc/graphics/api.h>

namespace vgc {
namespace graphics {

VGC_DECLARE_OBJECT(Engine);

using ResourcePtr = std::shared_ptr<class Resource>;
class VGC_GRAPHICS_API Resource : public std::enable_shared_from_this<Resource> {
protected:
    explicit Resource(Engine* engine);

public:
    ~Resource() {
        destroy_();
    }

    Resource(const Resource&) = delete;
    Resource& operator=(const Resource&) = delete;

    Engine* engine() const {
        return engine_;
    }

private:
    void destroy_();
    virtual void destroyImpl() = 0;

    core::ConnectionHandle onEngineDestroyedConnectionHandle;
    Engine* engine_;
};

using TrianglesBufferPtr = std::shared_ptr<class TrianglesBuffer>;
class VGC_GRAPHICS_API TrianglesBuffer : public Resource {
public:
    using Resource::Resource;

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
    virtual void load(const float* data, Int length) = 0;

    /// Draws the given triangles.
    ///
    virtual void draw() = 0;
};


/// \class vgc::graphics::Engine
/// \brief Abstract interface for graphics rendering.
///
/// This class is an abstract base class defining a common shared API for
/// graphics rendering. Implementations of this abstraction may provide
/// backends such as OpenGL, Vulkan, Direct3D, Metal, or software rendering.
///
/// The graphics engine is responsible for managing two matrix stacks: the
/// projection matrix stack, and the view matrix stack. When the engine is
/// constructed, each of these stacks is initialized with the identity matrix
/// as the only matrix in the stack. It is undefined behavior for clients to
/// call "pop" more times than "push": some implementations may emit an
/// exception (instantly or later), others may cause a crash (instantly or
/// later), or the drawing operations may fail.
///
class VGC_GRAPHICS_API Engine : public core::Object {
private:
    VGC_OBJECT(Engine, core::Object)
    VGC_PRIVATIZE_OBJECT_TREE_MUTATORS

protected:
    /// Constructs an Engine. This constructor is an implementation detail only
    /// available to derived classes.
    ///
    Engine();

public:
    /// Clears the whole render area with the given color.
    ///
    virtual void clear(const core::Color& color) = 0;

    /// Returns the current projection matrix.
    ///
    virtual core::Mat4f projectionMatrix() = 0;

    /// Sets the current projection matrix.
    ///
    virtual void setProjectionMatrix(const core::Mat4f& m) = 0;

    /// Adds a copy of the current projection matrix to the matrix stack.
    ///
    virtual void pushProjectionMatrix() = 0;

    /// Removes the current projection matrix from the matrix stack.
    ///
    /// The behavior is undefined if there is only one matrix in the stack
    /// before calling this function.
    ///
    virtual void popProjectionMatrix() = 0;

    /// Returns the current view matrix.
    ///
    virtual core::Mat4f viewMatrix() = 0;

    /// Sets the current view matrix.
    ///
    virtual void setViewMatrix(const core::Mat4f& m) = 0;

    /// Adds a copy of the current view matrix to the matrix stack.
    ///
    virtual void pushViewMatrix() = 0;

    /// Removes the current view matrix from the matrix stack.
    ///
    /// The behavior is undefined if there is only one matrix in the stack
    /// before calling this function.
    ///
    virtual void popViewMatrix() = 0;

    /// Creates a vertex buffer for storing triangles data, and returns the ID
    /// of the buffer.
    ///
    /// Once created, you can load triangles data using
    /// resource->load(...), and you can draw the loaded triangles
    /// using resource->draw().
    ///
    /// When you don't need the buffer anymore (e.g., in the
    /// vgc::ui::Widget::cleanup() function), you must reset the resource pointer.
    ///
    // Note: in the future, we may add overloads to this function to allow
    // specifying the vertex format (i.e., XYRGB, XYZRGBA, etc.), but for now
    // the format is fixed (XYRGB).
    //
    virtual TrianglesBufferPtr createTriangles() = 0;
};

} // namespace graphics
} // namespace vgc

#endif // VGC_GRAPHICS_ENGINE_H
