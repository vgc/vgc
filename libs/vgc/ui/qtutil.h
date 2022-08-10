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

/// \file vgc/widgets/qtutil.h
/// \brief Convenient functions to interface between Qt and the rest of the
///        application.
///

#ifndef VGC_UI_QTUTIL_H
#define VGC_UI_QTUTIL_H

#include <string>

#include <QColor>
#include <QMatrix4x4>
#include <QPointF>
#include <QString>

#include <vgc/core/color.h>
#include <vgc/core/object.h>
#include <vgc/geometry/mat4d.h>
#include <vgc/geometry/mat4f.h>
#include <vgc/geometry/vec2d.h>
#include <vgc/geometry/vec2f.h>
#include <vgc/ui/api.h>

class QMouseEvent;

namespace vgc::ui {

VGC_DECLARE_OBJECT(MouseEvent);

/// Converts the given UTF-8 encoded std::string \p s into a QString.
///
VGC_UI_API
QString toQt(const std::string& s);

/// Converts the given QString \p s into a UTF-8 encoded std::string.
///
VGC_UI_API
std::string fromQt(const QString& s);

/// Converts the given vgc::core::Color \p c into a QColor.
///
VGC_UI_API
QColor toQt(const core::Color& c);

/// Converts the given QColor \p c into a vgc::core::Color.
///
VGC_UI_API
core::Color fromQt(const QColor& c);

/// Converts the given vgc::geometry::Vec2d \p v into a QPointF.
///
VGC_UI_API
QPointF toQt(const geometry::Vec2d& v);

/// Converts the given vgc::geometry::Vec2f \p v into a QPointF.
///
VGC_UI_API
QPointF toQt(const geometry::Vec2f& v);

/// Converts the given QPointF \p v into a vgc::geometry::Vec2d.
///
VGC_UI_API
geometry::Vec2d fromQtd(const QPointF& v);

/// Converts the given QPointF \p v into a vgc::geometry::Vec2f.
///
VGC_UI_API
geometry::Vec2f fromQtf(const QPointF& v);

/// Converts the given QMouseEvent into a vgc::ui::MouseEvent.
///
VGC_UI_API
MouseEventPtr fromQt(QMouseEvent* event);

/// Converts the given geometry::Mat4f into a QMatrix4x4.
///
VGC_UI_API
QMatrix4x4 toQt(const geometry::Mat4f& m);

/// Converts the given geometry::Mat4d into a QMatrix4x4.
///
VGC_UI_API
QMatrix4x4 toQt(const geometry::Mat4d& m);

} // namespace vgc::ui

#endif // VGC_UI_QTUTIL_H
