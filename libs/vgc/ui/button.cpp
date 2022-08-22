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

#include <vgc/ui/button.h>

#include <vgc/core/array.h>
#include <vgc/graphics/strings.h>
#include <vgc/ui/strings.h>

#include <vgc/ui/detail/paintutil.h>

namespace vgc::ui {

Button::Button(std::string_view text)
    : Widget()
    , richText_(graphics::RichText::create()) {

    setText(text);
    richText_->setParentStylableObject(this);
    addStyleClass(strings::Button);
}

ButtonPtr Button::create() {
    return ButtonPtr(new Button(""));
}

ButtonPtr Button::create(std::string_view text) {
    return ButtonPtr(new Button(text));
}

void Button::setText(std::string_view text) {
    if (text != richText_->text()) {
        richText_->setText(text);
        reload_ = true;
        repaint();
    }
}

style::StylableObject* Button::firstChildStylableObject() const {
    return richText_.get();
}

style::StylableObject* Button::lastChildStylableObject() const {
    return richText_.get();
}

void Button::onResize() {

    namespace gs = graphics::strings;

    // Compute contentRect
    // TODO: move to Widget::contentRect()
    float paddingLeft = detail::getLength(this, gs::padding_left);
    float paddingRight = detail::getLength(this, gs::padding_right);
    float paddingTop = detail::getLength(this, gs::padding_top);
    float paddingBottom = detail::getLength(this, gs::padding_bottom);
    geometry::Rect2f r = rect();
    geometry::Vec2f pMinOffset(paddingLeft, paddingTop);
    geometry::Vec2f pMaxOffset(paddingRight, paddingBottom);
    geometry::Rect2f contentRect(r.pMin() + pMinOffset, r.pMax() - pMaxOffset);

    // Set appropriate size for the RichText
    richText_->setRect(contentRect);

    reload_ = true;
}

void Button::onPaintCreate(graphics::Engine* engine) {
    triangles_ =
        engine->createDynamicTriangleListView(graphics::BuiltinGeometryLayout::XYRGB);
}

void Button::onPaintDraw(graphics::Engine* engine, PaintOptions /*options*/) {

    namespace gs = graphics::strings;

    if (reload_) {
        reload_ = false;
        core::FloatArray a;

        // Draw background
        core::Color backgroundColor = detail::getColor(
            this, isHovered_ ? gs::background_color_on_hover : gs::background_color);
        float borderRadius = detail::getLength(this, gs::border_radius);
        if (backgroundColor.a() > 0) {
            detail::insertRect(a, backgroundColor, 0, 0, width(), height(), borderRadius);
        }

        // Draw text
        richText_->fill(a);

        // Load triangles data
        engine->updateVertexBufferData(triangles_, std::move(a));
    }
    engine->setProgram(graphics::BuiltinProgram::Simple);
    engine->draw(triangles_, -1, 0);
}

void Button::onPaintDestroy(graphics::Engine*) {
    triangles_.reset();
}

bool Button::onMouseMove(MouseEvent* /*event*/) {
    return true;
}

bool Button::onMousePress(MouseEvent* /*event*/) {
    return true;
}

bool Button::onMouseRelease(MouseEvent* /*event*/) {
    return true;
}

bool Button::onMouseEnter() {
    isHovered_ = true;
    reload_ = true;
    repaint();
    return true;
}

bool Button::onMouseLeave() {
    isHovered_ = false;
    reload_ = true;
    repaint();
    return true;
}

geometry::Vec2f Button::computePreferredSize() const {
    PreferredSizeType auto_ = PreferredSizeType::Auto;
    PreferredSize w = preferredWidth();
    PreferredSize h = preferredHeight();
    geometry::Vec2f res(0, 0);
    if (w.type() == auto_) {
        res[0] = 5;
        // TODO: compute appropriate width based on text length
    }
    else {
        res[0] = w.value();
    }
    if (h.type() == auto_) {
        res[1] = 5;
        // TODO: compute appropriate height based on font size?
    }
    else {
        res[1] = h.value();
    }
    return res;
}

} // namespace vgc::ui
