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

#ifndef VGC_UI_NUMBEREDIT_H
#define VGC_UI_NUMBEREDIT_H

#include <vgc/ui/lineedit.h>

namespace vgc::ui {

VGC_DECLARE_OBJECT(NumberEdit);

class VGC_UI_API NumberEdit : public LineEdit {
private:
    VGC_OBJECT(NumberEdit, LineEdit)

protected:
    /// This is an implementation details. Please use
    /// NumberEdit::create() instead.
    ///
    NumberEdit();

public:
    /// Creates a Label.
    ///
    static NumberEditPtr create();

    /// Sets the value of this `NumberEdit`.
    ///
    void setValue(double v);

    /// Returns the value of this `NumberEdit`.
    ///
    double value() const {
        return value_;
    }

private:
protected:
    // Reimplementation of Widget virtual methods
    bool onMouseEnter() override;
    bool onMouseLeave() override;
    bool onMouseMove(MouseEvent* event) override;
    bool onMousePress(MouseEvent* event) override;
    bool onMouseRelease(MouseEvent* event) override;
    bool onFocusIn(FocusReason reason) override;
    bool onFocusOut(FocusReason reason) override;
    bool onKeyPress(KeyEvent* event) override;

private:
    // Current value
    double value_ = 0;

    // Value before drag or text editing starts
    double oldValue_;

    // Drag mode
    bool isAbsoluteMode_ = true;
    bool isDragInitiated_ = false;
    bool isDragEpsilonReached_ = false;
    bool skipNextMouseMove_ = false;
    geometry::Vec2f mousePositionOnMousePress_;
    float deltaPositionX_ = 0;

    // Text mode
    void setTextFromValue_();
    void setValueFromText_();

    // Switch between modes
    bool isTextMode_ = true;
    void setTextMode_(bool isTextMode);

    // Cursor Handling
    ui::CursorChanger cursorChangerOnMouseHover_;
    ui::CursorChanger cursorChangerOnValueDrag_;
    void updateCursor_();
};

} // namespace vgc::ui

#endif // VGC_UI_NUMBEREDIT_H
