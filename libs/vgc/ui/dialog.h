// Copyright 2023 The VGC Developers
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

#ifndef VGC_UI_DIALOG_H
#define VGC_UI_DIALOG_H

#include <vgc/core/arithmetic.h>
#include <vgc/core/enum.h>
#include <vgc/geometry/rangealign.h>
#include <vgc/geometry/rectalign.h>
#include <vgc/ui/api.h>
#include <vgc/ui/widget.h>

namespace vgc::ui {

/// \enum vgc::ui::DialogLocationType
/// \brief Whether a dialog should be positionned relative to the cursor, a widget, or a window.
///
enum class DialogLocationType : UInt8 {
    Cursor,
    Widget,
    Window
};

VGC_UI_API
VGC_DECLARE_ENUM(DialogLocationType)

/// \class vgc::ui::DialogLocation
/// \brief Specifies, along one axis, where to show a Dialog.
///
/// A dialog can be positionned relative to the cursor, to a widget, or to a
/// window.
///
/// Such positionning can be controlled independently for the horizontal and
/// vertical axis, and this class represents a specification of such alignment
/// along one of the axes. Passing two instances of this class to the
/// `Dialog::showAt()` method fully specifies the 2D position of the dialog:
///
/// ```cpp
/// Dialog::showAt(DialogLocation horizontal, DialogLocation vertical)
/// ```
///
/// For example, it is possible to specify that a dialog should appear at the
/// top of the window, but horizontally centered with a given widget. Or appear
/// to the right of one widget, and vertically centered with another widget.
///
/// ```
///  widget1
/// +---------------------+
/// |   +--+              | +------+
/// |   |  | widget2      | |      | dialog: - outside the right side of widget1
/// |   +--+              | +------+         - vertically centered with widget2
/// |                     |
/// +---------------------+
/// ```
///
/// For this you would use the following:
///
/// ```cpp
/// dialog->showAt(
///     DialogLocation::atWidget(widget1, RangeAlign::OutMax),
///     DialogLocation::atWidget(widget2, RangeAlign::Center));
/// ```
///
/// Or equivalently, use the short form:
///
/// ```cpp
/// dialog->showAt(widget1, widget2, RectAlign::OutRight));
/// ```
///
class DialogLocation {
public:
    /// Creates a `DialogLocation` with the given `type`, `widget`, and `align`
    /// properties.
    ///
    /// \sa `atWidget()`, `atWindow()`, `atCursor()`.
    ///
    DialogLocation(DialogLocationType type, WidgetPtr widget, geometry::RangeAlign align)

        : widget_(widget)
        , type_(type)
        , align_(align) {
    }

    /// Creates a `DialogLocation` of type `DialogLocationType::Widget`
    /// in the given `widget->window()` aligned with the given `widget`.
    ///
    static DialogLocation atWidget(WidgetPtr widget, geometry::RangeAlign align) {
        return DialogLocation(DialogLocationType::Widget, widget, align);
    }

    /// Creates a `DialogLocation` of type `DialogLocationType::Window`
    /// in the given `widget->window()`, positionned at the given `anchor` relative
    /// to the window.
    ///
    static DialogLocation atWindow(WidgetPtr widget, geometry::RangeAnchor anchor) {
        return DialogLocation(DialogLocationType::Window, widget, toRangeAlign(anchor));
    }

    /// Creates a `DialogLocation` of type `DialogLocationType::Cursor` in the
    /// given `widget->window()`, positionned at the given `anchor` relative to
    /// the cursor.
    ///
    static DialogLocation atCursor(WidgetPtr widget, geometry::RangeAnchor anchor) {
        return DialogLocation(
            DialogLocationType::Window, widget, toRangeAlign(reverse(anchor)));
    }

    /// Returns the type of this `DialogLocation`.
    ///
    DialogLocationType type() const {
        return type_;
    }

    /// Sets the type of this `DialogLocation`.
    ///
    void setType(DialogLocationType type) {
        type_ = type;
    }

    /// Returns which widget this `DialogLocation` is relative to.
    ///
    Widget* widget() const {
        return widget_.get();
    }

    /// Sets which widget this `DialogLocation` is relative to.
    ///
    void setWidget(Widget* widget) {
        widget_ = widget;
    }

    /// Returns how to align the dialog with respect to the cursor, widget, or
    /// window (depending on `type()`).
    ///
    geometry::RangeAlign align() const {
        return align_;
    }

    /// Sets how to align the dialog with respect to the cursor, widget, or
    /// window (depending on `type()`).
    ///
    void setAlign(geometry::RangeAlign align) {
        align_ = align;
    }

private:
    WidgetPtr widget_;
    DialogLocationType type_;
    geometry::RangeAlign align_;
};

VGC_DECLARE_OBJECT(Dialog);

/// \class vgc::ui::Dialog
/// \brief Short-lived widget displayed as overlay or separate window.
///
/// The `Dialog` class is a base class meant to be used for short-lived widgets
/// displayed as overlays or in separate windows, typically informing users
/// about something important or asking them for input.
///
/// Dialogs can be either *modal* or *modeless*. A modal dialog is a dialog
/// that prevent users from performing any other action on the application
/// until they have closed the dialog (for example by clicking the "OK"
/// button). A modeless dialog is a dialog that doesn't prevent users from
/// performing other actions, for example changing the current selection or
/// scrolling the document which may be useful to change the content or the
/// dialog or get enough information to be able to provide the required input.
///
/// As a general design rule, it is preferred to use modeless dialogs whenever
/// possible as it is the least invasive for the user.
///
class VGC_UI_API Dialog : public Widget {
private:
    VGC_OBJECT(Dialog, Widget)

protected:
    /// This is an implementation details. Please use
    /// `Dialog::create()` instead.
    ///
    Dialog(CreateKey);

public:
    /// Creates a `Dialog`.
    ///
    static DialogPtr create();

