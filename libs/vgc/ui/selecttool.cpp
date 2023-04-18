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

#include <vgc/ui/selecttool.h>

namespace vgc::ui {

SelectTool::SelectTool()
    : CanvasTool() {
}

SelectToolPtr SelectTool::create() {
    return SelectToolPtr(new SelectTool());
}

bool SelectTool::onMouseMove(MouseEvent* event) {
    return false;
}

bool SelectTool::onMousePress(MouseEvent* event) {


    ui::Canvas* canvas = this->canvas();
    if (!canvas) {
        return false;
    }

    if (event->button() == MouseButton::Left) {
        if (event->modifierKeys() == ModifierKey::Alt) {
            canvas->selectAlternativeAtPosition(event->position());
        }
        else {
            canvas->selectAtPosition(event->position());
        }
        return true;
    }

    return false;
}

bool SelectTool::onMouseRelease(MouseEvent* event) {
    return false;
}

} // namespace vgc::ui
