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

#include <vgc/ui/label.h>

#include <vgc/core/array.h>
#include <vgc/style/strings.h>
#include <vgc/ui/preferredsizecalculator.h>
#include <vgc/ui/strings.h>

#include <vgc/ui/detail/paintutil.h>

namespace vgc::ui {

Label::Label(std::string_view text)
    : Widget()
    , richText_(graphics::RichText::create()) {

    addStyleClass(strings::Label);
    appendChildStylableObject(richText_.get());
    setText(text);
}

LabelPtr Label::create() {
    return LabelPtr(new Label(""));
}

LabelPtr Label::create(std::string_view text) {
    return LabelPtr(new Label(text));
}

void Label::setText(std::string_view text) {
    if (text != richText_->text()) {
        richText_->setText(text);
        reload_ = true;
        requestGeometryUpdate();
        requestRepaint();
    }
}

void Label::onResize() {
    SuperClass::onResize();
    richText_->setRect(contentRect());
    reload_ = true;
}

void Label::onPaintCreate(graphics::Engine* engine) {
    SuperClass::onPaintCreate(engine);
    triangles_ =
        engine->createDynamicTriangleListView(graphics::BuiltinGeometryLayout::XYRGB);
}

void Label::onPaintDraw(graphics::Engine* engine, PaintOptions options) {

    SuperClass::onPaintDraw(engine, options);

    if (reload_) {
        reload_ = false;
        core::FloatArray a;

        // Draw text
        richText_->fill(a);

        // Load triangles data
        engine->updateVertexBufferData(triangles_, std::move(a));
    }
    engine->setProgram(graphics::BuiltinProgram::Simple);
    engine->draw(triangles_);
}

void Label::onPaintDestroy(graphics::Engine* engine) {
    SuperClass::onPaintDestroy(engine);
    triangles_.reset();
}

bool Label::onMouseEnter() {
    reload_ = true;
    requestRepaint();
    return true;
}

bool Label::onMouseLeave() {
    reload_ = true;
    requestRepaint();
    return true;
}

void Label::onStyleChanged() {
    reload_ = true;
    SuperClass::onStyleChanged();
}

geometry::Vec2f Label::computePreferredSize() const {
    PreferredSizeCalculator calc(this);
    calc.add(richText_->preferredSize());
    calc.addPaddingAndBorder();
    return calc.compute();
}

} // namespace vgc::ui
