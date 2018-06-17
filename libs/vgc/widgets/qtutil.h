// Copyright 2018 The VGC Developers
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

#ifndef VGC_WIDGETS_QTUTIL_H
#define VGC_WIDGETS_QTUTIL_H

#include <string>
#include <QColor>
#include <QPointF>
#include <QString>
#include <vgc/core/color.h>
#include <vgc/core/vec2d.h>
#include <vgc/widgets/api.h>

namespace vgc {
namespace widgets {

/// Converts the given UTF-8 encoded std::string \p s into a QString.
///
VGC_WIDGETS_API
QString toQt(const std::string& s);

/// Converts the given QString \p s into a UTF-8 encoded std::string.
///
VGC_WIDGETS_API
std::string fromQt(const QString& s);

/// Converts the given vgc::core::Color \p c into a QColor.
///
VGC_WIDGETS_API
QColor toQt(const core::Color& c);

/// Converts the given QColor \p c into a vgc::core::Color.
///
VGC_WIDGETS_API
core::Color fromQt(const QColor& c);

/// Converts the given vgc::core::Vec2d \p v into a QPointF.
///
VGC_WIDGETS_API
QPointF toQt(const core::Vec2d& v);

/// Converts the given QPointF \p v into a vgc::core::Vec2d.
///
VGC_WIDGETS_API
core::Vec2d fromQt(const QPointF& v);

} // namespace widgets
} // namespace vgc

#endif // VGC_WIDGETS_QTUTIL_H
