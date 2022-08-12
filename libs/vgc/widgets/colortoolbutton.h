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

#ifndef VGC_WIDGETS_COLORTOOLBUTTON_H
#define VGC_WIDGETS_COLORTOOLBUTTON_H

#include <QToolButton>
#include <vgc/core/color.h>
#include <vgc/core/colors.h>
#include <vgc/widgets/api.h>
#include <vgc/widgets/colordialog.h>

namespace vgc::widgets {

/// \class vgc::core::ColorToolButton
/// \brief Subclass of QToolButton to select a current color
///
/// A ColorToolButton is a QToolButton that opens a ColorDialog when clicked.
/// The ColorDialog can be either owned by the ColorToolButton, or owned by
/// another widget and passed to the ColorToolButton (for example, the same
/// ColorDialog might be used by several ColorToolButton).
///
class VGC_WIDGETS_API ColorToolButton : public QToolButton {
private:
    Q_OBJECT

public:
    /// Constructs a ColorToolButton. If \p colorDialog is null, then the
    /// ColorToolButton will create and own a ColorDialog automatically.
    ///
    ColorToolButton(
        const core::Color& initialColor = core::colors::black,
        QWidget* parent = nullptr,
        ColorDialog* colorDialog = nullptr);

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

    /// Returns the ColorDialog associated with this ColorToolButton.
    ///
    ColorDialog* colorDialog();

Q_SIGNALS:
    /// This signal is emitted when the color changed.
    ///
    void colorChanged(const core::Color& newColor);

private Q_SLOTS:
    void onClicked_();
    void onColorDialogDestroyed_();
    void onColorDialogCurrentColorChanged_(const QColor& color);
    void onColorDialogFinished_(int result);

private:
    core::Color color_;
    core::Color previousColor_;
    ColorDialog* colorDialog_;
};

} // namespace vgc::widgets

#endif // VGC_WIDGETS_COLORTOOLBUTTON_H
