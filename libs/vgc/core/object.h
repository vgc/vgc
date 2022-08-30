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

#ifndef VGC_CORE_OBJECT_H
#define VGC_CORE_OBJECT_H

#include <iterator> // distance
#include <type_traits>

#include <vgc/core/api.h>
#include <vgc/core/arithmetic.h>
#include <vgc/core/exceptions.h>

#include <vgc/core/detail/signal.h>

namespace vgc::core {

class Object;
using ConnectionHandle = detail::ConnectionHandle;

namespace detail {

// This class is befriended by Object to allow ObjPtr<T> to access and modify
// the refCount, and destruct the object. Please do not use unless in the
// implementation of ObjPtr<T>.
//
class ObjPtrAccess {
public:
    // Increments by k the refCount of the given obj and of all its ancestors.
    // Does nothing if obj == nullptr.
    //
    static void incref(const Object* obj, Int64 k = 1);

    // Decrements by k the refCount of the given obj and of all its ancestors.
    // Destroys the root of the given obj if its refcount reaches zero. Does
    // nothing if obj == nullptr.
    //
    static void decref(const Object* obj, Int64 k = 1);
};

} // namespace detail

/// \class ObjPtr<T>
/// \brief Smart pointer for managing the lifetime of Object instances.
///
/// See documentation of Object for more details.
///
template<typename T>
class ObjPtr {
private:
    template<typename Y>
    static constexpr bool isCompatible_ = std::is_convertible_v<Y*, T*>;

    template<typename S, typename U>
    friend ObjPtr<S> static_pointer_cast(const ObjPtr<U>& r) noexcept;
    template<typename S, typename U>
    friend ObjPtr<S> static_pointer_cast(ObjPtr<U>&& r) noexcept;

    template<typename S, typename U>
    friend ObjPtr<S> dynamic_pointer_cast(const ObjPtr<U>& r) noexcept;
    template<typename S, typename U>
    friend ObjPtr<S> dynamic_pointer_cast(ObjPtr<U>&& r) noexcept;

    template<typename S, typename U>
    friend ObjPtr<S> const_pointer_cast(const ObjPtr<U>& r) noexcept;
    template<typename S, typename U>
    friend ObjPtr<S> const_pointer_cast(ObjPtr<U>&& r) noexcept;

    struct DontIncRefTag {};

    // For move casts
    ObjPtr(T* obj, DontIncRefTag)
        : obj_(obj) {
    }

public:
    /// Creates a null ObjPtr<T>, that is, an ObjPtr<T> which doesn't manage any
    /// Object.
    ///
    ObjPtr() noexcept
        : obj_(nullptr) {
    }

    /// Creates an ObjPtr<T> managing the given Object.
    ///
    /// Note that in the current implementation, we use intrusive reference
    /// counting and therefore this constructor never throws and could be
    /// marked noexept. However, you shouldn't rely on this: a future
    /// implementation may implement referencing counting via
    /// separately-allocated counter blocks, whose memory allocation may throw.
    ///
    ObjPtr(T* obj)
        : obj_(obj) {

        detail::ObjPtrAccess::incref(obj_);
    }

    /// Creates a copy of the given ObjPtr<T>.
    ///
    ObjPtr(const ObjPtr& other) noexcept
        : obj_(other.obj_) {

        detail::ObjPtrAccess::incref(obj_);
    }

    /// Creates a copy of the given ObjPtr<Y>. This template overload doesn't
    /// participate in overload resolution if Y* is not implicitly convertible
    /// to T*.
    ///
    template<typename Y, VGC_REQUIRES(isCompatible_<Y>)>
    ObjPtr(const ObjPtr<Y>& other) noexcept
        : obj_(other.obj_) {

        detail::ObjPtrAccess::incref(obj_);
    }

    /// Assigns the given ObjPtr<T> to this ObjPtr<T>.
    ///
    ObjPtr& operator=(const ObjPtr& other) noexcept {
        if (obj_ != other.obj_) {
            detail::ObjPtrAccess::decref(obj_);
            obj_ = other.obj_;
            detail::ObjPtrAccess::incref(obj_);
        }
        return *this;
    }

    /// Assigns the given ObjPtr<Y> to this ObjPtr<T>. This template overload
    /// doesn't participate in overload resolution if Y* is not implicitly
    /// convertible to T*.
    ///
    template<typename Y, VGC_REQUIRES(isCompatible_<Y>)>
    ObjPtr& operator=(const ObjPtr<Y>& other) noexcept {
        if (obj_ != other.obj_) {
            detail::ObjPtrAccess::decref(obj_);
            obj_ = other.obj_;
            detail::ObjPtrAccess::incref(obj_);
        }
        return *this;
    }

    /// Moves the given ObjPtr<T> to a new ObjPtr<T>.
    ///
    ObjPtr(ObjPtr&& other) noexcept
        : obj_(other.obj_) {

        other.obj_ = nullptr;
    }

    /// Moves the given ObjPtr<Y> to a new ObjPtr<T>. This template overload
    /// doesn't participate in overload resolution if Y* is not implicitly
    /// convertible to T*.
    ///
    template<typename Y, VGC_REQUIRES(isCompatible_<Y>)>
    ObjPtr(ObjPtr<Y>&& other) noexcept
        : obj_(other.obj_) {

        other.obj_ = nullptr;
    }

    /// Moves the given ObjPtr<T> to this ObjPtr<T>.
    ///
    ObjPtr& operator=(ObjPtr&& other) noexcept {
        if (*this != other) {
            detail::ObjPtrAccess::decref(obj_);
            obj_ = other.obj_;
            other.obj_ = nullptr;
        }
        return *this;
    }

    /// Moves the given ObjPtr<Y> to this ObjPtr<T>. This template overload
    /// doesn't participate in overload resolution if Y* is not implicitly
    /// convertible to T*.
    ///
    template<typename Y, VGC_REQUIRES(isCompatible_<Y>)>
    ObjPtr& operator=(ObjPtr<Y>&& other) noexcept {
        if (*this != other) {
            detail::ObjPtrAccess::decref(obj_);
            obj_ = other.obj_;
            other.obj_ = nullptr;
        }
        return *this;
    }

