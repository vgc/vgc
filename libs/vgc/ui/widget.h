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

#ifndef VGC_UI_WIDGET_H
#define VGC_UI_WIDGET_H

#include <algorithm> // std::find

#include <QOpenGLBuffer>
#include <QOpenGLFunctions_3_2_Core>
#include <QOpenGLShaderProgram>
#include <QOpenGLVertexArrayObject>

#include <vgc/core/array.h>
#include <vgc/core/innercore.h>
#include <vgc/core/stringid.h>
#include <vgc/geometry/rect2f.h>
#include <vgc/geometry/vec2f.h>
#include <vgc/graphics/engine.h>
#include <vgc/graphics/idgenerator.h>
#include <vgc/style/stylableobject.h>
#include <vgc/ui/action.h>
#include <vgc/ui/api.h>
#include <vgc/ui/exceptions.h>
#include <vgc/ui/mouseevent.h>
#include <vgc/ui/shortcut.h>
#include <vgc/ui/sizepolicy.h>

class QKeyEvent;

namespace vgc::ui {

VGC_DECLARE_OBJECT(Action);
VGC_DECLARE_OBJECT(Widget);
VGC_DECLARE_OBJECT(UiWidgetEngine);

enum class PaintOption : UInt16 {
    None = 0,
    Resizing            = 0x0001,
    LayoutViz           = 0x0002,
};
VGC_DEFINE_FLAGS(PaintOptions, PaintOption)

/// \class vgc::ui::Widget
/// \brief Base class of all elements in the user interface.
///
class VGC_UI_API Widget : public style::StylableObject {
private:
    VGC_OBJECT(Widget, style::StylableObject)
    VGC_PRIVATIZE_OBJECT_TREE_MUTATORS

protected :
    /// Constructs a Widget. This constructor is an implementation detail only
    /// available to derived classes. In order to create a Widget, please use
    /// the following:
    ///
    /// ```cpp
    /// WidgetPtr widget = Widget::create();
    /// ```
    ///
    Widget();

    void onDestroyed() override;

public:
    /// Creates a widget.
    ///
    static WidgetPtr create();

    /// Destroys this Widget.
    ///
    /// \sa vgc::core::Object::isAlive().
    ///
    void destroy()
    {
        destroyObject_();
    }

    /// Returns the parent Widget of this Widget. This can be nullptr
    /// for root widgets.
    ///
    /// \sa firstChild(), lastChild(), previousSibling(), and nextSibling().
    ///
    Widget* parent() const
    {
        // TODO; the parent widget should actually be a member variable, to be
        // able to correctly handle the case of a root widget owned by another
        // object (that is, the widget would have a non-null parent object, but
        // would still be the root widget).

        core::Object* list = parentObject();
        core::Object* widget = list ? list->parentObject() : nullptr;
        return static_cast<Widget*>(widget);
    }

    /// Returns the first child Widget of this Widget. Returns nullptr if this
    /// Widget has no children.
    ///
    /// \sa lastChild(), previousSibling(), nextSibling(), and parent().
    ///
    Widget* firstChild() const
    {
        return children_->first();
    }

    /// Returns the last child Widget of this Widget. Returns nullptr if this
    /// Widget has no children.
    ///
    /// \sa firstChild(), previousSibling(), nextSibling(), and parent().
    ///
    Widget* lastChild() const
    {
        return children_->last();
    }

    /// Returns the previous sibling of this Widget. Returns nullptr if this
    /// Widget is a root widget, or if it is the first child of its parent.
    ///
    /// \sa nextSibling(), parent(), firstChild(), and lastChild().
    ///
    Widget* previousSibling() const
    {
        return static_cast<Widget*>(previousSiblingObject());
    }

    /// Returns the next sibling of this Widget. Returns nullptr if this Widget
    /// is a root widget, or if it is the last child of its parent.
    ///
    /// \sa previousSibling(), parent(), firstChild(), and lastChild().
    ///
    Widget* nextSibling() const
    {
        return static_cast<Widget*>(nextSiblingObject());
    }

    /// Returns all children of this Widget as an iterable range.
    ///
    /// Example:
    ///
    /// \code
    /// for (Widget* child : widget->children()) {
    ///     // ...
    /// }
    /// \endcode
    ///
    WidgetListView children() const
    {
        return WidgetListView(children_);
    }

