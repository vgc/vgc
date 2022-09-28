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

#ifndef VGC_UI_POPUPLAYER_H
#define VGC_UI_POPUPLAYER_H

#include <vgc/core/innercore.h>
#include <vgc/ui/api.h>
#include <vgc/ui/widget.h>

namespace vgc::ui {

VGC_DECLARE_OBJECT(PopupLayer);

/// \class vgc::ui::PopupLayer
/// \brief A helper widget for popup initiators.
///
/// PopupLayer is a widget that covers an overlay area and prevent
/// clicks from reaching underneath except for a given underlying widget.
/// It also destroys itself if the underlying widget is destroyed.
///
class VGC_UI_API PopupLayer : public Widget {
private:
    VGC_OBJECT(PopupLayer, Widget)

protected:
    PopupLayer(Widget* underlyingWidget);

public:
    /// Creates a PopupLayer.
    ///
    static PopupLayerPtr create();

    /// Creates a PopupLayer with the given `underlyingWidget`.
    ///
    static PopupLayerPtr create(Widget* underlyingWidget);

    Widget* underlyingWidget() const {
        return underlyingWidget_;
    }

    void close();

    /// This signal is emitted when the layer is resized.
    VGC_SIGNAL(resized);

    /// This signal is emitted when a click happens in the layer but not in
    /// any child nor the underlying widget.
    ///
    VGC_SIGNAL(backgroundPressed);

protected:
    // Reimplementation of Widget virtual methods
    void onWidgetAdded(Widget* child, bool wasOnlyReordered) override;
    void onResize() override;
    Widget* computeHoverChainChild(const geometry::Vec2f& position) const override;
    bool onMousePress(MouseEvent* event) override;

private:
    Widget* underlyingWidget_ = nullptr;

    VGC_SLOT(onUnderlyingWidgetAboutToBeDestroyedSlot_, destroy);
};

} // namespace vgc::ui

#endif // VGC_UI_POPUPLAYER_H
