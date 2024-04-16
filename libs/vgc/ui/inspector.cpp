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

#include <vgc/ui/inspector.h>

#include <vgc/core/format.h>
#include <vgc/ui/command.h>
#include <vgc/ui/logcategories.h>
#include <vgc/ui/widget.h>
#include <vgc/ui/window.h>

namespace vgc::ui {

namespace {

namespace commands {

using modifierkeys::alt;
using modifierkeys::ctrl;
using modifierkeys::shift;

VGC_UI_DEFINE_WINDOW_COMMAND(
    inspectWidgets,
    "ui.inspectWidgets",
    "Inspect Widgets",
    Shortcut(ctrl | alt | shift, Key::I))

} // namespace commands

} // namespace

Inspector::Inspector(CreateKey key, const ui::ModuleContext& context)
    : Module(key, context) {

    defineAction(commands::inspectWidgets(), onInspect_Slot());
}

InspectorSharedPtr Inspector::create(const ui::ModuleContext& context) {
    return core::createObject<Inspector>(context);
}

namespace {

void widgetSizingInfo(std::string& out, ui::Widget& widget, ui::Widget& root) {

    auto outB = std::back_inserter(out);
    out += widget.objectType().unqualifiedName();

    out += "\nStyle =";
    for (core::StringId styleClass : widget.styleClasses()) {
        out += " ";
        out += styleClass;
    }
    out += "\n";
    core::formatTo(
        outB, "\nPosition       = {}", widget.mapTo(&root, geometry::Vec2f(0, 0)));
    core::formatTo(outB, "\nSize           = {}", widget.size());
    core::formatTo(outB, "\nPreferred Size = {}", widget.preferredSize());
    core::formatTo(outB, "\nMargin         = {}", widget.margin());
    core::formatTo(outB, "\nPadding        = {}", widget.padding());
    core::formatTo(outB, "\nBorder         = {}", widget.border());

    out += "\n\nMatching style rules:\n\n";
    core::StringWriter outS(out);
    widget.debugPrintStyle(outS);
}

} // namespace

void Inspector::onInspect_() {

    auto window = Window::activeWindow().lock();
    if (!window) {
        return;
    }

    auto root = WidgetWeakPtr(window->widget()).lock();
    if (!root) {
        return;
    }

    WidgetWeakPtr widget_ = root;
    std::string out;
    using core::write;
    out.append(80, '=');
    out += "\nPosition and size information about hovered widgets:\n";
    while (auto widget = widget_.lock()) {
        out.append(80, '-');
        out += "\n";
        widgetSizingInfo(out, *widget, *root);
        widget_ = widget->hoverChainChild();
    }
    VGC_DEBUG(LogVgcUi, out);
}

} // namespace vgc::ui