    /// Destroys this ObjPtr<T>, destroying the managed object if its reference
    /// count reaches zero.
    ///
    ~ObjPtr() {
        detail::ObjPtrAccess::decref(obj_);
    }

    /// Accesses a member of the object managed by this ObjPtr<T>. Throws
    /// NotAliveError if this ObjPtr is null or references a not-alive Object.
    ///
    T* operator->() const {
        return getAlive_();
    }

    /// Returns a reference to the object managed by this ObjPtr<T>. Throws
    /// NotAliveError if this ObjPtr is null or references a not-alive Object.
    ///
    T& operator*() const {
        return *getAlive_();
    }

    /// Returns a pointer to the object managed by this ObjPtr<T>. This method
    /// doesn't throw, buy may return a null pointer or a not-alive object.
    ///
    T* get() const noexcept {
        return obj_;
    }

    /// Returns whether the object managed by this ObjPtr<T> is a non-null and
    /// alive object. This method doesn't throw.
    ///
    bool isAlive() const noexcept {
        return obj_ && obj_->isAlive();
    }

    /// Returns whether the object managed by this ObjPtr<T> is a non-null and
    /// alive object. This is equivalent to isAlive(), and is provided for
    /// convenience for use in boolean expression. This method doesn't throw.
    ///
    operator bool() const noexcept {
        return obj_ && obj_->isAlive();
    }

    /// Returns the refCount of the object managed by this ObjPtr<T>. Returns
    /// -1 if this ObjPtr<T> is null. This method doesn't throw.
    ///
    Int64 refCount() const noexcept {
        return obj_ ? obj_->refCount() : -1;
    }

private:
    T* obj_;

    // Returns a pointer to the object managed by this ObjPtr<T>.
    // Throws NotAliveError if this ObjPtr is null or references
    // a not-alive Object.
    //
    T* getAlive_() const {
        if (isAlive()) {
            return obj_;
        }
        else {
            throw vgc::core::NotAliveError(obj_);
        }
    }

    template<typename Y>
    friend class ObjPtr;
};

/// Returns whether the two given ObjPtrs manage the same object.
///
template<typename T, typename U>
inline bool operator==(const ObjPtr<T>& a, const ObjPtr<U>& b) noexcept {
    return a.get() == b.get();
}

/// Returns whether the two given ObjPtrs manage a different object.
///
template<typename T, typename U>
inline bool operator!=(const ObjPtr<T>& a, const ObjPtr<U>& b) noexcept {
    return a.get() != b.get();
}

template<typename T, typename U>
ObjPtr<T> static_pointer_cast(const ObjPtr<U>& r) noexcept {
    return ObjPtr<T>(static_cast<T*>(r.get()));
}

template<typename T, typename U>
ObjPtr<T> static_pointer_cast(ObjPtr<U>&& r) noexcept {
    ObjPtr<T> ret(static_cast<T*>(r.get()), typename ObjPtr<T>::DontIncRefTag{});
    r.obj_ = nullptr;
    return ret;
}

template<typename T, typename U>
ObjPtr<T> dynamic_pointer_cast(const ObjPtr<U>& r) noexcept {
    return ObjPtr<T>(dynamic_cast<T*>(r.get()));
}

template<typename T, typename U>
ObjPtr<T> dynamic_pointer_cast(ObjPtr<U>&& r) noexcept {
    T* p = dynamic_cast<T*>(r.get());
    if (p) {
        r.obj_ = nullptr;
        return ObjPtr<T>(p, typename ObjPtr<T>::DontIncRefTag{});
    }
    return ObjPtr<T>(nullptr);
}

template<typename T, typename U>
ObjPtr<T> const_pointer_cast(const ObjPtr<U>& r) noexcept {
    return ObjPtr<T>(const_cast<T*>(r.get()));
}

template<typename T, typename U>
ObjPtr<T> const_pointer_cast(ObjPtr<U>&& r) noexcept {
    ObjPtr<T> ret(const_cast<T*>(r.get()), typename ObjPtr<T>::DontIncRefTag{});
    r.obj_ = nullptr;
    return ret;
}

} // namespace vgc::core

/// This macro should appear within a private section of the class declaration
/// of any Object subclass.
///
/// ```cpp
/// class Foo : public Object {
/// private:
///     VGC_OBJECT(Foo)
/// // ...
/// };
/// ```
///
#define VGC_OBJECT(T, S)                                                                 \
public:                                                                                  \
    using ThisClass = T;                                                                 \
    using SuperClass = S;                                                                \
    /*static_assert(std::is_base_of_v<SuperClass, ThisClass>,             */             \
    /*    "ThisClass is expected to inherit from SuperClass.");           */             \
    static_assert(                                                                       \
        ::vgc::core::isObject<SuperClass>,                                               \
        "Superclass must inherit from Object and use VGC_OBJECT(..).");                  \
                                                                                         \
protected:                                                                               \
    ~T() = default;                                                                      \
                                                                                         \
private:

/// This macro ensures that unsafe base protected methods are not accessible in
/// subclasses. This macro should typically be added to a private section of
/// direct sublasses of Object, but not to indirect subclasses.
///
#define VGC_PRIVATIZE_OBJECT_TREE_MUTATORS                                               \
private:                                                                                 \
    using Object::destroyObject_;                                                        \
    using Object::destroyChildObject_;                                                   \
    using Object::appendChildObject_;                                                    \
    using Object::prependChildObject_;                                                   \
    using Object::insertChildObject_;                                                    \
    using Object::removeChildObject_;                                                    \
    using Object::appendObjectToParent_;                                                 \
    using Object::prependObjectToParent_;                                                \
    using Object::insertObjectToParent_;                                                 \
    using Object::removeObjectFromParent_;

