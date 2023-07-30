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

#ifndef VGC_UI_SCROLLEVENT_H
#define VGC_UI_SCROLLEVENT_H

#include <vgc/geometry/vec2f.h>
#include <vgc/ui/mouseevent.h>

namespace vgc::ui {

VGC_DECLARE_OBJECT(ScrollEvent);

/// \class vgc::ui::ScrollEvent
/// \brief Class to handle mouse wheel and trackpad scroll gestures.
///
class VGC_UI_API ScrollEvent : public PropagatedMouseEvent {
private:
    VGC_OBJECT(ScrollEvent, MouseEvent)

protected:
    /// This is an implementation details. Please use
    /// ScrollEvent::create() instead.
    ///
    ScrollEvent(
        CreateKey,
        double timestamp,
        ModifierKeys modifiers,
        const geometry::Vec2f& position,
        const geometry::Vec2f& scrollDelta,
        Int horizontalSteps,
        Int verticalSteps);

public:
    /// Creates a ScrollEvent.
    ///
    static ScrollEventPtr create(
        double timestamp = 0,
        ModifierKeys modifiers = {},
        const geometry::Vec2f& position = {},
        const geometry::Vec2f& scrollDelta = {},
        Int horizontalSteps = 0,
        Int verticalSteps = 0);

    /// Returns the scrolling input delta that caused a scroll event,
    /// in fraction of steps in both axes.
    ///
    /// \sa `horizontalSteps()`, `verticalSteps()`
    ///
    const geometry::Vec2f& scrollDelta() const {
        return scrollDelta_;
    }

    /// Sets the scrolling input delta of this `ScrollEvent` in fraction of steps.
    ///
    /// Event handlers should typically not change this.
    ///
    void setScrollDelta(const geometry::Vec2f& scrollDelta) {
        scrollDelta_ = scrollDelta;
    }

    /// Returns the discrete horizontal scrolling steps.
    ///
    /// This is conveniently provided by the widget system as a ready-to-use
    /// discretization of the continuous scroll delta.
    ///
    /// A widget should use either the continuous scroll delta or the discrete steps.
    /// Using both is redundant and would typically result in twice the amount of scroll.
    ///
    /// \sa `verticalSteps()`, `scrollDelta()`
    ///
    Int horizontalSteps() const {
        return horizontalSteps_;
    }

    /// Sets the discrete horizontal scrolling steps.
    ///
    /// Event handlers should typically not change this.
    ///
    /// \sa `horizontalSteps()`
    ///
    void setHorizontalSteps(Int horizontalSteps) {
        horizontalSteps_ = horizontalSteps;
    }

    /// Returns the discrete vertical scrolling steps.
    ///
    /// This is conveniently provided by the widget system as a ready-to-use
    /// discretization of the continuous scroll delta.
    ///
    /// A widget should use either the continuous scroll delta or the discrete steps.
    /// Using both is redundant and would typically result in twice the amount of scroll.
    ///
    /// \sa `horizontalSteps()`, `scrollDelta()`
    ///
    Int verticalSteps() const {
        return verticalSteps_;
    }

    /// Sets the discrete vertical scrolling steps.
    ///
    /// Event handlers should typically not change this.
    ///
    /// \sa `verticalSteps()`
    ///
    void setVerticalSteps(Int verticalSteps) {
        verticalSteps_ = verticalSteps;
    }

private:
    geometry::Vec2f scrollDelta_ = {};
    Int horizontalSteps_ = 0;
    Int verticalSteps_ = 0;
};

} // namespace vgc::ui

#endif // VGC_UI_SCROLLEVENT_H
