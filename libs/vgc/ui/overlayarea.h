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

#ifndef VGC_UI_OVERLAYAREA_H
#define VGC_UI_OVERLAYAREA_H

#include <vgc/ui/widget.h>

namespace vgc::ui {

VGC_DECLARE_OBJECT(OverlayArea);

/// \class vgc::ui::OverlayArea
/// \brief Allows the area of a widget to be overlaid by other
/// widgets.
///
class VGC_UI_API OverlayArea : public Widget {
private:
    VGC_OBJECT(OverlayArea, Widget)

protected:
    /// This is an implementation details. Please use
    /// OverlayArea::create() instead.
    ///
    OverlayArea();

public:
    /// Creates an overlay area.
    ///
    static OverlayAreaPtr create();

    void setAreaWidget(Widget* w);
    void addOverlayWidget(Widget* w);

    template<typename WidgetClass, typename... Args>
    WidgetClass* createAreaWidget(Args&&... args) {
        core::ObjPtr<WidgetClass> child =
            WidgetClass::create(std::forward<Args>(args)...);
        setAreaWidget(child.get());
        return child.get();
    }

    template<typename WidgetClass, typename... Args>
    WidgetClass* createOverlayWidget(Args&&... args) {
        core::ObjPtr<WidgetClass> child =
            WidgetClass::create(std::forward<Args>(args)...);
        addOverlayWidget(child.get());
        return child.get();
    }

    // reimpl
    float preferredWidthForHeight(float height) const override;
    float preferredHeightForWidth(float width) const override;

protected:
    void onWidgetAdded(Widget* child) override;
    void onWidgetRemoved(Widget* child) override;
    geometry::Vec2f computePreferredSize() const override;
    void updateChildrenGeometry() override;

private:
    Widget* areaWidget_ = nullptr;
};

} // namespace vgc::ui

#endif // VGC_UI_OVERLAYAREA_H