namespace vgc::core {

// Forward declaration needed for Object::childObjects()
template<typename T>
class ObjListView;
using ObjectListView = ObjListView<Object>;

// Define the ObjectPtr name alias (we use it in the definition of Object).
//
// Note: the idea would be to simply call VGC_DECLARE_OBJECT(Object) here,
// which includes the ObjectPtr name alias. Unfortunately we can't call this
// macro here due to a cyclic dependency: the macro requires ObjList to be
// defined, which requires Object to be defined. So we defer the macro call to
// the end of this file, and simply redundantly define ObjectPtr here.
//
using ObjectPtr = ObjPtr<Object>;

/// \class vgc::core::Object
/// \brief Provides a common API for object-based tree hierarchies.
///
/// Object is the base class of some of the most important classes in the VGC
/// codebase, such as ui::Widget and dom::Node. The Object class helps
/// implementing tree hierarchies where the destruction of an Object
/// automatically destruct its children.
///
/// Defining an Object subclass
/// ---------------------------
///
/// Object subclasses should typically be declared as follows:
///
/// ```cpp
/// VGC_DECLARE_OBJECT(Foo);
///
/// class Foo : public Object {
/// private:
///     VGC_OBJECT(Foo, Object)
///
/// protected:
///     Foo();
///     Foo(Foo* parent);
///
/// public:
///     static FooPtr create();
///     // and/or
///     static Foo* create(Foo* parent);
///     // and/or
///     Foo* createChild();
/// };
/// ```
///
/// These VGC_DECLARE_OBJECT and VGC_OBJECT macros may seem strange, so let's
/// provide some explanations below as to what they do.
///
/// The `VGC_DECLARE_OBJECT(Foo);` line simply expands to the following:
///
/// ```cpp
/// class Foo;
/// using FooPtr = vgc::core::ObjPtr<Foo>;
/// using FooConstPtr = vgc::core::ObjPtr<const Foo>;
/// ```
///
/// In other words, it forward-declares `Foo`, and declares `FooPtr` as an
/// alias for `vgc::core::ObjPtr<Foo>` (as well as a const variant).
///
/// The class template `ObjPtr<T>` is a smart pointer that should be used to
/// manage the lifetime of objects without a parent, which are called "root
/// objects". Objects which do have a parent are called "child objects", and
/// are automatically destroyed by their parents, so they shouldn't be managed
/// by an `ObjPtr<T>`. In most cases, you should just use a raw pointer `T*`,
/// such as in function parameters and return values, except in the rare cases
/// where ownership is involved, such as creating a new root object, or
/// detaching a child object from its parent.
///
/// The `VGC_OBJECT(Foo, SuperClass)` macro adds a few definitions common to
/// all objects, for example, it makes the destructor protected.
///
/// The constructors should always be protected, and sublasses should instead
/// provide either static functions like `Foo::create()` or
/// `Foo::create(parent)`, or member functions like `parent->createChild()`.
/// This ensures proper use of smart pointers: functions creating root objects
/// should return ObjPtrs, and functions returning child objects should return
/// raw pointers. Note that some Object subclasses may only allow to create
/// root objects, for example a `dom::Document` is always a root. Other may
/// only allow to create child objects, for example a `dom::Element` is always
/// a child object. The decision depends on the situation, and usually comes
/// from which invariants an Object subclass desires to enforce. For example, a
/// `dom::Element` always has a valid parent, and the root is always a
/// `dom::Document`.
///
///
/// Ownership model
/// ---------------
///
/// The ownership of root objects is shared among all ObjPtrs referencing them,
/// but child objects are uniquely owned by their parent.
///
/// This means that if a root object is destroyed, then all its children are
/// also destroyed, even if there was ObjPtrs referencing these children. This
/// design make it possible to create data structures with strong hierarchical
/// invariants. For example, some objects can have a guarantee that, if alive,
/// they always have a parent.
///
/// An ObjPtr to a child object behaves like a weak pointer: it doesn't keep
/// the child alive, but it keeps some of its memory allocated in order to be
/// able to check whether it is still alive. When trying to dereference an
/// ObjPtr whose referenced object has already already been destroyed, a
/// NotAliveError is raised.
///
/// In Python, all variables bound to VGC objects are using ObjPtrs under the
/// hood. This ensures that attempting to use a destroyed child in Python never
/// crashes the program (segmentation fault), but instead raises an exception:
///
/// ```python
/// root = vgc.ui.Widget()
/// child = vgc.ui.Widget(root)
/// root = None        # Both root and child are destroyed
/// w = child.width()  # NotAlive exception raised
/// ```
///
/// Calling Conventions
/// -------------------
///
/// If a function needs an instance of vgc::core::Object as argument, it is
/// recommended to pass this object by raw pointer (unless they participate in
/// ownership, which is rare). Unless stated otherwise, any function that takes
/// an Object* as argument assumes that the following is true:
///
/// 1. The pointer is non-null.
///
/// 2. The lifetime of the object exceeds the lifetime of the function.
///
/// Please watch the first 30min of following talk by Herb Sutter for a
/// rationale (or consult the C++ Core Guidelines):
///
/// https://www.youtube.com/watch?v=xnqTKD8uD64
///
/// If an object is passed to a function by ObjPtr, this typically
/// indicates that the function takes ownership of the object. In this case,
/// pass the shared pointer directly by value, like so:
///
/// \code
/// myFunction(FooPtr foo);
/// \endcode
///
/// Miscellaneous Notes
/// -------------------
///
/// Since the destructor of Object is virtual, there is no need to explicitly
/// declare a (virtual) destructor in derived classes. In most cases, the
/// default destructor is the most appropriate.
///
/// http://en.cppreference.com/w/cpp/language/rule_of_three
/// https://blog.rmf.io/cxx11/rule-of-zero
/// http://scottmeyers.blogspot.fr/2014/03/a-concern-about-rule-of-zero.html
///
/// Note that the copy constructor, move constructor, assignement operator, and
/// move assignement operators of Object are all declared private. This means
/// that objects cannot be copied. If you desire to introduce copy-semantics to
/// a specific subclass of Object, you must manually define a clone() method.
///
class VGC_CORE_API Object {
public:
    using ThisClass = Object;

private:
    // Disable move and copy
    Object(const Object&) = delete;
    Object(Object&&) = delete;
    Object& operator=(const Object&) = delete;
    Object& operator=(Object&&) = delete;

public:
    /// Returns how many ObjPtrs are currently referencing this Object.
    ///
    /// If this Object is a root object, then the refCount represents a "strong
    /// reference count". The root object is kept alive as long as its refCount
    /// is greater than zero, and the root object is automatically destructed
    /// when its refCount becomes zero.
    ///
    /// If this Object is a child object, then the refCount represents a "weak
    /// reference count". The child object is uniquely owned by its parent:
    /// this means it can be "destroyed" (i.e., isAlive() becomes false) even
    /// if its refCount is greater than zero. However, the C++ object is not
    /// actually destructed until the refCount becomes zero, so that observers
    /// holding an `ObjPtr` to the child object can safely check the value of
    /// `isAlive()`.
    ///
    /// One way to think about this ownership model is:
    /// - An ObjPtr is acting like an std::shared_ptr for root Objects.
    /// - An ObjPtr is acting like an std::weak_ptr for child Objects.
    /// - A parent Object is acting like an std::unique_ptr for its children.
    ///
    /// This may seem a little complicated, but is quite simple to use in
    /// practice:
    ///
    /// - In C++, just use raw pointers everywhere, except for root Objects
    /// where you should use an ObjPtr. You must avoid cyclic references of
    /// ObjPtr of root objects, otherwise it will cause memory leaks.
    ///
    /// - In Python, all Objects are under the hood wrapped in an ObjPtr. This
    /// is more likely to cause cyclic references of root objects, but it's
    /// okay since the garbage collector of Python will detect those and
    /// prevent memory leaks.
    ///
    Int64 refCount() const {
        if (refCount_ >= 0) {
            return refCount_;
        }
        else {
            return refCount_ - Int64Min;
        }
    }

