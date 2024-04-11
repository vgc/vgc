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

namespace detail {

VGC_DECLARE_OBJECT(ModalBackdrop);

}

enum class OverlayModalPolicy {

    /// The overlay is not modal: users can still interact with other widgets
    /// in the application.
    ///
    NotModal,

    /// The overlay is modal and clicking outside the overlay does nothing.
    ///
    PersistentModal,

    /// The overlay is modal and clicking outside the overlay closes it.
    ///
    TransientModal
};

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
    WidgetWeakPtr areaWidget() const {
        return areaWidget_;
    }

    /// Sets the given widget as the area widget of this overlay area.
    ///
    /// \sa `areaWidget()`, `createAreaWidget()`
    ///
    void setAreaWidget(WidgetWeakPtr widget);

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
        WidgetWeakPtr widget,
        OverlayModalPolicy modalPolicy = OverlayModalPolicy::NotModal,
        OverlayResizePolicy resizePolicy = OverlayResizePolicy::None);

    /// Creates a new widget of the given `WidgetClass`, and adds it as an
    /// overlay to this overlay area.
    ///
    /// \sa `addOverlayWidget()`.
    ///
    template<typename WidgetClass, typename... Args>
    WidgetClass* createOverlayWidget(
        OverlayModalPolicy modalPolicy,
        OverlayResizePolicy resizePolicy,
        Args&&... args) {

        core::ObjPtr<WidgetClass> child =
            WidgetClass::create(std::forward<Args>(args)...);
        addOverlayWidget(child.get(), modalPolicy, resizePolicy);
        return child.get();
    }

    /// Removes the given `overlay` from this `OverlayArea`.
    ///
    /// If `overlay` was the last modal overlay of this `OverlayArea`, then the
    /// modal background is also removed, making it possible again to interact
    /// with widgets in the `areaWidget()`.
    ///
    /// If the given `overlay` was not in the list of overlays, then this function
    /// returns a null pointer.
    ///
    /// Otherwise, it returns a shared pointer to the now-parentless widget.
    ///
    WidgetSharedPtr removeOverlay(WidgetWeakPtr overlay);

    /// Allows the given `passthrough` widget to be accessible even if
    /// `overlay` is a modal overlay. In other words, this makes mouse events
    /// "pass through" the modal background.
    ///
    /// An example is the main menubar of the application: when one of its
    /// submenu is open, we still want the menubar to accept mouse events, for
    /// example to allow users to open other submenus by simply moving the
    /// mouse. Without adding the menubar as passthrough for the submenu
    /// overlay, this would not work, since by the submenu is a modal overlay
    /// and would by default prevent interaction with all other application
    /// widgets.
    ///
    void addPassthroughFor(WidgetWeakPtr overlay, WidgetWeakPtr passthrough);

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
    WidgetWeakPtr areaWidget_ = nullptr;
    detail::ModalBackdropWeakPtr modalBackdrop_ = nullptr;

    bool hasModalOverlays_() const;
    void addModalBackdropIfNeeded_();
    void removeModalBackdropIfUnneeded_();
    WidgetSharedPtr getFirstTransientModal_() const;
    void onModalBackdropClicked_();
    VGC_SLOT(onModalBackdropClicked_)

    class OverlayDesc {
    public:
        OverlayDesc(
            WidgetWeakPtr widget,
            OverlayModalPolicy modalPolicy,
            OverlayResizePolicy resizePolicy)

            : widget_(widget)
            , modalPolicy_(modalPolicy)
            , resizePolicy_(resizePolicy) {
        }

        const WidgetWeakPtr& widget() const {
            return widget_;
        }

        OverlayModalPolicy modalPolicy() const {
            return modalPolicy_;
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
        WidgetWeakPtr widget_;
        OverlayModalPolicy modalPolicy_ = OverlayModalPolicy::NotModal;
        OverlayResizePolicy resizePolicy_ = OverlayResizePolicy::None;
        bool isGeometryDirty_ = true;
    };

    // order does not matter
    core::Array<OverlayDesc> overlays_ = {};
};

} // namespace vgc::ui

#endif // VGC_UI_OVERLAYAREA_H
