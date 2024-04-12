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

#include <vgc/ui/overlayarea.h>

namespace vgc::ui {

namespace detail {

// A ModalBackdrop is a widget that covers an overlay area and prevents clicks
// from reaching underneath except for a given passthrough widget.
//
// In the future, we may also want to enable making this backdrop semi-opaque
// to hide the underneath content as visual clue that they are not clickable
// anymore.
//
// Example:
//
//   .ModalBackdrop.weak {
//       background-color: rgba(0, 0, 0, 0);
//   }
//
//   .ModalBackdrop.strong {
//       background-color: rgba(0, 0, 0, 0.5);
//   }
//
// Or equivalently, to keep the class ModalBackdrop private (although this
// would be less generic? What if users want a gradient? or a border? Or in the
// future add some blur?):
//
//   .OverlayArea {
//       weak-backdrop-color: rgba(0, 0, 0, 0);
//       strong-backdrop-color: rgba(0, 0, 0, 0.5);
//   }
//
// Or is it better to be something like this?
//
//   .Menu {
//       weak-backdrop-color: rgba(0, 0, 0, 0);
//       strong-backdrop-color: rgba(0, 0, 0, 0.5);
//   }
//
// In other words, do we want to allow per-overlay specific styling (third
// option), or do we prefer to enforce consistency (first two options)?
//
// Also, how to handle passthrough widgets? Possibly we can reimplementg
// ModalBackdrop::onPaintDraw() by manually drawing several quads around the
// passthrough instead of calling paintBackground(). Or maybe more generic and
// performant, setting a custom fragment shader to clip the inside of the
// passthrough widgets (would better support gradients, potential future blur,
// etc.).
//
// Also, how to handle multiple modal overlays? Currently there is only one
// backdrop. Should we possibly have more? Or perhaps one backdrop per
// consecutive series of weak modal overlays, but one backdrop per strong
// modal overlay?
//
class ModalBackdrop : public Widget {
private:
    VGC_OBJECT(ModalBackdrop, Widget)

protected:
    ModalBackdrop(CreateKey key)
        : Widget(key) {
    }

public:
    static ModalBackdropPtr create() {
        return core::createObject<ModalBackdrop>();
    }

    void setPassthrough(WidgetWeakPtr passthrough) {
        passthrough_ = std::move(passthrough);
    }

    // This signal is emitted when the ModalBackdrop itself received a click.
    //
    // This is not emitted if the click was propagated to child widgets or to
    // the passthrough widget.
    //
    VGC_SIGNAL(clicked)

protected:
    Widget* computeHoverChainChild(MouseHoverEvent* event) const override {
        if (auto passthrough = passthrough_.lock()) {
            geometry::Vec2f posInTarget = mapTo(passthrough.get(), event->position());
            if (passthrough->rect().contains(posInTarget)) {
                return passthrough.get();
            }
        }
        return nullptr;
    }