    /// Returns whether this Object is alive. An Object becomes non-alive
    /// either when it is has no parent and its refCount reaches zero, it when
    /// it is explicitly destroyed (for example, via `node->destroy()` in the
    /// case of dom::Node, but not all Object subclasses provide such API).
    ///
    /// Note that it is possible and even common for child Objects to be alive
    /// and have a refCount of zero.
    ///
    bool isAlive() const {
        return refCount_ >= 0;
    }

    /// Returns the parent of this Object, or nullptr if this object has no
    /// parent. An Object without parent is called a "root Object". An Object
    /// with a parent is called a "child Object".
    ///
    Object* parentObject() const {
        return parentObject_;
    }

    /// Returns the first child of this Object, of nullptr if this object has
    /// no children.
    ///
    Object* firstChildObject() const {
        return firstChildObject_;
    }

    /// Returns the last child of this Object, of nullptr if this object has
    /// no children.
    ///
    Object* lastChildObject() const {
        return lastChildObject_;
    }

    /// Returns the next sibling of this Object, of nullptr if this Object has
    /// no parent or is the last child of its parent.
    ///
    Object* nextSiblingObject() const {
        return nextSiblingObject_;
    }

    /// Returns the previous sibling of this Object, of nullptr if this Object has
    /// no parent or is the first child of its parent.
    ///
    Object* previousSiblingObject() const {
        return previousSiblingObject_;
    }

    /// Returns a range that iterates over all child objects of this Object.
    ///
    /// Example :
    ///
    /// ```cpp
    /// for (Object* child : obj->childObjects()) {
    ///     ...
    /// }
    /// ```
    ///
    ObjectListView childObjects() const;

    /// Returns the number of child objects of this Object.
    ///
    Int numChildObjects() const;

    /// Returns whether this Object is a descendant of the given \p other
    /// Object. Returns true in the special case where `this == other`. Returns
    /// false if \p other is nullptr.
    ///
    bool isDescendantObject(const Object* other) const;

    /// Prints this object tree to stdout, typically for debugging purposes.
    ///
    void dumpObjectTree() const;

    /// Disconnects the signal-slot connection represented by \p h.
    /// Returns true if the connection was present.
    ///
    bool disconnect(ConnectionHandle h) const {
        return detail::SignalHub::disconnect(this, h);
    }

    /// Removes all the signal-slot connections between this and \p receiver.
    /// Returns true if the connection was present.
    ///
    bool disconnect(const Object* receiver) const {
        return detail::SignalHub::disconnect(this, receiver);
    }

    /// Disconnects all signals bound to this object from any slots.
    ///
    void disconnect() const {
        detail::SignalHub::disconnectSignals(this);
    }

    /// Returns the number of outbound signal-slot connections.
    ///
    Int numConnections() const {
        return detail::SignalHub::numOutboundConnections(this);
    }

    /// This signal is emitted by this `object` just before it is destroyed.
    /// The object is still alive, its children have not yet been recursively
    /// destroyed, and none of the signals and slots have been automatically
    /// disconnected.
    ///
    VGC_SIGNAL(aboutToBeDestroyed, (Object*, object));

protected:
    // This callback method is invoked when this object has just been
    // destroyed, that is, just after isAlive() has switched from true to
    // false. However, note that its C++ destructor has not been called yet:
    // the destructor will be called when refCount() reaches zero, and for now
    // refCount() is still at least one.
    //
    // Implementing this callback is useful when you can release expensive
    // resources early. For example, it is often a good idea to release
    // allocated memory here: there is no need to keep it around until the
    // destructor is called. The destructor will be called when all
    // ObjectPtr observers (for example, a python variable pointing to this
    // object) are themselves destroyed.
    //
    // Note that when this method is called, all the children of this Object
    // have already been destroyed, and if this Object previsouly had a parent,
    // then it has already been removed from this parent.
    //
    virtual void onDestroyed();

    /// This callback method is invoked whenever a child has been added to this
    /// Object. When this method is called, the child has already been added
    /// to its parent, that is, `child->parent() == this`
    ///
    virtual void onChildAdded(Object* child);

    /// This callback method is invoked whenever a child has been removed from
    /// this Object. When this function is called, the child has already been removed
    /// from its parent, that is, `child->parent() == nullptr`.
    ///
    virtual void onChildRemoved(Object* child);

protected:
    /// Constructs an Object.
    ///
    Object();

