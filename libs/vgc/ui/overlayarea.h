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

enum class OverlayResizePolicy {
    None,
    //Fill,
    Stretch,
    //Center,
    //PositionStretch,
    //Suicide,
};

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
    OverlayArea(CreateKey);

public:
    /// Creates an overlay area.
    ///
    static OverlayAreaPtr create();

    /// Returns the area widget of this overlay area. This is the only child of
    /// the overlay area that is not actually an overlay, but instead is the
    /// widget that fills the space of the overlay area, below all overlays.
    ///
    /// \sa `setAreaWidget()`, `createAreaWidget()`
    ///
    Widget* areaWidget() const {
        return areaWidget_;
    }

    /// Sets the given widget as the area widget of this overlay area.
    ///
    /// \sa `areaWidget()`, `createAreaWidget()`
    ///
    void setAreaWidget(Widget* widget);

    /// Creates a new widget of the given `WidgetClass`, and sets it
    /// as the area widget of this overlay area.
    ///
    /// \sa `areaWidget()`, `setAreaWidget()`
    ///
    template<typename WidgetClass, typename... Args>
    WidgetClass* createAreaWidget(Args&&... args) {
        core::ObjPtr<WidgetClass> child =
            WidgetClass::create(std::forward<Args>(args)...);
        setAreaWidget(child.get());
        return child.get();
    }

    /// Adds the given widget as an overlay to this overlay area.
    ///
    /// \sa `createOverlayWidget()`.
    ///
    void addOverlayWidget(
        Widget* widget,
        OverlayResizePolicy resizePolicy = OverlayResizePolicy::None);

    /// Creates a new widget of the given `WidgetClass`, and adds it as an
    /// overlay to this overlay area.
    ///
    /// \sa `addOverlayWidget()`.
    ///
    template<typename WidgetClass, typename... Args>
    WidgetClass* createOverlayWidget(OverlayResizePolicy resizePolicy, Args&&... args) {
        core::ObjPtr<WidgetClass> child =
            WidgetClass::create(std::forward<Args>(args)...);
        addOverlayWidget(child.get(), resizePolicy);
        return child.get();
    }

    // reimpl
    float preferredWidthForHeight(float height) const override;
    float preferredHeightForWidth(float width) const override;

protected:
    void onResize() override;
    void onWidgetAdded(Widget* child, bool wasOnlyReordered) override;
    void onWidgetRemoved(Widget* child) override;
    geometry::Vec2f computePreferredSize() const override;
    void updateChildrenGeometry() override;

private:
    Widget* areaWidget_ = nullptr;

    class OverlayDesc {
    public:
        OverlayDesc(Widget* widget, OverlayResizePolicy resizePolicy)
            : widget_(widget)
            , resizePolicy_(resizePolicy) {
        }

        Widget* widget() const {
            return widget_;
        }

        OverlayResizePolicy resizePolicy() const {
            return resizePolicy_;
        }

        bool isGeometryDirty() const {
            return isGeometryDirty_;
        }

        void setGeometryDirty(bool isDirty) {
            isGeometryDirty_ = isDirty;
        }

    private:
        Widget* widget_ = nullptr;
        OverlayResizePolicy resizePolicy_ = {};
        bool isGeometryDirty_ = true;
    };

    // order does not matter
    core::Array<OverlayDesc> overlays_ = {};
};

} // namespace vgc::ui

#endif // VGC_UI_OVERLAYAREA_H
