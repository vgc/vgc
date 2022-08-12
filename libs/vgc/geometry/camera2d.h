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

#ifndef VGC_GEOMETRY_CAMERA2D_H
#define VGC_GEOMETRY_CAMERA2D_H

#include <vgc/geometry/api.h>
#include <vgc/geometry/mat4d.h>
#include <vgc/geometry/vec2d.h>

namespace vgc::geometry {

/// \class vgc::geometry::Camera2d
/// \brief 2D camera using double-precision floating points.
///
/// This class is intended to be used for mouse navigation (e.g., pan,
/// zoom, rotate) in a 2D viewer via intuitive controls.
///
/// A 2D camera is defined via the following properties:
///
/// - center: 2D position, in world coordinates, which appears at the center of
///   the viewport.
///
/// - zoom: ratio between the size of an object in view coordinates (i.e., in
///   pixels), and its size in world coordinates. Example: if zoom = 2, then an
///   object which is 100-unit wide in world coordinates appears as 200 pixels
///   on screen.
///
/// - rotation: angle, in radian, between world coordinates and view coordinates.
///   Example: if angle = pi/4, then objects appear rotated 45 degrees
///   anti-clockwise.
///
/// - viewportWidth: the width of the viewport, in pixels.
///
/// - viewportHeight: the height of the viewport, in pixels.
///
/// - nearPlane: value under which z-coordinates are clipped. Default is -1.0.
///
/// - farPlane: value above which z-coordinates are clipped. Default is 1.0.
///
/// Where:
///
/// - "World coordinates" refer to the coordinates of an object as authored
///   by the user. For example, in the following example:
///
///   <vgc>
///     <vertex pos="300 100" />,
///   </vgc>,
///
///   the world coordinates of the vertex are (300, 100). Note that the world
///   coordinates of objects do not change when the user pan, zoom, or rotate
///   the view.
///
///   In SVG terminology, world coordinates are referred to as "user space".
///   For consistency with SVG, we use the convention that the Y-axis in
///   world coordinates is top-down:
///
///   \verbatim
///         o---> X
///         |
///         v Y
///   \endverbatim
///
/// - "Viewport" refers to the area of the screen where the VGC illustration
///   or animation is rendered.
///
/// - "View coordinates" refer to the coordinates of an object relative to the
///   viewport. For example, an object which appears exactly at the top-left
///   corner of the viewport has view coordinates equal to (0, 0). For
///   consistency with Qt (i.e., widget coordinates), we use the convention that
///   the viewport origin is top-left, and that the Y-axis is top-down:
///
///   \verbatim
///         o---> X
///         |
///         v Y
///   \endverbatim
///
///   Note that the view coordinates of an object change when the user pan,
///   zoom, or rotate the view. For example, an object A whose world
///   coordinates are (0, 0) may be initially rendered at the top-left corner
///   of the viewport, in which case its view coordinates are also (0, 0). But
///   if the user decides to center the viewport on the world origin, then the
///   view coordinates of A becomes (w/2, h/2), where (w,h) is the size of the
///   viewport in pixels.
///
/// In order to convert from world coordinates to view coordinates, one can
/// use the viewMatrix() associated with the 2D camera:
///
/// \code
/// Vec2d viewCoords = camera.viewMatrix() * worldCoords;
/// \endcode
///
/// This view matrix is always invertible, therefore we also have:
///
/// \code
/// Vec2d worldCoords = camera.viewMatrix().inverse() * viewCoords;
/// \endcode
///
/// The projectionMatrix() is provided for conveninence when using OpenGL. It
/// maps from view coordinates to NDC (normalized device coordinates), that is,
/// the top-left corner of the viewport (0, 0) is mapped to (-1, -1), and the
/// bottom
///
/// \verbatim
///       Y
///    ---^---  OpenGL NDC
///   |   |   |
///   |   o--->  X
///   |       |
///    -------
/// \endverbatim
///
class VGC_GEOMETRY_API Camera2d {
public:
    /// Construct a 2D Camera centered at the world origin, without zoom or rotation.
    ///
    Camera2d();

    /// Returns the center of the camera. This is the 2D position, in world
    /// coordinates, which appears at the center of the viewport.
    ///
    /// \sa setCenter()
    ///
    const Vec2d& center() const {
        return center_;
    }

    /// Sets the center of the camera.
    ///
    /// \sa center()
    ///
    void setCenter(const Vec2d& center) {
        center_ = center;
    }

    /// Returns the zoom of the camera. This is the ratio between the size of
    /// an object in view coordinates (i.e., in pixels), and its size in world
    /// coordinates. Example: if zoom = 2, then an object which is 100-unit
    /// wide in world coordinates appears as 200 pixels on screen.
    ///
    /// \sa setZoom()
    ///
    double zoom() const {
        return zoom_;
    }

    /// Sets the zoom of the camera.
    ///
    /// \sa zoom()
    ///
    void setZoom(double zoom) {
        zoom_ = zoom;
    }

    /// Returns the rotation of the camera. This is the angle, in radian,
    /// between world coordinates and view coordinates. Example: if angle =
    /// pi/4, then objects appear rotated 45 degrees anti-clockwise.
    ///
    /// \sa setRotation()
    ///
    double rotation() const {
        return rotation_;
    }

    /// Sets the rotation of the camera.
    ///
    /// \sa rotation()
    ///
    void setRotation(double rotation) {
        rotation_ = rotation;
    }

    /// Returns the width of the viewport, in pixels.
    ///
    /// \sa setViewportWidth()
    ///
    double viewportWidth() const {
        return viewportWidth_;
    }

    /// Sets the viewport width.
    ///
    /// \sa viewportWidth()
    ///
    void setViewportWidth(double width) {
        viewportWidth_ = width;
    }

    /// Returns the height of the viewport, in pixels.
    ///
    /// \sa setViewportHeight()
    ///
    double viewportHeight() const {
        return viewportHeight_;
    }

    /// Sets the viewport height.
    ///
    /// \sa viewportHeight()
    ///
    void setViewportHeight(double height) {
        viewportHeight_ = height;
    }

    /// Sets the viewport size.
    ///
    /// \sa viewportWidth(), viewportHeight()
    ///
    void setViewportSize(double width, double height) {
        viewportWidth_ = width;
        viewportHeight_ = height;
    }

    /// Returns the near plane of the camera. This is the value under which
    /// z-coordinates are clipped. Default is -1.0.
    ///
    /// \sa setNearPlane()
    ///
    double nearPlane() const {
        return nearPlane_;
    }

    /// Sets the near plane of the camera.
    ///
    /// \sa nearPlane()
    ///
    void setNearPlane(double nearPlane) {
        nearPlane_ = nearPlane;
    }

    /// Returns the far plane of the camera. This is the value above which
    /// z-coordinates are clipped. Default is 1.0.
    ///
    /// \sa setFarPlane()
    ///
    double farPlane() const {
        return farPlane_;
    }

    /// Sets the far plane of the camera.
    ///
    /// \sa farPlane()
    ///
    void setFarPlane(double farPlane) {
        farPlane_ = farPlane;
    }

    /// Returns the 4x4 view matrix corresponding to the camera.
    ///
    Mat4d viewMatrix() const;

    /// Returns the 4x4 projection matrix corresponding to the camera.
    ///
    Mat4d projectionMatrix() const;

private:
    Vec2d center_;
    double zoom_;
    double rotation_;
    double viewportWidth_;
    double viewportHeight_;
    double nearPlane_;
    double farPlane_;
};

} // namespace vgc::geometry

#endif // VGC_GEOMETRY_CAMERA2D_H