    /// Calls the Object's destructor. Note that you should never call this
    /// method or `delete` directly. Under normal usage, the objects should be
    /// automatically destroyed for you: child objects are automatically
    /// destroyed by their parent, and root objects are automatically destroyed
    /// by the smart pointers managing them.
    ///
    /// Note that some Object subclasses may choose to expose a safe
    /// `destroy()` method in their public API, but it is optional.
    ///
    /// \sa destroyObject()
    ///
    virtual ~Object();

    /// Destroys this Object.
    ///
    /// This is a low-level method which should only be used by direct
    /// subclasses as a helper method to implement their API.
    ///
    /// Use VGC_PRIVATIZE_OBJECT_TREE_MUTATORS to privatize this function
    /// in your subclass.
    ///
    /// This design allows subclasses to decide whether they wish to expose a
    /// destroy() method or not. For example, a Foo subclass may, or may not,
    /// choose to define:
    ///
    /// ```cpp
    /// void Foo::destroy()
    /// {
    ///     Object::destroyObject_();
    /// }
    /// ```
    ///
    /// This method performs the following, in this order:
    ///
    /// 1. Recursively destroy the children of this Object.
    ///
    /// 2. Call the onDestroyed() callback method. At this stage, isAlive() is
    ///    still true, and parentObject() is still accessible.
    ///
    /// 3. Remove from parent. Just after removal, a side effect might be to
    ///    destroy the root of the parent, in case its refCount() becomes zero.
    ///
    /// 4. Set isAlive() to false.
    ///
    /// 5. Deallocate memory, in case the refCount() of this Object is zero.
    ///
    void destroyObject_();

    /// Destroys all children of this Object.
    ///
    /// This is a low-level method which should only be used by direct
    /// subclasses as a helper method to implement their API.
    ///
    /// Use VGC_PRIVATIZE_OBJECT_TREE_MUTATORS to privatize this function
    /// in your subclass.
    ///
    void destroyAllChildObjects_();

    /// Destroys a child of this Object. If child is null or isn't a child of
    /// this Object, then NotAChildError is raised. See destroyObject_() for
    /// details.
    ///
    /// This is a low-level method which should only be used by direct
    /// subclasses as a helper method to implement their API.
    ///
    /// Use VGC_PRIVATIZE_OBJECT_TREE_MUTATORS to privatize this function
    /// in your subclass.
    ///
    void destroyChildObject_(Object* child);

    /// Inserts the given Object as the last child of this Object. If child is
    /// null, then NullError is raised.
    ///
    /// This is a low-level method which should only be used by direct
    /// subclasses as a helper method to implement their API.
    ///
    /// Use VGC_PRIVATIZE_OBJECT_TREE_MUTATORS to privatize this function
    /// in your subclass.
    ///
    /// This design allows subclasses to add additional restrictions on which
    /// types of children are allowed. For example, a Foo subclass which only
    /// allows Foo children may define:
    ///
    /// ```cpp
    /// void Foo::appendChild(Foo* child)
    /// {
    ///     Object::appendChildObject_(child);
    /// }
    /// ```
    ///
    /// Very importantly, you must NOT use this method to append an Object
    /// whose subclass isn't your own or closely related (= friend classes, or
    /// classes developed in the same cpp file or perhaps module). Indeed, each
    /// Object subclass defines its own invariants regarding which Object
    /// subclasses they allow as children or parent. For example the children
    /// of a vgc::dom::Node are always other vgc::dom::Nodes, and a
    /// vgc::dom::Document is guaranteed to always be a root object. Therefore,
    /// using appendChildObject_() in Foo to modify a hierachies of dom::Nodes
    /// would break these invariants.
    ///
    void appendChildObject_(Object* child);

    /// Inserts the given Object as the first child of this Object. If child is
    /// null, then NullError is raised.
    ///
    /// This is a low-level method which should only be used by direct
    /// subclasses as a helper method to implement their API.
    ///
    /// Use VGC_PRIVATIZE_OBJECT_TREE_MUTATORS to privatize this function
    /// in your subclass.
    ///
    /// This design allows subclasses to add additional restrictions on which
    /// types of children are allowed. For example, a Foo subclass which only
    /// allows Foo children may define:
    ///
    /// ```cpp
    /// void Foo::prependChild(Foo* child)
    /// {
    ///     Object::prependChildObject_(child);
    /// }
    /// ```
    ///
    /// Very importantly, you must NOT use this method to prepend an Object
    /// whose subclass isn't your own or closely related (= friend classes, or
    /// classes developed in the same cpp file or perhaps module). Indeed, each
    /// Object subclass defines its own invariants regarding which Object
    /// subclasses they allow as children or parent. For example the children
    /// of a vgc::dom::Node are always other vgc::dom::Nodes, and a
    /// vgc::dom::Document is guaranteed to always be a root object. Therefore,
    /// using appendChildObject_() in Foo to modify a hierachies of dom::Nodes
    /// would break these invariants.
    ///
    void prependChildObject_(Object* child);

    /// Inserts the given Object as a child of this Object, just before
    /// nextSibling. If child is null, then NullError is raised. If nextSibling
    /// is null, then it is inserted last. If nextSibling is non-null but isn't
    /// a child of this Object, then NotAChildError is raised.
    ///
    /// This is a low-level method which should only be used by direct
    /// subclasses as a helper method to implement their API.
    ///
    /// Use VGC_PRIVATIZE_OBJECT_TREE_MUTATORS to privatize this function
    /// in your subclass.
    ///
    /// This design allows subclasses to add additional restrictions on which
    /// types of children are allowed. For example, a Foo subclass which only
    /// allows Foo children may define:
    ///
    /// ```cpp
    /// void Foo::insertChild(Foo* child, Foo* nextSibling)
    /// {
    ///     Object::insertChildObject_(child, nextSibling);
    /// }
    /// ```
    ///
    /// Very importantly, you must NOT use this method to insert an Object
    /// whose subclass isn't your own or closely related (= friend classes, or
    /// classes developed in the same cpp file or perhaps module). Indeed, each
    /// Object subclass defines its own invariants regarding which Object
    /// subclasses they allow as children or parent. For example the children
    /// of a vgc::dom::Node are always other vgc::dom::Nodes, and a
    /// vgc::dom::Document is guaranteed to always be a root object. Therefore,
    /// using appendChildObject_() in Foo to modify a hierachies of dom::Nodes
    /// would break these invariants.
    ///
    void insertChildObject_(Object* child, Object* nextSibling = nullptr);