    /// Returns the content widget of this dialog, that is, its only
    /// child (if any).
    ///
    /// Returns `nullptr` if this dialog doesn't have any child.
    ///
    /// \sa `setContent()`.
    ///
    Widget* content() const {
        return firstChild();
    }

    /// Sets the given `widget` as content of this dialog.
    ///
    /// The dialog becomes the new parent of the widget, and any pre-existing
    /// content of the dialog is destroyed.
    ///
    /// If `widget` is null, the dialog becomes childless.
    ///
    /// \sa `content()`.
    ///
    void setContent(Widget* widget);

    /// Creates a new widget of the given `WidgetClass`, and sets it
    /// as the content widget of this dialog.
    ///
    /// \sa `content()`, `setContent()`
    ///
    template<typename WidgetClass, typename... Args>
    WidgetClass* createContent(Args&&... args) {
        core::ObjPtr<WidgetClass> child =
            WidgetClass::create(std::forward<Args>(args)...);
        setContent(child.get());
        return child.get();
    }

    // TODO: isModal, isDismissable, title, basic signals, etc.

    /// Shows the dialog at the given `horizontal` and `vertical` location.
    ///
    void showAt(DialogLocation horizontal, DialogLocation vertical);

    /// Shows the dialog on a location defined by the given `type` and `widget`,
    /// with the given alignment.
    ///
    void showAt(DialogLocationType type, Widget* widget, geometry::RectAlign align) {
        showAt(
            DialogLocation(type, widget, horizontalAlign(align)),
            DialogLocation(type, widget, verticalAlign(align)));
    }

    /// Shows the dialog at the given `widget` with the given alignment.
    ///
    void showAt(Widget* widget, geometry::RectAlign align) {
        showAt(DialogLocationType::Widget, widget, align);
    }

    /// Shows the dialog relative to the given `hWidget` for the horizontal
    /// direction, and `vWidget`.
    ///
    /// Example:
    ///
    /// ```cpp
    /// dialog->showAt(hWidget, vWidget, RectAlign::OutRight);
    /// ```
    ///
    /// Output:
    ///
    /// ```
    ///  hWidget
    /// +---------------------+
    /// |   +--+              | +------+
    /// |   |  | vWidget      | |      | dialog: - outside the right side of hWidget
    /// |   +--+              | +------+         - vertically centered with vWidget
    /// |                     |
    /// +---------------------+
    /// ```
    ///
    void showAt(Widget* hWidget, Widget* vWidget, geometry::RectAlign align) {
        DialogLocationType type = DialogLocationType::Widget;
        showAt(
            DialogLocation(type, hWidget, horizontalAlign(align)),
            DialogLocation(type, vWidget, verticalAlign(align)));
    }

    /// Shows the dialog relative to the given `hWidget` for the horizontal
    /// direction, and `vWidget` for the vertical direction.
    ///
    void showAt(
        Widget* hWidget,
        geometry::RangeAlign hAlign,
        Widget* vWidget,
        geometry::RangeAlign vAlign) {

        DialogLocationType type = DialogLocationType::Widget;
        showAt(
            DialogLocation(type, hWidget, hAlign), DialogLocation(type, vWidget, vAlign));
    }

    /// Shows the dialog aligned with the edges of the window of the given
    /// `widget`.
    ///
    void showAtWindow(
        Widget* widget,
        geometry::RectAnchor anchor = geometry::RectAnchor::Center) {

        showAt(DialogLocationType::Window, widget, toRectAlign(anchor));
    }

    /// Shows the dialog aligned with the current cursor, shown on the
    /// window of the given `widget`.
    ///
    /// Note that for convenience, in this function, `TopRight` is interpreted to mean
    /// "place the dialog above the cursor, and to its right". This
    /// is equivalent to actually specifying either `XTopXRight` or `BottomLeft`
    /// if using the `showAt(DialogLocationType, Widget*, XRectAlign)` overload.
    ///
    void showAtCursor(
        Widget* widget,
        geometry::RectAnchor anchor = geometry::RectAnchor::Center) {

        showAt(DialogLocationType::Cursor, widget, toRectAlign(reverse(anchor)));
    }

    /// Shows the dialog on the side of the inner-most `PanelArea`, if any,
    /// that contains the given `widget`.
    ///
    /// If there is no such `PanelArea` then this function returns false, and
    /// as fallback the dialog is shown outside the bottom-right corner of the
    /// widget. If such fallback does not suit your needs, you can then call
    /// another `showAt()` overload just after to move the dialog in a more
    /// appropriate location.
    ///
    bool showOutsidePanelArea(Widget* widget);

    // reimpl
    float preferredWidthForHeight(float height) const override;
    float preferredHeightForWidth(float width) const override;

protected:
    void onWidgetAdded(Widget* child, bool wasOnlyReordered) override;
    void onWidgetRemoved(Widget* child) override;
    geometry::Vec2f computePreferredSize() const override;
    void updateChildrenGeometry() override;
};

} // namespace vgc::ui

#endif // VGC_UI_DIALOG_H