    /// Creates a new widget of type \p WidgetClass constructed with
    /// the given arguments \p args, and add it as a child of this widget.
    /// Returns a pointer to the created widget.
    ///
    template<typename WidgetClass, typename... Args>
    WidgetClass* createChild(Args&&... args) {
        core::ObjPtr<WidgetClass> child = WidgetClass::create(args...);
        addChild(child.get());
        return child.get();
    }

    /// Adds a child to this widget.
    ///
    void addChild(Widget* child);

    /// Returns whether this Widget can be reparented with the given \p newParent.
    /// See reparent() for details.
    ///
    bool canReparent(Widget* newParent);

    /// Moves this Widget from its current position in the widget tree to the
    /// last child of the given \p newParent. If \p newParent is already the
    /// current parent of this widget, then this Widget is simply moved last
    /// without changing its parent. If \p newParent is nullptr, then the
    /// widget becomes a root widget.
    ///
    /// A ChildCycleError exception is raised if \p newParent is this Widget
    /// itself or one of its descendant.
    ///
    void reparent(Widget* newParent);

    /// Returns whether \p oldWidget can be replaced by this Widget. See replace()
    /// for details.
    ///
    bool canReplace(Widget* oldWidget);

    /// Replaces the given \p oldWidget with this Widget. This destroys \p oldWidget
    /// and all its descendants, except this Widget and all its descendants. Does
    /// nothing if \p oldWidget is this Widget itself.
    ///
    /// A NullError exception is raised if \p oldWidget is null.
    ///
    /// A ChildCycleError exception is raised if \p oldWidget is a (strict)
    /// descendant this Widget
    ///
    void replace(Widget* oldWidget);

    /// Returns whether this widget is a descendant of the given \p other widget.
    /// Returns true if this widget is equal to the \p other widget.
    ///
    bool isDescendant(const Widget* other) const
    {
        return isDescendantObject(other);
    }

    /// Returns the root widget of this widget, that is, the ancestor of this
    /// widget which has no parent widget itself.
    ///
    Widget* root() const;

    /// Returns the position of the widget relative to its parent.
    ///
    geometry::Vec2f position() const
    {
        return position_;
    }

    /// Returns the X coordinate of the widget relative to its parent.
    ///
    float x() const
    {
        return position_[0];
    }

    /// Returns the Y coordinate of the widget relative to its parent.
    ///
    float y() const
    {
        return position_[1];
    }

    /// Returns the geometry of the widget relative to its parent.
    ///
    /// This is equivalent to `Rect2f::fromPositionSize(position(), size())`.
    ///
    geometry::Rect2f geometry() const
    {
        return geometry::Rect2f::fromPositionSize(position_, size_);
    }

    /// Returns the geometry of the widget relative to itself.
    ///
    /// This is equivalent to `Rect2f::fromPositionSize(0, 0, size())`.
    ///
    geometry::Rect2f rect() const
    {
        return geometry::Rect2f::fromPositionSize(0, 0, size_);
    }

    /// Sets the new position and size of this widget (relative to its parent),
    /// then calls updateChildrenGeometry().
    ///
    /// Unless this widget is the root widget, this method should only be
    /// called by the parent of this widget, inside updateChildrenGeometry().
    /// In other words, setGeometry() calls updateChildrenGeometry(), which
    /// calls setGeometry() on all its children, etc.
    ///
    /// In order to prevent infinite loops, this function does not
    /// automatically triggers a repaint nor informs the parent widget that the
    /// geometry changed. Indeed, the assumption is that the parent widget
    /// already knows, since it is the object that called this function in the
    /// first place.
    ///
    void setGeometry(const geometry::Vec2f& position, const geometry::Vec2f& size);
    /// \overload
    void setGeometry(float x, float y, float width, float height)
    {
        setGeometry(geometry::Vec2f(x, y), geometry::Vec2f(width, height));
    }