    /// Removes the given Object from the list of children of this Object.
    /// Returns an ObjPtr<Object> managing the given Object which is now a root
    /// Object and therefore not uniquely owned by its parent. If child is null or isn't a
    /// child of this Object, then NotAChildError is raised.
    ///
    /// This is a low-level method which should only be used by direct
    /// subclasses to implement their API.
    ///
    /// Use VGC_PRIVATIZE_OBJECT_TREE_MUTATORS to privatize this function
    /// in your subclass.
    ///
    /// This design allows subclasses to add additional restrictions on which
    /// types of children are allowed, and whether removing children is allowed
    /// for this subclass of Object. For example, a Foo subclass which only
    /// allows Foo children may define:
    ///
    /// ```cpp
    /// void Foo::removeChild(Foo* child)
    /// {
    ///     Object::removeChildObject_(child);
    /// }
    /// ```
    ///
    /// Very importantly, you must NOT use this method to remove an Object
    /// whose subclass isn't your own or closely related (= friend classes, or
    /// classes developed in the same cpp file or perhaps module). Indeed, each
    /// Object subclass defines its own invariants regarding which Object
    /// subclasses they allow as children or parent. For example the children
    /// of a vgc::dom::Node are always other vgc::dom::Nodes, and a
    /// vgc::dom::Document is guaranteed to always be a root object. Therefore,
    /// using removeChildObject_() in Foo to modify a hierachies of dom::Nodes
    /// would break these invariants.
    ///
    ObjectPtr removeChildObject_(Object* child);

    /// Inserts this Object as the last child of the given parent Object. If
    /// parent is null, this function is equivalent to \p
    /// removeObjectFromParent_();
    ///
    /// This is a low-level method which should only be used by direct
    /// subclasses to implement their API.
    ///
    /// Use VGC_PRIVATIZE_OBJECT_TREE_MUTATORS to privatize this function
    /// in your subclass.
    ///
    /// Very importantly, you must NOT use this method to append an Object
    /// whose subclass isn't your own or closely related (= friend classes, or
    /// classes developed in the same cpp file or perhaps module). Indeed, each
    /// Object subclass defines its own invariants regarding which Object
    /// subclasses they allow as children or parent. For example the children
    /// of a vgc::dom::Node are always other vgc::dom::Nodes, and a
    /// vgc::dom::Document is guaranteed to always be a root object. Therefore,
    /// using appendObjectToParent_() in Foo to modify a hierachies of
    /// dom::Nodes would break these invariants.
    ///
    void appendObjectToParent_(Object* parent);

    /// Inserts this Object as the first child of the given parent Object. If
    /// parent is null, this function is equivalent to \p
    /// removeObjectFromParent_();
    ///
    /// This is a low-level method which should only be used by direct
    /// subclasses to implement their API.
    ///
    /// Use VGC_PRIVATIZE_OBJECT_TREE_MUTATORS to privatize this function
    /// in your subclass.
    ///
    /// Very importantly, you must NOT use this method to prepend an Object
    /// whose subclass isn't your own or closely related (= friend classes, or
    /// classes developed in the same cpp file or perhaps module). Indeed, each
    /// Object subclass defines its own invariants regarding which Object
    /// subclasses they allow as children or parent. For example the children
    /// of a vgc::dom::Node are always other vgc::dom::Nodes, and a
    /// vgc::dom::Document is guaranteed to always be a root object. Therefore,
    /// using prependObjectToParent_() in Foo to modify a hierachies of
    /// dom::Nodes would break these invariants.
    ///
    void prependObjectToParent_(Object* parent);

    /// Inserts this Object as a child of the given parent Object, just before
    /// nextSibling. If nextSibling is null, then it is inserted last. If
    /// nextSibling is non-null but isn't a child of the given parent, then
    /// NotAChildError is raised. If parent is null, this function is
    /// equivalent to \p removeObjectFromParent_();
    ///
    /// This is a low-level method which should only be used by direct
    /// subclasses to implement their API.
    ///
    /// Use VGC_PRIVATIZE_OBJECT_TREE_MUTATORS to privatize this function
    /// in your subclass.
    ///
    /// Very importantly, you must NOT use this method to insert an Object
    /// whose subclass isn't your own or closely related (= friend classes, or
    /// classes developed in the same cpp file or perhaps module). Indeed, each
    /// Object subclass defines its own invariants regarding which Object
    /// subclasses they allow as children or parent. For example the children
    /// of a vgc::dom::Node are always other vgc::dom::Nodes, and a
    /// vgc::dom::Document is guaranteed to always be a root object. Therefore,
    /// using insertObjectToParent_() in Foo to modify a hierachies of
    /// dom::Nodes would break these invariants.
    ///
    void insertObjectToParent_(Object* parent, Object* nextSibling);

    /// Removes this Object from the list of children of its parent.
    /// Returns an ObjPtr<Object> managing this Object which is now a root
    /// Object and therefore not uniquely owned by its parent. Nothing happens
    /// if this Object is already a root Object.
    ///
    /// This is a low-level method which should only be used by direct
    /// subclasses to implement their API.
    ///
    /// Use VGC_PRIVATIZE_OBJECT_TREE_MUTATORS to privatize this function
    /// in your subclass.
    ///
    ObjPtr<Object> removeObjectFromParent_();

    Int branchSize() const;

private:
    // Reference counting.
    //
    // Note that `refCount_` is used to store both `refCount()` and `isAlive()`,
    // which are semantically independent variables.
    //
    // If refCount_ >= 0, then:
    //     isAlive() = true
    //     refCount() = refCount_
    //
    // If refCount_ < 0, then:
    //     isAlive() = false
    //     refCount() = refCount_ - Int64Min
    //
    friend class detail::ObjPtrAccess;
    mutable Int64 refCount_ = 0;

    // Parent-child relationship
    Object* parentObject_ = nullptr;
    Object* firstChildObject_ = nullptr;
    Object* lastChildObject_ = nullptr;
    Object* previousSiblingObject_ = nullptr;
    Object* nextSiblingObject_ = nullptr;
    Int numChildObjects_ = 0;

