// Copyright 2017 The VGC Developers
// See the COPYRIGHT file at the top-level directory of this distribution
// and at https://github.com/vgc.io/vgc/blob/master/COPYRIGHT
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
#include <vgc/geometry/vec2d.h>

namespace vgc {
namespace geometry {

/// \class vgc::geometry::Camera2d
/// \brief 2D camera using double-precision floating points.
///
/// This class is intended to be used to provide mouse navigation (e.g., pan,
/// zoom, rotate) in a 2D viewer via intuitive controls. In traditional
/// computer graphics terminology (e.g., OpenGL), this class represents both
/// the "view" matrix and the "projection" matrix.
///
/// A 2D camera is defined via the following properties:
///
/// - center: 2D position, in scene coordinates, which appears
///   at the center of the viewport.
///
/// - zoom: ratio between the size of an object in pixels and its size in scene
///   coordinates. Example: if zoom = 2, then one unit in scene coordinates
///   appears as 2 pixels on screen.
///
/// - rotation: angle, in radian, between scene coordinates and view coordinates.
///   Example: if angle = pi/4, then objects appear rotated 45 degrees
///   anti-clockwise.
///
/// - viewportWidth: the width of the viewport, in pixels.
///
/// - viewportHeight: the height of the viewport, in pixels.
///
/// - near: value under which z-coordinates are clipped. Default is -1.0.
///
/// - far: value above which z-coordinates are clipped. Default is 1.0.
///
class VGC_GEOMETRY_API Camera2d
{
public:
    /// Construct a 2D Camera centered at the scene origin, without zoom or rotation.
    ///
    Camera2d();

    /// Returns the center of the camera. This is the 2D position, in scene
    /// coordinates, which appears at the center of the viewport.
    ///
    /// \sa setCenter()
    ///
    const Vec2d& center() const { return center_; }

    /// Sets the center of the camera.
    ///
    /// \sa center()
    ///
    void setCenter(const Vec2d& center) { center_ = center; }

    /// Returns the zoom of the camera. This is the ratio between the size of
    /// an object in pixels and its size in scene coordinates. Example: if zoom
    /// = 2, then one unit in scene coordinates appears as 2 pixels on screen.
    ///
    /// \sa setZoom()
    ///
    double zoom() const { return zoom_; }

    /// Sets the zoom of the camera.
    ///
    /// \sa zoom()
    ///
    void setZoom(double zoom) { zoom_ = zoom; }

    /// Returns the rotation of the camera. This is the angle, in radian,
    /// between scene coordinates and view coordinates. Example: if angle =
    /// pi/4, then objects appear rotated 45 degrees anti-clockwise.
    ///
    /// \sa setRotation()
    ///
    double rotation() const { return rotation_; }

    /// Sets the rotation of the camera.
    ///
    /// \sa rotation()
    ///
    void setRotation(double rotation) { rotation_ = rotation; }

    /// Returns the width of the viewport, in pixels.
    ///
    /// \sa setViewportWidth()
    ///
    double viewportWidth() const { return viewportWidth_; }

    /// Sets the viewport width.
    ///
    /// \sa viewportWidth()
    ///
    void setViewportWidth(double viewportWidth) { viewportWidth_ = viewportWidth; }

    /// Returns the height of the viewport, in pixels.
    ///
    /// \sa setViewportHeight()
    ///
    double viewportHeight() const { return viewportHeight_; }

    /// Sets the viewport height.
    ///
    /// \sa viewportHeight()
    ///
    void setViewportHeight(double viewportHeight) { viewportHeight_ = viewportHeight; }

    /// Returns the near value of the camera. This is the value under which
    /// z-coordinates are clipped. Default is -1.0.
    ///
    /// \sa setNear()
    ///
    double near() const { return near_; }

    /// Sets the near value of the camera.
    ///
    /// \sa near()
    ///
    void setNear(double near) { near_ = near; }

    /// Returns the far value of the camera. This is the value above which
    /// z-coordinates are clipped. Default is 1.0.
    ///
    /// \sa setFar()
    ///
    double far() const { return far_; }

    /// Sets the far value of the camera.
    ///
    /// \sa far()
    ///
    void setFar(double far) { far_ = far; }

private:
    Vec2d center_;
    double zoom_;
    double rotation_;
    double viewportWidth_;
    double viewportHeight_;
    double near_;
    double far_;
};

} // namespace geometry
} // namespace vgc

#endif // VGC_GEOMETRY_CAMERA2D_H
