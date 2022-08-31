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

#ifndef VGC_UI_LABEL_H
#define VGC_UI_LABEL_H

#include <string>
#include <string_view>

#include <vgc/graphics/richtext.h>
#include <vgc/ui/widget.h>

namespace vgc::ui {

VGC_DECLARE_OBJECT(Label);

/// \class vgc::ui::Label
/// \brief Widget to display text.
///
class VGC_UI_API Label : public Widget {
private:
    VGC_OBJECT(Label, Widget)

protected:
    /// This is an implementation details. Please use
    /// Label::create(text) instead.
    ///
    Label(std::string_view text);

public:
    /// Creates a Label.
    ///
    static LabelPtr create();

    /// Creates a Label with the given text.
    ///
    static LabelPtr create(std::string_view text);

    /// Returns the label's text.
    ///
    const std::string& text() const {
        return richText_->text();
    }

    /// Sets the label's text.
    ///
    void setText(std::string_view text);

    // Reimplementation of StylableObject virtual methods
    style::StylableObject* firstChildStylableObject() const override;
    style::StylableObject* lastChildStylableObject() const override;

    // Reimplementation of Widget virtual methods
    void onResize() override;
    void onPaintCreate(graphics::Engine* engine) override;
    void onPaintDraw(graphics::Engine* engine, PaintOptions options) override;
    void onPaintDestroy(graphics::Engine* engine) override;
    bool onMouseEnter() override;
    bool onMouseLeave() override;

protected:
    geometry::Vec2f computePreferredSize() const override;

private:
    graphics::RichTextPtr richText_;
    graphics::GeometryViewPtr triangles_;
    bool reload_ = true;
};

} // namespace vgc::ui

#endif // VGC_UI_LABEL_H