    // Deferred values
    mutable bool isBranchSizeDirty_ = false;
    Int branchSize_ = 0;

    // Signal-slot mechanism
    friend class detail::SignalHub; // To access signalHub_
    mutable detail::SignalHub signalHub_;

    void destroyObjectImpl_();

    void setBranchSizeDirty_();
    void updateBranchSize_() const;

    void onChildAdded_(Object* child);
    void onChildRemoved_(Object* child);
};

namespace detail {

// Required by signal.h
constexpr SignalHub& SignalHub::access(const Object* o) {
    return const_cast<Object*>(o)->signalHub_;
}

inline void ObjPtrAccess::incref(const Object* obj, Int64 k) {
    if (obj) {
        obj->refCount_ += k;
    }
}

inline void ObjPtrAccess::decref(const Object* obj, Int64 k) {
    if (obj) {
        obj->refCount_ -= k;
        bool isRoot = (obj->parentObject_ == nullptr);
        if (isRoot) {
            if (obj->refCount_ == 0) {
                // Here, isAlive() == true and refCount() == 0.
                // See implementation of destroyObjectImpl_() for details on the
                // const-cast and other subtleties of the following two lines.
                Object* root_ = const_cast<Object*>(obj);
                root_->destroyObjectImpl_();
            }
            else if (obj->refCount_ == Int64Min) {
                // Here, isAlive() == false and refCount() == 0.
                delete obj;
            }
        }
    }
}

} // namespace detail

/// \class vgc::core::ObjListIterator
/// \brief Iterates over an ObjList
///
template<typename T>
class ObjListIterator {
public:
    typedef T* value_type;
    typedef Int difference_type;
    typedef const value_type& reference; // 'const' => non-mutable forward iterator
    typedef const value_type* pointer;   // 'const' => non-mutable forward iterator
    typedef std::forward_iterator_tag iterator_category;

    /// Constructs an invalid `ObjListIterator`.
    ///
    ObjListIterator()
        : p_(nullptr) {
    }

    /// Constructs an iterator pointing to the given object.
    ///
    explicit ObjListIterator(T* p)
        : p_(p) {
    }

    /// Prefix-increments this iterator.
    ///
    ObjListIterator& operator++() {
        p_ = static_cast<T*>(p_->nextSiblingObject());
        return *this;
    }

    /// Postfix-increments this iterator.
    ///
    ObjListIterator operator++(int) {
        ObjListIterator res(*this);
        operator++();
        return res;
    }

    /// Dereferences this iterator with the star operator.
    ///
    const value_type& operator*() const {
        return p_;
    }

    /// Dereferences this iterator with the arrow operator.
    ///
    const value_type* operator->() const {
        return &p_;
    }

    /// Returns whether the two iterators are equal.
    ///
    bool operator==(const ObjListIterator& other) const {
        return p_ == other.p_;
    }

    /// Returns whether the two iterators are different.
    ///
    bool operator!=(const ObjListIterator& other) const {
        return p_ != other.p_;
    }

private:
    T* p_;
};

template<typename T>
class ObjList;

/// \class vgc::core::ObjListView
/// \brief A non-owning view onto an ObjList<T>, or onto any range of sibling
///        objects of type T.
///
/// This is a range-like class to make it convenient to iterate over sibling
/// Object instances sharing the same type T, such as for example an
/// ObjList<T>.
///
/// Example:
///
/// \code
/// for (Widget* child : widget->children()) {
///     // ...
/// }
/// \endcode
///
/// \sa ObjList
///
template<typename T>
class ObjListView {
public:
    /// Constructs an ObjListView from the given ObjList.
    ///
    ObjListView(ObjList<T>* list)
        : begin_(list->first())
        , end_(nullptr) {
    }

    /// Constructs a range of sibling objects from \p begin to \p end. The
    /// range includes \p begin but excludes \p end. The behavior is undefined
    /// if \p begin and \p end do not have the same parent. If \p end is null,
    /// then the range includes all siblings after \p begin. If both \p start
    /// and \p end are null, then the range is empty. The behavior is undefined
    /// if \p begin is null but \p end is not.
    ///
    ObjListView(T* begin, T* end)
        : begin_(begin)
        , end_(end) {
    }

    /// Returns the begin of the range.
    ///
    const ObjListIterator<T>& begin() const {
        return begin_;
    }

    /// Returns the end of the range.
    ///
    const ObjListIterator<T>& end() const {
        return end_;
    }

    /// Returns the number of objects in the range.
    ///
    /// Note that this function is slow (linear complexity), because it has to
    /// iterate over all objects in the range.
    ///
    Int length() const {
        return std::distance(begin_, end_);
    }

private:
    ObjListIterator<T> begin_;
    ObjListIterator<T> end_;
};

/// \class vgc::core::ObjList
/// \brief Stores a list of child objects of a given type T.
///
/// An ObjList<T> is an Object that stores and manages a list of Objects of
/// type T. The ObjList owns the contained objects, that is, the ObjList is the
/// parent object of the contained objects.
///
/// Using an ObjList is the preferred way for a given object `a` to own a
/// list of other objects `b_i`. Note that this means that the `b_i` objects
/// are not direct child objects of `a`, but are instead grand-child objects:
///
/// ```
/// a
/// └ list
///    ├ b_0
///    ├ b_0
///    │ ...
///    └ b_n
/// ```
///
/// Instead of using the above design, one could be tempted to have `a` own the
/// `b_i` as direct child objects. It is indeed possible, but a problem is that
/// whenever `a` needs to own other unrelated objects, they would all be
/// sibling objects, which is not as clean semantically, and makes it bug-prone
/// and harder to maintain/debug. Using an ObjList ensures that sibling objects
/// are part of the same semantic "list", which makes the design more robust
/// and extensible. ObjList also provide convenient methods to make iterating
/// easier.
///
template<typename T>
class ObjList : public Object {
private:
    VGC_OBJECT(ObjList<T>, Object)
    VGC_PRIVATIZE_OBJECT_TREE_MUTATORS

protected:
    ObjList() {
    }

public:
    static ObjList* create(Object* parent) {
        ObjList* res = new ObjList();
        res->appendObjectToParent_(parent);
        return res;
    }

