// Copyright 2017 The VGC Developers
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

#ifndef VGC_WIDGETS_COLORSELECTOR_H
#define VGC_WIDGETS_COLORSELECTOR_H

#include <QToolButton>
#include <vgc/core/color.h>
#include <vgc/core/colors.h>
#include <vgc/widgets/api.h>

namespace vgc {
namespace widgets {

/// \class vgc::core::ColorSelector
/// \brief Subclass of QToolButton to select a current color
///
class VGC_WIDGETS_API ColorSelector : public QToolButton
{
    Q_OBJECT

public:
    /// Constructs a ColorSelector.
    ///
    ColorSelector(const core::Color& initialColor = core::colors::black,
                  QWidget* parent = nullptr);

    /// Returns the current color.
    ///
    core::Color color() const;

    /// Modifies the current color.
    ///
    void setColor(const core::Color& color);

    /// Updates the icon of this QToolButton to match the current color. Note
    /// that this is automatically called when setColor() is called, but it is
    /// NOT automatically called when setIconSize() is called (because
    /// setIconSize() is not a virtual function and therefore its behaviour
    /// could not be changed). Therefore, you must call this function manually
    /// whenever you call setIconSize().
    ///
    void updateIcon();

Q_SIGNALS:
    void colorChanged(const core::Color& newColor);

private Q_SLOTS:
    void onClicked_();

private:
    core::Color color_;
};

} // namespace widgets
} // namespace vgc

#endif // VGC_WIDGETS_COLORSELECTOR_H
