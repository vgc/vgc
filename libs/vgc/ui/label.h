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

#include <vgc/ui/widget.h>

namespace vgc {
namespace ui {

VGC_DECLARE_OBJECT(Label);

/// \class vgc::ui::Label
/// \brief Widget to display text.
///
class VGC_UI_API Label : public Widget {
private:
    VGC_OBJECT(Label, Widget)

protected:
    /// This is an implementation details. Please use
    /// Label::create() instead.
    ///
    Label();

    /// This is an implementation details. Please use
    /// Label::create(text) instead.
    ///
    Label(const std::string& text);

public:
    /// Creates a Label.
    ///
    static LabelPtr create();

    /// Creates a Label with the given text.
    ///
    static LabelPtr create(const std::string& text);

    /// Returns the label's text.
    ///
    const std::string& text() const {
        return text_;
    }

    /// Sets the label's text.
    ///
    void setText(const std::string& text);

    // reimpl
    void onPaintCreate(graphics::Engine* engine) override;
    void onPaintDraw(graphics::Engine* engine, PaintFlags flags) override;
    void onPaintDestroy(graphics::Engine* engine) override;

private:
    std::string text_;
    graphics::GeometryViewPtr triangles_;
    bool reload_;
};

} // namespace ui
} // namespace vgc

#endif // VGC_UI_LABEL_H