    T* first() {
        return static_cast<T*>(firstChildObject());
    }

    T* last() {
        return static_cast<T*>(lastChildObject());
    }

    void append(T* child) {
        appendChildObject_(child);
    }

    void insert(T* child, T* nextSibling) {
        insertChildObject_(child, nextSibling);
    }

    void insertAt(Int i, T* child) {
        if (i < 0 || i > numChildObjects()) {
            throw IndexError(core::format("Cannot insert child in list at index {}.", i));
        }
        Object* nextSibling = firstChildObject();
        for (; i > 0; --i) {
            nextSibling = nextSibling->nextSiblingObject();
        }
        insertChildObject_(child, nextSibling);
    }

    core::ObjPtr<T> remove(T* child) {
        core::ObjPtr<Object> removed = removeChildObject_(child);
        core::ObjPtr<T> res(static_cast<T*>(removed.get()));
        return res;

        // Note: if we were using std::shared_ptr, then we would have to use
        // std::static_pointer_cast instead, otherwise the reference counting
        // block wouldn't be shared. But in our case, the above static_cast
        // works since our core::ObjPtr use intrusive reference counting.
    }

    void onChildAdded(Object* child) override {
        childAdded().emit(static_cast<T*>(child));
    }

    void onChildRemoved(Object* child) override {
        childRemoved().emit(static_cast<T*>(child));
    }

    VGC_SIGNAL(childAdded, (T*, child));
    VGC_SIGNAL(childRemoved, (T*, child));
};

inline ObjectListView Object::childObjects() const {
    return ObjectListView(firstChildObject(), nullptr);
}

inline Int Object::numChildObjects() const {
    return numChildObjects_;
}

} // namespace vgc::core

/// Forward-declares the given object subclass, and define convenient name
/// aliases for its related smart pointers and list classes. More specifically,
/// the following code:
///
/// ```cpp
/// VGC_DECLARE_OBJECT(Foo);
/// ```
///
/// expands to:
///
/// ```cpp
/// class Foo;
/// using FooPtr          = vgc::core::ObjPtr<Foo>;
/// using FooConstPtr     = vgc::core::ObjPtr<const Foo>;
/// using FooList         = vgc::core::ObjList<Foo>;
/// using FooListIterator = vgc::core::ObjListIterator<Foo>;
/// using FooListView     = vgc::core::ObjListView<Foo>;
/// ```
///
/// This macro should typically be called in the header file `foo.h` just
/// before declaring the `Foo` class, or in other header files when you need to
/// use the `Foo` and/or `FooPtr` types but cannot include "foo.h" due to cylic
/// dependencies.
///
/// Like when using traditional forward-declarations, you should call this
/// macro in the same namespace as Foo, for example:
///
/// ```cpp
/// namespace vgc {
/// namespace ui {
/// VGC_DECLARE_OBJECT(Widget);
/// }
/// }
/// ```
///
#define VGC_DECLARE_OBJECT(T)                                                            \
    class T;                                                                             \
    using T##Ptr = vgc::core::ObjPtr<T>;                                                 \
    using T##ConstPtr = vgc::core::ObjPtr<const T>;                                      \
    using T##List = vgc::core::ObjList<T>;                                               \
    using T##ListIterator = vgc::core::ObjListIterator<T>;                               \
    using T##ListView = vgc::core::ObjListView<T>

namespace vgc::core {

VGC_DECLARE_OBJECT(Object);

} // namespace vgc::core

namespace vgc::core::detail {

VGC_DECLARE_OBJECT(ConstructibleTestObject);

// XXX add a create() in Object directly?
class ConstructibleTestObject : public Object {
    VGC_OBJECT(ConstructibleTestObject, Object)

public:
    static ConstructibleTestObjectPtr create() {
        return new ConstructibleTestObject();
    }
};

VGC_DECLARE_OBJECT(SignalTestObject);

class VGC_CORE_API SignalTestObject : public Object {
    VGC_OBJECT(SignalTestObject, Object)

public:
    static SignalTestObjectPtr create() {
        return new SignalTestObject();
    }

    static inline bool sfnIntCalled = false;
    Int slotNoargsCallCount = 0;
    int sumInt = 0;
    float sumFloat = 0.;

    void reset() {
        sfnIntCalled = false;
        slotNoargsCallCount = 0;
        sumInt = 0;
        sumFloat = 0.;
    }

    void slotNoArgs_() {
        ++slotNoargsCallCount;
    }

    void slotFloat_(float a) {
        this->sumFloat += a;
    }

    void slotUInt_(unsigned int a) {
        this->sumInt += a;
    }

    void slotInt_(int a) {
        this->sumInt += a;
    }

    void slotConstIntRef_(const int& a) {
        this->sumInt += a;
    }

    void slotIncIntRef_(int& a) {
        ++a;
    }

    void slotIntFloat_(int a, float b) {
        this->sumInt += a;
        this->sumFloat += b;
    }

    static inline void staticFuncInt() {
        sfnIntCalled = true;
    }

    void connectToOtherNoArgs(SignalTestObject* other) const;

    VGC_SIGNAL(signalNoArgs);
    VGC_SIGNAL(signalInt, (int, a));
    VGC_SIGNAL(signalIntRef, (int&, a));
    VGC_SIGNAL(signalConstIntRef, (const int&, a));
    VGC_SIGNAL(signalIntFloat, (int, a), (float, b));
    VGC_SIGNAL(signalIntFloatBool, (int, a), (float, b), (bool, c));

    VGC_SLOT(slotNoArgs, slotNoArgs_);
    VGC_SLOT(slotFloat, slotFloat_);
    VGC_SLOT(slotUInt, slotUInt_);
    VGC_SLOT(slotInt, slotInt_);
    VGC_SLOT(slotConstIntRef, slotConstIntRef_);
    VGC_SLOT(slotIncIntRef, slotIncIntRef_);
    VGC_SLOT(slotIntFloat, slotIntFloat_);
};

} // namespace vgc::core::detail

#endif // VGC_CORE_OBJECT_H