    /// Returns the preferred size of this widget, that is, the size that
    /// layout classes will try to assign to this widget. However, note this
    /// widget may be assigned a different size if its size policy allows it to
    /// shrink or grow, or if it is impossible to meet all size constraints.
    ///
    /// If you need to modify the preferred size of a given widget, you can
    /// either change its `preferred-width` or `preferred-height` from auto
    /// (the default value) to a fixed length, or you can create a new Widget
    /// subclass and reimplement computePreferredSize() for finer control.
    ///
    /// Note that this function returns the "computed" preferred size of the
    /// widget. In particular, while preferredWidth() and preferredHeight() may
    /// return "auto", this function always returns actual values based on the
    /// widget content.
    ///
    geometry::Vec2f preferredSize() const {
        if (!isPreferredSizeComputed_) {
            preferredSize_ = computePreferredSize();
            isPreferredSizeComputed_ = true;
        }
        return preferredSize_;
    }

    /// Returns the size of the widget.
    ///
    geometry::Vec2f size() const
    {
        return size_;
    }

    /// Returns the width of the widget.
    ///
    float width() const
    {
        return size_[0];
    }

    /// Returns the height of the widget.
    ///
    float height() const
    {
        return size_[1];
    }

    /// Returns the preferred width of this widget.
    ///
    PreferredSize preferredWidth() const;

    /// Returns the width stretch factor of this widget.
    ///
    float stretchWidth() const;

    /// Returns the width shrink factor of this widget.
    ///
    float shrinkWidth() const;

    /// Returns the preferred height of this widget.
    ///
    PreferredSize preferredHeight() const;

    /// Returns the height stretch factor of this widget.
    ///
    float stretchHeight() const;

    /// Returns the height shrink factor of this widget.
    ///
    float shrinkHeight() const;

    /// This method should be called when the size policy or preferred size of
    /// this widget changed, to inform its parent that its geometry should be
    /// recomputed.
    ///
    void updateGeometry();

    /// This virtual function is called each time the widget is resized. When
    /// this function is called, the widget already has its new size.
    ///
    virtual void onResize();

    /// Requests this widget to be repainted, for example because the data
    /// displayed by this widget has changed. The widget is not immediately
    /// repainted: it is only scheduled for repaint. The actual moment when the
    /// widget is repainted depends on the graphics::Engine and other
    /// platform-dependent factors.
    ///
    /// You should typically call this function in your event handlers (e.g.,
    /// mousePressEvent()), to notify that the appearance of this widget has
    /// changed as a result of the event. Such call can be indirect, below is a
    /// example scenario:
    ///
    /// 1. The user clicks on a "Add Circle" button.
    /// 2. The event handler of the button emits the "clicked" signal.
    /// 3. A listener of this signal calls scene->addCircle().
    /// 4. This modifies the scene, which emits a "changed" signal.
    /// 5. A view of the scene detects the change, and calls this->repaint().
    ///
    /// Note how in this scenario, the repainted view is unrelated to the
    /// button which initially handled the event.
    ///
    void repaint();

    /// This function is called by the widget container whenever the widget
    /// needs to prepare to be repainted for a frame.
    ///
    void preparePaint(graphics::Engine* engine, PaintOptions flags = PaintOption::None);

    /// This function is called by the widget container whenever the widget
    /// needs to be repainted for a frame.
    ///
    void paint(graphics::Engine* engine, PaintOptions flags = PaintOption::None);

    /// This signal is emitted when someone requested this widget, or one of
    /// its descendent widgets, to be repainted.
    ///
    VGC_SIGNAL(repaintRequested);

    /// Override this function if you wish to handle MouseMove events. You must
    /// return true if the event was handled, false otherwise.
    ///
    virtual bool onMouseMove(MouseEvent* event);

    /// Override this function if you wish to handle MousePress events. You
    /// must return true if the event was handled, false otherwise.
    ///
    virtual bool onMousePress(MouseEvent* event);

    /// Override this function if you wish to handle MouseRelease events. You
    /// must return true if the event was handled, false otherwise.
    ///
    virtual bool onMouseRelease(MouseEvent* event);

    /// Override this function if you wish to handle MouseEnter events. You
    /// must return true if the event was handled, false otherwise.
    ///
    virtual bool onMouseEnter();

    /// Override this function if you wish to handle MouseLeave events. You
    /// must return true if the event was handled, false otherwise.
    ///
    virtual bool onMouseLeave();

    /// Returns whether this widget tree is active, that is, whether it
    /// receives key press and focus events.
    ///
    /// More precisely, when the widget tree is active, then one FocusOut event
    /// and one FocusIn event are sent whenever the focused widget changes.
    ///
    /// On the contrary, when the widget tree is not active, then no FocusIn
    /// and FocusOut events are sent.
    ///
    /// A FocusIn event is sent to the focused widget (if any) when the tree
    /// becomes active.
    ///
    /// A FocusOut event is sent to the focused widget (if any) when the tree
    /// becomes inactive.
    ///
    /// \sa setTreeActive()
    ///
    bool isTreeActive() const {
        return root()->isTreeActive_;
    }

