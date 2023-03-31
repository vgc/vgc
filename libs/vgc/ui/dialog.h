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

#include <vgc/ui/widget.h>

namespace vgc::ui {

enum class DialogLocation : Int8 {
    WindowCenter

    // TODO?
    //
    //    VCenter,
    //    TopAlign,
    //    BottomAlign,
    //    Above,
    //    Below
    //
    //    x
    //
    //    HCenter,
    //    LeftAlign,
    //    RightAlign,
    //    ToTheRight,
    //    ToTheLeft
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
    Dialog();

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

    /// Move the dialog centered in the topmost overlay area of the given widget.
    ///
    /// TODO: next to widget, to the right, below, etc.
    ///
    void showAt(Widget* widget, DialogLocation position = DialogLocation::WindowCenter);

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
