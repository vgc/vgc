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

#include <vgc/core/array.h>
#include <vgc/core/innercore.h>
#include <vgc/core/stringid.h>
#include <vgc/core/vec2f.h>
#include <vgc/graphics/engine.h>
#include <vgc/ui/api.h>
#include <vgc/ui/exceptions.h>
#include <vgc/ui/lengthpolicy.h>
#include <vgc/ui/mouseevent.h>
#include <vgc/ui/style.h>

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

    /// Returns whether the two iterators are equal.
    ///
    friend bool operator==(const WidgetIterator&, const WidgetIterator&);

    /// Returns whether the two iterators are different.
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

/// \typedef vgc::ui::WidgetClasses
/// \brief Stores a set of widget classes.
///
/// Each widget is assigned a set of classes (e.g., "Button", "on") which can
/// be used to apply different styles to different widgets, or select a subset
/// of widgets in the application.
///
/// ```cpp
/// for (core::StringId class_ : widget->classes()) {
///     if (class_ == "some string") {
///         // ...
///     }
/// }
/// ```
///
/// Note that WidgetClasses guarantees that the same class cannot be added
/// twice, that is, if you call add() twice with the same class, then it is
/// added only once. Therefore, you always get a sequence of unique class names
/// when iterating over a WidgetClasses.
///
class VGC_UI_API WidgetClasses {
public:
    using const_iterator = typename core::Array<core::StringId>::const_iterator;

    /// Returns an iterator to the first class.
    ///
    const_iterator begin() const {
        return a_.begin();
    }

    /// Returns an iterator to the past-the-last class.
    ///
    const_iterator end() const {
        return a_.end();
    }

    /// Returns whether this set of classes contains the
    /// given class.
    ///
    bool contains(core::StringId class_) const {
        // TODO: add contains() method directly to core::Array
        return std::find(begin(), end(), class_) != end();
    }

    /// Adds a class.
    ///
    void add(core::StringId class_) {
        if(!contains(class_)) {
            a_.append(class_);
        }
    }

    /// Removes a class.
    ///
    void remove(core::StringId class_) {
        // TODO: implement removeOne() and removeAll() method directly in core::Array
        auto it = find(begin(), end(), class_);
        if (it != end()) {
            a_.erase(it);
        }
    }

    /// Adds the class to the list if it's not already there,
    /// otherwise removes the class.
    ///
    void toggle(core::StringId class_) {
        auto it = find(begin(), end(), class_);
        if (it == end()) {
            a_.append(class_);
        }
        else {
            a_.erase(it);
        }
    }

private:
    // The array storing all the strings.
    //
    // Note 1: we use StringId instead of std::string because there is
    // typically only a fixed number of class names, which are reused by many
    // widgets. This makes comparing between strings faster, and reduce memory
    // usage.
    //
    // Note 2: we use an array rather than an std::set or std::unordered_set
    // because it's typically very small, so an array is most likely faster
    // even for searching.
    //
    // TODO: in fact, should we even use a SmallArray<n, T> instead? Similar to
    // the small-string optimization, this would be a class that keeps on the
    // stack any array with less than n elements. In this case, a widget rarely
    // has more than n = 5 classes.
    //
    core::Array<core::StringId> a_;
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
    core::Vec2f position() const
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
    void setGeometry(const core::Vec2f& position, const core::Vec2f& size);
    /// \overload
    void setGeometry(float x, float y, float width, float height)
    {
        setGeometry(core::Vec2f(x, y), core::Vec2f(width, height));
    }

    /// Returns the preferred size of this widget, that is, the size that
    /// layout classes will try to assign to this widget. However, note this
    /// widget may be assigned a different size if its size policy allows it to
    /// shrink or grow, or if it is impossible to meet all size constraints.
    ///
    /// If you need to modify the preferred size of a given widget, you can
    /// either change its widthPolicy() or heightPolicy() from LengthType::Auto
    /// to a fixed length, or you create a new Widget subclass and reimplement
    /// computePreferredSize() for finer control.
    ///
    core::Vec2f preferredSize() const {
        if (!isPreferredSizeComputed_) {
            preferredSize_ = computePreferredSize();
            isPreferredSizeComputed_ = true;
        }
        return preferredSize_;
    }

    /// Returns the size of the widget.
    ///
    core::Vec2f size() const
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

    /// Returns the LengthPolicy related to the width of this widget. This
    /// tells layout classes how to stretch and shrink this widget, and what
    /// the desired width should be in case it's not LengthType::Auto.
    ///
    LengthPolicy widthPolicy() const
    {
        return widthPolicy_;
    }

    /// Sets the LengthPolicy related to the width of this widget.
    ///
    void setWidthPolicy(const LengthPolicy& policy);

    /// Returns the LengthPolicy related to the height of this widget. This
    /// tells layout classes how to stretch and shrink this widget, and what
    /// the desired height should be in case it's not LengthType::Auto.
    ///
    LengthPolicy heightPolicy() const
    {
        return heightPolicy_;
    }

    /// Sets the LengthPolicy related to the height of this widget.
    ///
    void setHeightPolicy(const LengthPolicy& policy);

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

    /// Returns the set of classes of this widget.
    ///
    const WidgetClasses& classes() const {
        return classes_;
    }

    /// Returns whether this widget is assigned the given class.
    ///
    bool hasClass(core::StringId class_) const {
        return classes_.contains(class_);
    }

    /// Adds the given class to this widget.
    ///
    void addClass(core::StringId class_);

    /// Removes the given class to this widget.
    ///
    void removeClass(core::StringId class_) ;

    /// Toggles the given class to this widget.
    ///
    void toggleClass(core::StringId class_);

    /// Returns the computed value of a given style property of this widget.
    ///
    StyleValue style(core::StringId property);

protected:
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
    /// LengthType::Auto, in which case this function should return the
    /// specified fixed value.
    ///
    /// Note that if, for example, widthPolicy() is a fixed value, but
    /// heightPolicy() is Auto, then the preferred height may depend on the
    /// value of the fixed width.
    ///
    virtual core::Vec2f computePreferredSize() const;

    /// Updates the position and size of children of this widget (by calling
    /// the setGeometry() methods of the children), based on the current width
    /// and height of this widget.
    ///
    virtual void updateChildrenGeometry();

private:
    mutable core::Vec2f preferredSize_;
    mutable bool isPreferredSizeComputed_;
    LengthPolicy widthPolicy_;
    LengthPolicy heightPolicy_;
    core::Vec2f position_;
    core::Vec2f size_;
    Widget* mousePressedChild_;
    Widget* mouseEnteredChild_;
    WidgetClasses classes_;
    friend class Style;
    Style style_;
    void onClassesChanged_();
};

inline WidgetIterator& WidgetIterator::operator++() {
    widget_ = widget_->nextSibling();
    return *this;
}

inline WidgetIterator WidgetIterator::operator++(int) {
    WidgetIterator res(*this);
    operator++();
    return res;
}

inline bool operator==(const WidgetIterator& it1, const WidgetIterator& it2) {
    return it1.widget_ == it2.widget_;
}

inline bool operator!=(const WidgetIterator& it1, const WidgetIterator& it2) {
    return !(it1 == it2);
}

} // namespace ui
} // namespace vgc

#endif // VGC_UI_WIDGET_H