    /// Sets whether this widget tree is active.
    ///
    /// \sa isTreeActive()
    ///
    void setTreeActive(bool active);

    /// This signal is emitted when someone requested this widget, or one of
    /// its descendent widgets, to be focused.
    ///
    VGC_SIGNAL(focusRequested);

    /// Makes this widget the focused widget of this widget tree, and emits the
    /// focusRequested signal.
    ///
    /// If the tree is active, then this widget will now receive keyboard
    /// events.
    ///
    /// If the tree is active and if this widget was not already the focused
    /// widget, then this widget will receive a FocusIn event.
    ///
    /// Note that if this widget was already the focused widget, then typically
    /// it will not receive a FocusIn event. However, the focusRequested signal
    /// is still emitted, and as a result the tree may switch from inactive to
    /// active, in which case this widget will in fact indirectly receive a
    /// FocusIn event.
    ///
    /// After calling this function, all ancestors of this widget become part
    /// of what is called the "focus branch": it is the branch that goes from
    /// the root of the widget tree to the focused widget. For all widgets in
    /// this branch (including the focused widget), hasFocusWithin() returns
    /// true, and focusedChild() returns the child in this branch.
    ///
    /// \sa isTreeActive(), clearFocus(), focusedWidget()
    ///
    void setFocus();

    /// Removes the focus from the focused widget, if any. If the tree is
    /// active, the focused widget will receive a FocusOut event, and from now
    /// on will not receive keyboard events.
    ///
    /// Does nothing if focusedWidget() is nullptr.
    ///
    void clearFocus();

    /// Returns which child of this widget is part of the focus branch (see
    /// setFocus() for details). The returned widget is the only child of this
    /// widget, if any, such that hasFocusWithin() returns true.
    ///
    /// Returns nullptr if none of the descendant of this widget is the focused
    /// widget. Also returns nullptr if this widget itself is the focused
    /// widget.
    ///
    /// \sa setFocus(), clearFocus(), focusedWidget(), hasFocusWithin()
    ///
    Widget* focusedChild() const {
        return isFocusedWidget() ? nullptr : focus_;
    }

    /// Returns the focused widget of this widget tree, if any.
    ///
    Widget* focusedWidget() const;

    /// Returns whether this widget is the focused widget.
    ///
    bool isFocusedWidget() const {
        return focus_ == this;
    }

    /// Returns whether this widget is the focused widget, and this widget tree
    /// is active.
    ///
    bool hasFocus() const {
        return isFocusedWidget() && isTreeActive();
    }

    /// Returns whether this widget or any of its descendant is the focused
    /// widget, and this widget tree is active.
    ///
    bool hasFocusWithin() const {
        return focus_ != nullptr && isTreeActive();
    }

    /// Override this function if you wish to handle FocusIn events. You must
    /// return true if the event was handled, false otherwise. The default
    /// implementation returns false.
    ///
    /// This function is called when:
    /// 1. isTreeActive() is true and the focused widget changed, or
    /// 2. isTreeActive() changed from false to true
    ///
    /// Note that this function is only called for the focused widget itself,
    /// not for all its ancestors.
    ///
    /// \sa onFocusOut(), setFocus(), clearFocus(), isTreeActive()
    ///
    virtual bool onFocusIn();

    /// Override this function if you wish to handle FocusOut events. You must
    /// return true if the event was handled, false otherwise. The default
    /// implementation returns false.
    ///
    /// This function is called when:
    /// 1. isTreeActive() is true and the focused widget changed, or
    /// 2. isTreeActive() changed from true to false
    ///
    /// This function is called just after the widget loses focus, so you can rely
    /// on the return value of `isFocusedWidget()` to determine whether this widget is
    /// still the focused widget despite losing focus (i.e., situation 2. above).
    ///
    /// Note that this function is only called for the focused widget itself,
    /// not for all its ancestors.
    ///
    /// \sa onFocusIn(), setFocus(), clearFocus(), isTreeActive()
    ///
    virtual bool onFocusOut();

