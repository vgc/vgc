// Copyright 2022 The VGC Developers
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

#include <vgc/ui/preferredsizecalculator.h>

#include <vgc/graphics/strings.h>
#include <vgc/style/strings.h>
#include <vgc/style/types.h>
#include <vgc/ui/widget.h>

namespace vgc::ui {

namespace detail {

void LengthContributions::add(
    const style::Metrics& metrics,
    const style::StyleValue& value,
    float count) {

    if (value.has<style::LengthOrPercentage>()) {
        style::LengthOrPercentage l = value.to<style::LengthOrPercentage>();
        if (l.isPercentage()) {
            addRelative(l.valuef(), count);
        }
        else {
            float dummyRefLength = 1.0f;
            addAbsolute(l.toPx(metrics.scaleFactor(), dummyRefLength), count);
        }
    }
    else if (value.has<style::Length>()) {
        style::Length l = value.to<style::Length>();
        addAbsolute(l.toPx(metrics.scaleFactor()), count);
    }
}

float LengthContributions::compute() const {
    constexpr float maxRelative = 0.99f;
    float r = core::clamp(relative_, 0.f, maxRelative);
    return absolute_ / (1.0f - r);
}

} // namespace detail

namespace {

bool isHinted(const Widget* widget) {
    namespace gs = graphics::strings;
    return widget->style(gs::pixel_hinting) == gs::normal;
}

} // namespace

PreferredSizeCalculator::PreferredSizeCalculator(const Widget* widget)
    : widget_(widget)
    , preferredWidth_(widget->preferredWidth())
    , preferredHeight_(widget->preferredHeight())
    , hint_(isHinted(widget)) {
}

void PreferredSizeCalculator::addPaddingAndBorder() {
    addWidth(style::strings::padding_left);
    addWidth(style::strings::padding_right);
    addWidth(style::strings::border_width, 2);
    addHeight(style::strings::padding_top);
    addHeight(style::strings::padding_bottom);
    addHeight(style::strings::border_width, 2);
}

namespace {

float compute_(
    const style::Metrics& metrics,
    const PreferredSize& preferredSize,
    const detail::LengthContributions& contributions) {

    // TODO: Change PreferredSize to LengthOrAuto, and use the toPx() function
    if (preferredSize.isAuto()) {
        return contributions.compute();
    }
    else {
        return metrics.scaleFactor() * preferredSize.value();
    }
}

} // namespace

geometry::Vec2f PreferredSizeCalculator::compute() const {
    const style::Metrics& metrics = widget_->styleMetrics();
    geometry::Vec2f res(
        compute_(metrics, preferredWidth_, widthContributions_),
        compute_(metrics, preferredHeight_, heightContributions_));
    if (hint_) {
        res[0] = std::round(res[0]);
        res[1] = std::round(res[1]);
    }
    return res;

    // TODO: also hint in the add() functions.
}

} // namespace vgc::ui