    bool onMousePress(MousePressEvent* event) override {
        bool handled = Widget::onMousePress(event);
        if (!handled && !hoverChainChild()) {
            clicked().emit();
        }
        return handled;
    }

private:
    WidgetWeakPtr passthrough_;
};

} // namespace detail

OverlayArea::OverlayArea(CreateKey key)
    : Widget(key) {
}

OverlayAreaPtr OverlayArea::create() {
    return core::createObject<OverlayArea>();
}

void OverlayArea::setBody(WidgetWeakPtr widget_) {

    if (widget_ != body_) {

        // Handle the case when widget_ is the modal backdrop.
        // This is not allowed.
        //
        if (modalBackdrop_.isAlive() && widget_ == modalBackdrop_) {
            return;
        }

        // Handle the case when widget_ was initially an overlay widget.
        //
        WidgetSharedPtr keepAlive = removeOverlay(widget_);

        // Replace the old body by the given widget.
        //
        auto oldBody = body_.lock();
        auto newBody = widget_.lock();
        body_ = widget_;
        if (oldBody) {
            if (newBody) {
                newBody->replace(oldBody.get());
            }
            else {
                oldBody->reparent(nullptr);
            }
        }
        else {
            if (newBody) {
                insertChildAt(0, newBody.get());
            }
            else {
                // Nothing to do
            }
        }
    }
}

void OverlayArea::addOverlay(OverlayModality modality, WidgetWeakPtr widget_) {

    auto widget = widget_.lock();
    if (!widget) {
        return;
    }

    // Handle the case when widget_ was initially the body.
    //
    if (widget_ == body_) {
        setBody(nullptr);
    }

    // Handle the case when widget_ was alreay an overlay.
    // In this case, we first remove it, then re-add it as the last child
    // with potentially a different modal policy and resize policy.
    //
    auto keepAlive = removeOverlay(widget_);

    // Register the new overlay and add it as a child widget.
    //
    overlays_.try_emplace(widget_, widget_, modality);
    addChild(widget.get());

    // Add the modal backdrop if necessary.
    //
    addModalBackdropIfNeeded_();

    requestRepaint();
}

WidgetSharedPtr OverlayArea::removeOverlay(WidgetWeakPtr widget) {

    // Find the overlay in the list of overlays and remove it.
    //
    WidgetSharedPtr res;
    auto it = overlays_.find(widget);
    if (it != overlays_.end()) {
        res = widget.lock();
        overlays_.erase(it);
    }

    // If found, make the overlay parentless, and also remove
    // the modal backdrop if there is no modal overlays anymore.
    //
    if (res) {
        res->reparent(nullptr);
        removeModalBackdropIfUnneeded_();
    }

    return res;
}

namespace {

void applyResizePolicy_(
    Widget& widget,
    OverlayResizePolicy resizePolicy,
    const geometry::Rect2f& areaRect) {

    switch (resizePolicy) {
    case OverlayResizePolicy::Stretch: {
        widget.updateGeometry(areaRect);
        break;
    }
    case OverlayResizePolicy::None:
        [[fallthrough]];
    default:
        widget.updateGeometry();
    }
}

} // namespace

void OverlayArea::setResizePolicy(
    WidgetWeakPtr widget_,
    OverlayResizePolicy resizePolicy) {

    if (auto widget = widget_.lock()) {
        auto it = overlays_.find(widget);
        if (it != overlays_.end()) {
            auto& [key, overlay] = *it;
            VGC_UNUSED(key);
            overlay.setResizePolicy(resizePolicy);
            applyResizePolicy_(*widget, resizePolicy, rect());
        }
    }
}

void OverlayArea::addPassthrough(WidgetWeakPtr overlay, WidgetWeakPtr passthrough) {

    VGC_UNUSED(overlay);

    // TODO: allow multiple passthrough, auto-remove when the overlay is not an
    // overlay anymore, etc.
    //
    if (auto modalBackdrop = modalBackdrop_.lock()) {
        modalBackdrop->setPassthrough(std::move(passthrough));
    }
}

void OverlayArea::onResize() {
    SuperClass::onResize();
}

void OverlayArea::onWidgetAdded(Widget* w, bool wasOnlyReordered) {

    VGC_UNUSED(wasOnlyReordered);

    // If area is no longer first, move to first.
    if (auto body = body_.lock()) {
        if (body->previousSibling()) {
            insertChildAt(0, body.get());
        }
    }

    // If modal backdrop no longer at its desired location, move it.
    if (auto modalBackdrop = modalBackdrop_.lock()) {
        if (auto body = body_.lock()) {
            if (modalBackdrop->previousSibling() != body.get()) {
                insertChildAt(1, modalBackdrop.get());
            }
        }
        else {
            if (modalBackdrop->previousSibling()) {
                insertChildAt(0, modalBackdrop.get());
            }
        }
    }

    if (WidgetWeakPtr(w) == body_) {
        requestGeometryUpdate();
    }
    else {
        requestRepaint();
    }
}

void OverlayArea::onWidgetRemoved(Widget* w) {
    WidgetWeakPtr widget = w;
    if (widget == body_) {
        body_ = nullptr;
        requestGeometryUpdate();
    }
    else if (widget == modalBackdrop_) {
        modalBackdrop_ = nullptr;
        addModalBackdropIfNeeded_(); // re-create it if someone stole it
    }
    else {
        overlays_.erase(widget);
        removeModalBackdropIfUnneeded_();
        requestRepaint();
    }
}

float OverlayArea::preferredWidthForHeight(float height) const {
    if (auto body = body_.lock()) {
        return body->preferredWidthForHeight(height);
    }
    else {
        return 0;
    }
}

float OverlayArea::preferredHeightForWidth(float width) const {
    if (auto body = body_.lock()) {
        return body->preferredHeightForWidth(width);
    }
    else {
        return 0;
    }
}

geometry::Vec2f OverlayArea::computePreferredSize() const {
    if (auto body = body_.lock()) {
        return body->preferredSize();
    }
    else {
        return {};
    }
}

namespace {

template<typename Key, typename Value>
core::Array<Value> copyValues(const std::unordered_map<Key, Value>& map) {
    core::Array<Value> res;
    res.reserve(map.size());
    for (const auto& [key, value] : map) {
        VGC_UNUSED(key);
        res.append(value);
    }
    return res;
}

} // namespace

void OverlayArea::updateChildrenGeometry() {

    // Update body
    geometry::Rect2f areaRect = rect();
    if (auto body = body_.lock()) {
        body->updateGeometry(areaRect);
    }

    // Update modal backdrop
    if (auto modalBackdrop = modalBackdrop_.lock()) {
        modalBackdrop->updateGeometry(areaRect);
    }

    // Update overlays. Note that `overlays_` may change during iteration,
    // reason why we must do a copy for memory safety (invalidated iterators).
    // However, new overlays created during iteration are intentionally not
    // updated. Indeed, it should typically not happen, and supporting this use
    // case would complexify significantly the code (prevent infinite loops,
    // etc.) and decrease performance.
    //
    for (const detail::Overlay& overlay : copyValues(overlays_)) {
        if (auto widget = overlay.widget().lock()) {
            applyResizePolicy_(*widget, overlay.resizePolicy(), areaRect);
        }
    }
}

bool OverlayArea::hasModalOverlays_() const {
    for (const auto& [key, overlay] : overlays_) {
        VGC_UNUSED(key);
        if (overlay.modality() != OverlayModality::Modeless) {
            return true;
        }
    }
    return false;
}

void OverlayArea::addModalBackdropIfNeeded_() {
    if (hasModalOverlays_()) {
        if (!modalBackdrop_.isAlive()) {
            Int index = body_.isAlive() ? 1 : 0;
            modalBackdrop_ = createChildAt<detail::ModalBackdrop>(index);
            if (auto modalBackdrop = modalBackdrop_.lock()) {
                modalBackdrop->clicked().connect(onModalBackdropClicked_Slot());
            }
        }
    }
}

void OverlayArea::removeModalBackdropIfUnneeded_() {
    if (!hasModalOverlays_()) {
        if (auto modalBackdrop = modalBackdrop_.lock()) {
            modalBackdrop->reparent(nullptr);
        }
        modalBackdrop_ = nullptr;
    }
}

void OverlayArea::onModalBackdropClicked_() {

    // We make a copy because `overlay_` changes during iteration.
    //
    // Newly created overlays are intentionally not closed, and potentially
    // already closed overlays are intentionally re-closed.
    //
    for (const detail::Overlay& overlay : copyValues(overlays_)) {
        if (overlay.modality() != OverlayModality::Modeless) {
            if (auto widget = overlay.widget().lock()) {

                // Perform custom close operation.
                // This may add or remove other overlays: we ignore them.
                widget->close();

                // Remove overlay. It might have already been done indirectly by widget->close(),
                // in which case this is a no-op.
                removeOverlay(widget);
            }
        }
    }
}

} // namespace vgc::ui