    /// Override this function if you wish to handle key press events. You must
    /// return true if the event was handled, false otherwise.
    ///
    /// The default implementation recursively calls onKeyPress() on the
    /// focusedChild(), if any. If there is no focusedChild(), the default
    /// implementation returns false.
    ///
    virtual bool onKeyPress(QKeyEvent* event);

    /// Override this function if you wish to handle key release events. You must
    /// return true if the event was handled, false otherwise.
    ///
    /// The default implementation recursively calls onKeyRelease() on the
    /// focusedChild(), if any. If there is no focusedChild(), the default
    /// implementation returns false.
    ///
    virtual bool onKeyRelease(QKeyEvent* event);

    /// Returns the list of actions of this widget.
    ///
    ActionListView actions() const {
        return ActionListView(actions_);
    }

    /// Creates an Action with the given shortcut, adds it to this widget, and
    /// returns the action.
    ///
    Action* createAction(const Shortcut& shortcut);

    /// This virtual function is called once before the first call to
    /// onPaintDraw(), and should be reimplemented to create required static GPU
    /// resources.
    ///
    virtual void onPaintCreate(graphics::Engine* engine);

    /// This virtual function is called whenever the widget needs to prepare to
    /// be repainted. Subclasses should reimplement this, typically to update
    /// resources.
    ///
    virtual void onPaintPrepare(graphics::Engine* engine, PaintOptions flags);

    /// This virtual function is called whenever the widget needs to be
    /// repainted. Subclasses should reimplement this, typically by issuing
    /// draw calls.
    ///
    virtual void onPaintDraw(graphics::Engine* engine, PaintOptions flags);

    /// This virtual function is called once after the last call to
    /// onPaintDraw(), for example before the widget is destructed, or if
    /// switching graphics engine. It should be reimplemented to destroy the
    /// created GPU resources.
    ///
    virtual void onPaintDestroy(graphics::Engine* engine);

    // Implements StylableObject interface
    style::StylableObject* parentStylableObject() const override;
    style::StylableObject* firstChildStylableObject() const override;
    style::StylableObject* lastChildStylableObject() const override;
    style::StylableObject* previousSiblingStylableObject() const override;
    style::StylableObject* nextSiblingStylableObject() const override;
    const style::StyleSheet* defaultStyleSheet() const override;

protected:
    virtual void onWidgetAdded(Object*) {};
    virtual void onWidgetRemoved(Object*) {};

    /// Computes the preferred size of this widget based on its size policy, as
    /// well as its content and the preferred size and size policy of its
    /// children.
    ///
    /// For example, the preferred size of Row class is computed based on
    /// the preferred size of its children, and the preferred size of Label is
    /// computed based on the length of the text.
    ///
    /// If you reimplement this method, make sure to check whether the
    /// widthPolicy() or heightPolicy() of this widget is different from
    /// PreferredSizeType::Auto, in which case this function should return the
    /// specified fixed value.
    ///
    /// Note that if, for example, widthPolicy() is a fixed value, but
    /// heightPolicy() is Auto, then the preferred height may depend on the
    /// value of the fixed width.
    ///
    virtual geometry::Vec2f computePreferredSize() const;

    /// Updates the position and size of children of this widget (by calling
    /// the setGeometry() methods of the children), based on the current width
    /// and height of this widget.
    ///
    virtual void updateChildrenGeometry();

private:
    WidgetList* children_;
    ActionList* actions_;
    mutable geometry::Vec2f preferredSize_;
    mutable bool isPreferredSizeComputed_;
    geometry::Vec2f position_;
    geometry::Vec2f size_;
    Widget* mousePressedChild_;
    Widget* mouseEnteredChild_;
    bool isTreeActive_;
    Widget* focus_; // This can be: nullptr   (when no focused widget)
                    //              this      (when this is the focused widget)
                    //              child ptr (when a descendant is the focused widget)

    graphics::Engine* lastPaintEngine_ = nullptr;
    void releaseEngine_();
    void setEngine_(graphics::Engine* engine);

    VGC_SLOT(onEngineAboutToBeDestroyed, releaseEngine_);
    VGC_SLOT(onWidgetAdded_, onWidgetAdded);
    VGC_SLOT(onWidgetRemoved_, onWidgetRemoved);
};

} // namespace vgc::ui

#endif // VGC_UI_WIDGET_H
