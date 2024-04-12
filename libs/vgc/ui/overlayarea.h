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

/// \enum vgc::ui::OverlayModality
/// \brief Specifies whether an overlay is modeless or modal (weak/strong).
///
/// A *modal overlay* is a type of overlay that prevents users from interacting
/// with other widgets in the application. This is achieved via an invisible
/// *modal backdrop* automatically created behind the overlay, covering the
/// rest of the application.
///
/// An overlay which is not modal is called `Modeless`.
///
/// An overlay which is modal can either be `Weak` or `Strong`.
///
/// A `Weak` modal overlay is automatically closed when clicking outside the
/// overlay, which is typically a good choice for dropdown menus.
///
/// A `Strong` modal overlay stays visible until the user explicitly closes
/// them via in-overlay interaction, such as clicking an "OK" or "Cancel"
/// button inside a modal dialog. Clicking outside a `Strong` modal overlay
/// does nothing.
///
enum class OverlayModality {

    /// The overlay is not modal: users can still interact with other widgets
    /// in the application.
    ///
    Modeless,

    /// The overlay is modal and clicking outside the overlay closes it.
    ///
    Weak,

    /// The overlay is modal and clicking outside the overlay does nothing.
    ///
    Strong
};

/// \enum vgc::ui::OverlayResizePolicy
/// \brief Specifies how should an overlay react when the OverlayArea is resized.
///
enum class OverlayResizePolicy {
    None,
    //Fill,
    Stretch,
    //Center,
    //PositionStretch,
    //Suicide,
};

namespace detail {

VGC_DECLARE_OBJECT(ModalBackdrop);

class Overlay {
public:
    Overlay(
        WidgetWeakPtr widget,
        OverlayModality modality,
        OverlayResizePolicy resizePolicy)

        : widget_(widget)
        , modality_(modality)
        , resizePolicy_(resizePolicy) {
    }

    const WidgetWeakPtr& widget() const {
        return widget_;
    }

    OverlayModality modality() const {
        return modality_;
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
    OverlayModality modality_ = OverlayModality::Modeless;
    OverlayResizePolicy resizePolicy_ = OverlayResizePolicy::None;
    bool isGeometryDirty_ = true;
};

} // namespace detail

VGC_DECLARE_OBJECT(OverlayArea);

/// \class vgc::ui::OverlayArea
/// \brief Allows a widget to be overlaid by other widgets.
///
class VGC_UI_API OverlayArea : public Widget {
private:
    VGC_OBJECT(OverlayArea, Widget)

protected:
    OverlayArea(CreateKey);

public:
    /// Creates an `OverlayArea`.
    ///
    static OverlayAreaPtr create();

    /// Returns the body widget of this overlay area, if any. This is the only
    /// child of the overlay area that is not actually an overlay, but instead
    /// is a widget that fills the whole space of the overlay area, below all
    /// overlays.
    ///
    /// \sa `setBody()`, `createBody()`.
    ///
    WidgetWeakPtr body() const {
        return body_;
    }

    /// Sets the given `widget` as body of this overlay area.
    ///
    /// \sa `body()`, `createBody()`.
    ///
    void setBody(WidgetWeakPtr widget);

    /// Creates a new widget of the given `WidgetClass`, and sets it
    /// as body of this overlay area.
    ///
    /// \sa `body()`, `setBody()`.
    ///
    template<typename WidgetClass, typename... Args>
    WidgetClass* createBody(Args&&... args) {
        core::ObjPtr<WidgetClass> child =
            WidgetClass::create(std::forward<Args>(args)...);
        setBody(child.get());
        return child.get();
    }

    /// Adds the given `widget` as an overlay to this overlay area.
    ///
    /// \sa `createOverlay()`.
    ///
    void addOverlay(
        WidgetWeakPtr widget,
        OverlayModality modality = OverlayModality::Modeless,
        OverlayResizePolicy resizePolicy = OverlayResizePolicy::None);

    /// Creates a new widget of the given `WidgetClass`, and adds it as an
    /// overlay to this overlay area.
    ///
    /// \sa `addOverlay()`.
    ///
    template<typename WidgetClass, typename... Args>
    WidgetClass* createOverlay(
        OverlayModality modality,
        OverlayResizePolicy resizePolicy,
        Args&&... args) {

        core::ObjPtr<WidgetClass> child =
            WidgetClass::create(std::forward<Args>(args)...);
        addOverlay(child.get(), modality, resizePolicy);
        return child.get();
    }

    /// Removes the given `overlay` from this `OverlayArea`.
    ///
    /// If `overlay` was the last modal overlay of this `OverlayArea`, then the
    /// modal backdrop is also removed, making it possible again to interact
    /// with widgets in the `body()`.
    ///
    /// If the given `overlay` was not in the list of overlays, then this function
    /// returns a null pointer.
    ///
    /// Otherwise, it returns a shared pointer to the now-parentless widget.
    ///
    WidgetSharedPtr removeOverlay(WidgetWeakPtr overlay);

    /// Allows the given `passthrough` widget to be accessible even if
    /// `overlay` is a modal overlay. In other words, this makes mouse events
    /// "pass through" the modal backdrop.
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
    WidgetWeakPtr body_;
    detail::ModalBackdropWeakPtr modalBackdrop_;
    core::Array<detail::Overlay> overlays_; // order may differ from child widgets

    bool hasModalOverlays_() const;
    void addModalBackdropIfNeeded_();
    void removeModalBackdropIfUnneeded_();
    WidgetSharedPtr getFirstWeakOverlay_() const;
    void onModalBackdropClicked_();
    VGC_SLOT(onModalBackdropClicked_)
};

} // namespace vgc::ui

#endif // VGC_UI_OVERLAYAREA_H
