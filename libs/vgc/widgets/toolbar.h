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

#ifndef VGC_WIDGETS_TOOLBAR_H
#define VGC_WIDGETS_TOOLBAR_H

#include <QAction>
#include <QToolBar>

#include <vgc/ui/colorpalette.h>
#include <vgc/widgets/api.h>
#include <vgc/widgets/colortoolbutton.h>
#include <vgc/widgets/uiwidget.h>

namespace vgc::widgets {

/// \class vgc::widgets::Toolbar
/// \brief The toolbar
///
class VGC_WIDGETS_API Toolbar : public QToolBar {
private:
    Q_OBJECT

public:
    /// Constructs a Toolbar.
    ///
    explicit Toolbar(QWidget* parent = nullptr);

    /// Destructs the Toolbar.
    ///
    ~Toolbar() override;

    /// Returns the current color.
    ///
    core::Color color() const;

Q_SIGNALS:
    /// This signal is emitted when the color changed.
    ///
    void colorChanged(const core::Color& newColor);

protected:
    void resizeEvent(QResizeEvent* event) override;

private Q_SLOTS:
    void onColorToolButtonColorChanged_();
    void onColorPaletteColorSelected_();

private:
    ColorToolButton* colorToolButton_;
    QAction* colorToolButtonAction_;
    ui::ColorPalette* colorPalette_;
    UiWidget* colorPaletteq_;
};

} // namespace vgc::widgets

#endif // VGC_WIDGETS_TOOLBAR_H
