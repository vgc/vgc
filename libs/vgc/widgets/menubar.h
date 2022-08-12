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

#ifndef VGC_WIDGETS_MENUBAR_H
#define VGC_WIDGETS_MENUBAR_H

#include <vector>

#include <QMenuBar>

#include <vgc/widgets/api.h>

namespace vgc::widgets {

/// \class vgc::widgets::MenuBar
/// \brief A subclass of QMenuBar for increased customizability.
///
/// Qt provides limited styling options for QMenuBar, documented here:
///
///    https://doc.qt.io/qt-5/stylesheet-examples.html#customizing-qmenubar
///
/// Unfortunately, it lacks documentation and/or support for specifying the
/// borders of the menu bar, as well as the margin between the menu items and the
/// border of the menu bar. To the best of my knowledge, this makes it
/// impossible to specify, using a stylesheet only, a flat-style menu where
/// menu items extend to the very bottom of a borderless menu bar.
///
/// This subclass of QMenuBar helps alleviating this problem by reimplementing
/// paintEvent() and providing additional properties that can be specified in
/// a stylesheet.
///
class VGC_WIDGETS_API MenuBar : public QMenuBar {
private:
    Q_OBJECT

    // Additional properties that can be specified in a stylesheet. See:
    //
    // https://wiki.qt.io/Qt_Style_Sheets_and_Custom_Painting_Example
    //
    Q_PROPERTY(QColor activeBorderBottomColor READ activeBorderBottomColor WRITE
                   setActiveBorderBottomColor DESIGNABLE true)

public:
    /// Constructs a MenuBar.
    ///
    explicit MenuBar(QWidget* parent = nullptr);

    /// Destructs the MenuBar.
    ///
    ~MenuBar();

    /// Returns the color used as border-bottom of the active menu item,
    /// overriding the border-bottom of the menu bar itself:
    ///
    /// ------------------------------
    ///
    ///    File      Edit      View      <- computed height of menu items
    ///
    /// ------------------------------
    ///           ##########             <- border-bottom of the menu bar
    /// ------------------------------
    ///               ^
    ///            overriding QMenuBar's border-bottom for the active menu item
    ///
    /// Why do we need this? Because as of Qt 5.12.4, and at least on Windows
    /// 10, it seems impossible to remove the 1px spacing between the menu
    /// items and the bottom of the QMenuBar. Either you set QMenuBar {
    /// border-bottom: 1px solid red } and you get a 1px border-bottom as
    /// expected, with menu items extending up to the border. Or you set
    /// QMenuBar { border: 0px }, and indeed there is no border, but the menu
    /// items don't extend to the bottom of the menu bar, instead leaving a
    /// 1px margin, filled by the QMenuBar's background color. If you're
    /// aiming for a flat design, this 1px margin looks out of place when
    /// highlighting the active menu item.
    ///
    /// This additional property provides a workaround by using the following
    /// stylesheet:
    ///
    /// \code
    /// QMenuBar {
    ///     background: blue;
    ///     border-bottom: 1px solid blue;
    /// }
    ///
    /// QMenuBar::item {
    ///     spacing: 0px;
    ///     background: transparent;
    /// }
    ///
    /// QMenuBar::item:selected,
    /// QMenuBar::item:pressed {
    ///     background: red;
    /// }
    ///
    /// vgc--widgets--MenuBar {
    ///     qproperty-activeBorderBottomColor: red;
    /// }
    /// \endcode
    ///
    /// Note: currently, the popup menu that appears when opening the menu
    /// will overlap with the border-bottom of the menu-bar. This is also
    /// annoying when using a flat design, since we would prefer this popup
    /// menu to open 1px lower, not hiding the last row of pixels of the
    /// menubar. We don't currently have a workaround for that.
    ///
    /// \sa setActiveBorderBottomColor()
    ///
    QColor activeBorderBottomColor() const;

    /// Sets the color used as border-bottom of the active menu item,
    /// overriding the border-bottom of the menu bar itself.
    ///
    /// \sa activeBorderBottomColor()
    ///
    void setActiveBorderBottomColor(QColor c);

protected:
    /// Reimplements QMenuBar::paintEvent().
    ///
    virtual void paintEvent(QPaintEvent*) override;

private:
    QColor activeBorderBottomColor_;
};

} // namespace vgc::widgets

#endif // VGC_WIDGETS_MENUBAR_H
