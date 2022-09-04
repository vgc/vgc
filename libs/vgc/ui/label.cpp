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
#include <vgc/graphics/strings.h>
#include <vgc/ui/strings.h>

#include <vgc/ui/detail/paintutil.h>

namespace vgc::ui {

Label::Label(std::string_view text)
    : Widget()
    , richText_(graphics::RichText::create()) {

    addStyleClass(strings::Label);
    setText(text);
    richText_->setParentStylableObject(this);
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
        requestRepaint();
    }
}

style::StylableObject* Label::firstChildStylableObject() const {
    return richText_.get();
}

style::StylableObject* Label::lastChildStylableObject() const {
    return richText_.get();
}

void Label::onResize() {

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

void Label::onPaintCreate(graphics::Engine* engine) {
    triangles_ =
        engine->createDynamicTriangleListView(graphics::BuiltinGeometryLayout::XYRGB);
}

void Label::onPaintDraw(graphics::Engine* engine, PaintOptions /*options*/) {

    namespace gs = graphics::strings;

    if (reload_) {
        reload_ = false;
        core::FloatArray a;

        // Draw background
        core::Color backgroundColor = detail::getColor(this, gs::background_color);
        if (backgroundColor.a() > 0) {
            style::BorderRadiuses borderRadiuses = detail::getBorderRadiuses(this);
            detail::insertRect(a, backgroundColor, rect(), borderRadiuses);
        }

        // Draw text
        richText_->fill(a);

        // Load triangles data
        engine->updateVertexBufferData(triangles_, std::move(a));
    }
    engine->setProgram(graphics::BuiltinProgram::Simple);
    engine->draw(triangles_, -1, 0);
}

void Label::onPaintDestroy(graphics::Engine*) {
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

geometry::Vec2f Label::computePreferredSize() const {

    namespace gs = graphics::strings;

    PreferredSizeType auto_ = PreferredSizeType::Auto;
    PreferredSize w = preferredWidth();
    PreferredSize h = preferredHeight();

    geometry::Vec2f res(0, 0);
    geometry::Vec2f textPreferredSize(0, 0);
    if (w.type() == auto_ || h.type() == auto_) {
        textPreferredSize = richText_->preferredSize();
    }
    if (w.type() == auto_) {
        float paddingLeft = detail::getLength(this, gs::padding_left);
        float paddingRight = detail::getLength(this, gs::padding_right);
        res[0] = textPreferredSize[0] + paddingLeft + paddingRight;
    }
    else {
        res[0] = w.value();
    }
    if (h.type() == auto_) {
        float paddingTop = detail::getLength(this, gs::padding_top);
        float paddingBottom = detail::getLength(this, gs::padding_bottom);
        res[1] = textPreferredSize[1] + paddingTop + paddingBottom;
    }
    else {
        res[1] = h.value();
    }
    return res;
}

} // namespace vgc::ui
