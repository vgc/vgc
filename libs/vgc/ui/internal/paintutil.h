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

#ifndef VGC_UI_INTERNAL_PAINTUTIL_H
#define VGC_UI_INTERNAL_PAINTUTIL_H

#include <string>

#include <vgc/core/array.h>
#include <vgc/core/color.h>
#include <vgc/core/stringid.h>
#include <vgc/graphics/text.h>
#include <vgc/ui/widget.h>

namespace vgc {
namespace ui {
namespace internal {

void insertTriangle(
        core::FloatArray& a,
        float r, float g, float b,
        float x1, float y1,
        float x2, float y2,
        float x3, float y3);

void insertRect(
        core::FloatArray& a,
        float r, float g, float b,
        float x1, float y1, float x2, float y2);

void insertRect(
        core::FloatArray& a,
        const core::Color& c,
        float x1, float y1, float x2, float y2,
        float borderRadius);

void insertRect(
        core::FloatArray& a,
        const core::Color& c,
        float x1, float y1, float x2, float y2);

void insertText(
        core::FloatArray& a,
        const core::Color& c,
        float x1, float y1, float x2, float y2,
        float paddingLeft, float paddingRight, float paddingTop, float paddingBottom,
        const std::string& text,
        const graphics::TextProperties& textProperties,
        const graphics::TextCursor& textCursor,
        bool hinting,
        float scrollLeft = 0.0f);

void insertText(
        core::FloatArray& a,
        const core::Color& c,
        float x1, float y1, float x2, float y2,
        float paddingLeft, float paddingRight, float paddingTop, float paddingBottom,
        const graphics::ShapedText& shapedText,
        const graphics::TextProperties& textProperties,
        const graphics::TextCursor& textCursor,
        bool hinting,
        float scrollLeft = 0.0f);

core::Color getColor(const Widget* widget, core::StringId property);

float getLength(const Widget* widget, core::StringId property);

// Note: we don't use default arguments to avoid recompiling everything
// when we want to change them for testing
graphics::SizedFont* getDefaultSizedFont();
graphics::SizedFont* getDefaultSizedFont(Int ppem);
graphics::SizedFont* getDefaultSizedFont(Int ppem, graphics::FontHinting hinting);

} // namespace internal
} // namespace ui
} // namespace vgc

#endif // VGC_UI_INTERNAL_PAINTUTIL_H
