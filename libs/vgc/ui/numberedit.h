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

public:
    //Setter
    void setValue(double v);

    //Getter
    double value() const {
        return value_;
    }
    bool onMouseEnter() override;
    bool onMouseLeave() override;
    bool onMouseMove(MouseEvent* event) override;
    bool onMousePress(MouseEvent* event) override;
    bool onMouseRelease(MouseEvent* event) override;

private:
    double value_ = 0;
    double valueOnMousePress_;
    float positionX_;
    bool isMousePressed_ = false;
    bool isSettingCursorPosition_ = false;
    float deltaMousePostion_;
    geometry::Vec2f globalCursorPositionOnPress_;

    ui::CursorChanger cursorChangerOnMouseHover_;
    ui::CursorChanger cursorChangerOnValueDrag_;

    void setTextFromValue_();
};

} // namespace vgc::ui

#endif // VGC_UI_NUMBEREDIT_H