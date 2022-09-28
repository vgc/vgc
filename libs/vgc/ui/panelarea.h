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

#ifndef VGC_UI_PANELAREA_H
#define VGC_UI_PANELAREA_H

#include <vgc/ui/widget.h>

namespace vgc::ui {

/// \enum vgc::ui::PanelAreaType
/// \brief The type of a PanelArea
///
enum class PanelAreaType : UInt8 {
    HorizontalSplit = 0,
    VerticalSplit = 1,
    Tabs = 2
};

VGC_DECLARE_OBJECT(PanelArea);

namespace detail {

struct PanelAreaSplitData {

    PanelAreaSplitData(PanelArea* childArea, float position, float size)
        : childArea(childArea)
        , position(position)
        , size(size) {
    }

    PanelArea* childArea;
    float position;
    float size;

    // hinted values
    float hPosition;
    float hSize;

    bool isInteractive = true;

    //
    // todo: gap/padding, isInteractive, lastVisibleSize,
    //       animatedPosition, animatedSize...
};

using PanelAreaSplitDataArray = core::Array<PanelAreaSplitData>;

} // namespace detail

/// \class vgc::ui::PanelArea
/// \brief Splits the workspace into different areas where to place Panels.
///
class VGC_UI_API PanelArea : public Widget {
private:
    VGC_OBJECT(PanelArea, Widget)

protected:
    PanelArea(PanelAreaType type);

public:
    /// Creates a `PanelArea`.
    ///
    static PanelAreaPtr create(PanelAreaType type);

    /// Creates a `PanelArea` of type `HorizontalSplit` as a child of the given `parent`.
    ///
    static PanelArea* createHorizontalSplit(Widget* parent) {
        return parent->createChild<PanelArea>(PanelAreaType::HorizontalSplit);
    }

    /// Creates a `PanelArea` of type `VerticalSplit` as a child of the given `parent`.
    ///
    static PanelArea* createVerticalSplit(Widget* parent) {
        return parent->createChild<PanelArea>(PanelAreaType::VerticalSplit);
    }

    /// Creates a `PanelArea` of type `Tabs` as a child of the given `parent`.
    ///
    static PanelArea* createTabs(Widget* parent) {
        return parent->createChild<PanelArea>(PanelAreaType::Tabs);
    }

    /// Returns the type of this `PanelArea`.
    ///
    PanelAreaType type() const {
        return type_;
    }

    /// Returns whether the `type()` of this `PanelArea` is `HorizontalSplit` or
    /// `VerticalSplit`.
    ///
    bool isSplit() const {
        return static_cast<UInt8>(type()) < 2;
    }

protected:
    // Reimplementation of Widget protected virtual methods
    // XXX Some of these are currently public in Widget, but we should
    //     probably make them protected for consistency
    void onWidgetAdded(Widget* child, bool wasOnlyReordered) override;
    void onWidgetRemoved(Widget* child) override;
    bool onMouseEnter() override;
    bool onMouseLeave() override;
    Widget* computeHoverChainChild(MouseEvent* event) const override;
    void preMouseMove(MouseEvent* event) override;
    bool onMouseMove(MouseEvent* event) override;
    bool onMousePress(MouseEvent* event) override;
    bool onMouseRelease(MouseEvent* event) override;
    void onResize() override;
    void updateChildrenGeometry() override;
    void onPaintCreate(graphics::Engine* engine) override;
    void onPaintDraw(graphics::Engine* engine, PaintOptions options) override;
    void onPaintDestroy(graphics::Engine* engine) override;

private:
    using SplitData = detail::PanelAreaSplitData;
    using SplitDataArray = detail::PanelAreaSplitDataArray;

    PanelAreaType type_;
    SplitDataArray splitData_; // size = num child widgets

    graphics::GeometryViewPtr triangles_;
    Int hoveredSplitHandle_ = -1;
    Int draggedSplitHandle_ = -1;
    float dragStartMousePosition_;
    float dragStartSplitPosition_;
    float dragStartSplitSizeAfter_;
    float dragStartSplitSizeBefore_;

    bool isUpdatingSplitData_ = false;
    void onChildrenChanged_();
    Int computeHoveredSplitHandle_(const geometry::Vec2f& position) const;
    void updateHoveredSplitHandle_(const geometry::Vec2f& position);
    void setHoveredSplitHandle_(Int index);
    void startDragging_(const geometry::Vec2f& position);
    void continueDragging_(const geometry::Vec2f& position);
    void stopDragging_(const geometry::Vec2f& position);
};

} // namespace vgc::ui

#endif // VGC_UI_PANELAREA_H
