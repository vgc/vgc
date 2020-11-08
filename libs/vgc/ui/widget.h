// Copyright 2020 The VGC Developers
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

#include <vgc/core/innercore.h>
#include <vgc/graphics/engine.h>
#include <vgc/ui/api.h>
#include <vgc/ui/exceptions.h>
#include <vgc/ui/mouseevent.h>

namespace vgc {
namespace ui {

VGC_DECLARE_OBJECT(Widget);

/// \class vgc::ui::WidgetIterator
/// \brief Iterates over a range of Widget siblings.
///
class VGC_UI_API WidgetIterator {
public:
    typedef Widget* value_type;
    typedef Widget*& reference;
    typedef Widget** pointer;
    typedef std::input_iterator_tag iterator_category;

    /// Constructs an iterator pointing to the given Widget.
    ///
    WidgetIterator(Widget* widget) : widget_(widget)
    {

    }

    /// Prefix-increments this iterator.
    ///
    WidgetIterator& operator++();

    /// Postfix-increments this iterator.
    ///
    WidgetIterator operator++(int);

    /// Dereferences this iterator with the star operator.
    ///
    const value_type& operator*() const
    {
        return widget_;
    }

    /// Dereferences this iterator with the arrow operator.
    ///
    const value_type* operator->() const
    {
        return &widget_;
    }

    /// Returns whether the two iterators are equals.
    ///
    friend bool operator==(const WidgetIterator&, const WidgetIterator&);

    /// Returns whether the two iterators are differents.
    ///
    friend bool operator!=(const WidgetIterator&, const WidgetIterator&);

private:
    Widget* widget_;
};

/// \class vgc::ui::WidgetList
/// \brief Enable range-based loops for sibling Widgets.
///
/// This range class is used together with WidgetIterator to
/// enable range-based loops like the following:
///
/// \code
/// for (Widget* child : widget->children()) {
///     // ...
/// }
/// \endcode
///
/// \sa WidgetIterator and Widget.
///
class VGC_UI_API WidgetList {
public:
    /// Constructs a range of sibling widgets from \p begin to \p end. The
    /// range includes \p begin but excludes \p end. The behavior is undefined
    /// if \p begin and \p end do not have the same parent. If \p end is null,
    /// then the range includes all siblings after \p begin. If both \p start
    /// and \p end are null, then the range is empty. The behavior is undefined
    /// if \p begin is null but \p end is not.
    ///
    WidgetList(Widget* begin, Widget* end) : begin_(begin), end_(end)
    {

    }

    /// Returns the begin of the range.
    ///
    const WidgetIterator& begin() const
    {
        return begin_;
    }

    /// Returns the end of the range.
    ///
    const WidgetIterator& end() const
    {
        return end_;
    }

private:
    WidgetIterator begin_;
    WidgetIterator end_;
};

/// \class vgc::ui::Widget
/// \brief Base class of all elements in the user interface.
///
class VGC_UI_API Widget : public core::Object {
private:
    VGC_OBJECT(Widget)
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
        return static_cast<Widget*>(parentObject());
    }

    /// Returns the first child Widget of this Widget. Returns nullptr if this
    /// Widget has no children.
    ///
    /// \sa lastChild(), previousSibling(), nextSibling(), and parent().
    ///
    Widget* firstChild() const
    {
        return static_cast<Widget*>(firstChildObject());
    }

    /// Returns the last child Widget of this Widget. Returns nullptr if this
    /// Widget has no children.
    ///
    /// \sa firstChild(), previousSibling(), nextSibling(), and parent().
    ///
    Widget* lastChild() const
    {
        return static_cast<Widget*>(lastChildObject());
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
    WidgetList children() const
    {
        return WidgetList(firstChild(), nullptr);
    }

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

    /// Returns the width of the widget.
    ///
    float width() const
    {
        return width_;
    }

    /// Returns the height of the widget.
    ///
    float height() const
    {
        return height_;
    }

    /// Resizes the widget to the given width and height.
    ///
    void resize(float width, float height);

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

    /// This signal is emitted when someone requested this widget, or one of
    /// its descendent widgets, to be repainted.
    ///
    const core::Signal<> repaintRequested;

    /// This virtual function is called once before the first call to
    /// onPaintDraw(), and should be reimplemented to create required GPU
    /// resources.
    ///
    virtual void onPaintCreate(graphics::Engine* engine);

    /// This virtual function is called whenever the widget needs to be
    /// repainted. Subclasses should reimplement this, typically by issuing
    /// draw calls.
    ///
    virtual void onPaintDraw(graphics::Engine* engine);

    /// This virtual function is called once after the last call to
    /// onPaintDraw(), for example before the widget is destructed, or if
    /// switching graphics engine. It should be reimplemented to destroy the
    /// created GPU resources.
    ///
    virtual void onPaintDestroy(graphics::Engine* engine);

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

private:
    float width_;
    float height_;
};

} // namespace ui
} // namespace vgc

#endif // VGC_UI_WIDGET_H
