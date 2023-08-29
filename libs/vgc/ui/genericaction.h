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

#ifndef VGC_UI_GENERICACTION_H
#define VGC_UI_GENERICACTION_H

#include <vgc/ui/action.h>

namespace vgc::ui {

VGC_DECLARE_OBJECT(GenericAction);

/// \class vgc::ui::GenericAction
/// \brief A generic trigger action that delegates its implementation based on focus
///
/// A `GenericAction` is designed to be used for menu items like "Copy",
/// "Paste", etc., which should perform a different action based on which
/// widgets are currently in the focus stack.
///
/// For example, if a `LineEdit` is focused, then it should invoke the copy
/// action of the line edit (copying text). If a `canvas::Canvas` is focused,
/// then it should invoke the copy action of the canvas (copying selected
/// canvas items).
///
/// The intended use is the following:
///
/// 1. An application (or top-level widget) creates a `GenericAction` for a
/// given command (e.g., `generic.copy`), and inserts it in the application
/// menu at the appropriate location (e.g., Edit > Copy).
///
/// 2. Other specific widgets (e.g., `LineEdit`) supporting the generic command
/// create an `Action` (not a `GenericAction`) implementing its specific
/// behavior.
///
/// Under the hood, the `GenericAction` listens to changes in the focus stack,
/// and whenever the `GenericAction` is triggered, if there is a focused widget
/// that has an `Action` implementing the same command, they the
/// `GenericAction` automatically triggers this specific action.
///
class VGC_UI_API GenericAction : public Action {
private:
    VGC_OBJECT(GenericAction, Action)

protected:
    GenericAction(CreateKey, core::StringId commandId);
    GenericAction(CreateKey, core::StringId commandId, std::string_view text);

public:
    /// Creates a `GenericAction`.
    ///
    static GenericActionPtr create(core::StringId commandId);

    /// Creates a `GenericAction` with the given text.
    ///
    static GenericActionPtr create(core::StringId commandId, std::string_view text);

private:
    void init_();

    // Handling changes in which specific action is referred to, and changing
    // in the state of this specific action.

    Action* action_ = nullptr;

    void updateAction_();
    VGC_SLOT(updateActionSlot_, updateAction_);

    void updateActionState_();
    VGC_SLOT(updateActionStateSlot_, updateActionState_);

    // Handling changes of widget tree.

    Widget* widgetRoot_ = nullptr;

    void onWidgetRootChanged_(Widget* widgetRoot);
    VGC_SLOT(onWidgetRootChangedSlot_, onWidgetRootChanged_);

    void onOwningWidgetChanged_(Widget* owningWidget);
    VGC_SLOT(onOwningWidgetChangedSlot_, onOwningWidgetChanged_);

    // Handling triggering the specific action when the generic action is
    // triggered.

    void onTriggered_();
    VGC_SLOT(onTriggeredSlot_, onTriggered_);
};

} // namespace vgc::ui

#endif // VGC_UI_GENERICACTION_H
