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

#ifndef VGC_CORE_OBJECT_H
#define VGC_CORE_OBJECT_H

#include <type_traits>

#include <vgc/core/api.h>
#include <vgc/core/arithmetic.h>
#include <vgc/core/exceptions.h>

namespace vgc {
namespace core {

class Object;

namespace internal {

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

} // namespace internal

/// \class ObjPtr<T>
/// \brief Smart pointer for managing the lifetime of Object instances.
///
/// See documentation of Object for more details.
///
template<typename T>
class ObjPtr {
private:
    template<typename Y>
    using Compatible_ = typename std::enable_if<
        std::is_convertible<Y*, T*>::value>::type;

public:
    /// Creates a null ObjPtr<T>, that is, an ObjPtr<T> which doesn't manage any
    /// Object.
    ///
    ObjPtr() noexcept : obj_(nullptr)
    {

    }

    /// Creates an ObjPtr<T> managing the given Object.
    ///
    /// Note that in the current implementation, we use intrusive reference
    /// counting and therefore this constructor never throws and could be
    /// marked noexept. However, you shouldn't rely on this: a future
    /// implementation may implement referencing counting via
    /// separately-allocated counter blocks, whose memory allocation may throw.
    ///
    ObjPtr(T* obj) : obj_(obj)
    {
        internal::ObjPtrAccess::incref(obj_);
    }

    /// Creates a copy of the given ObjPtr<T>.
    ///
    ObjPtr(const ObjPtr& other) noexcept :
        obj_(other.obj_)
    {
        internal::ObjPtrAccess::incref(obj_);
    }

    /// Creates a copy of the given ObjPtr<Y>. This template overload doesn't
    /// participate in overload resolution if Y* is not implicitly convertible
    /// to T*.
    ///
    template<typename Y, typename = Compatible_<Y>>
    ObjPtr(const ObjPtr<Y>& other) noexcept :
        obj_(other.obj_)
    {
        internal::ObjPtrAccess::incref(obj_);
    }

    /// Assigns the given ObjPtr<T> to this ObjPtr<T>.
    ///
    ObjPtr& operator=(const ObjPtr& other) noexcept
    {
        if(obj_ != other.obj_) {
            internal::ObjPtrAccess::decref(obj_);
            obj_ = other.obj_;
            internal::ObjPtrAccess::incref(obj_);
        }
        return *this;
    }

    /// Assigns the given ObjPtr<Y> to this ObjPtr<T>. This template overload
    /// doesn't participate in overload resolution if Y* is not implicitly
    /// convertible to T*.
    ///
    template<typename Y, typename = Compatible_<Y>>
    ObjPtr& operator=(const ObjPtr<Y>& other) noexcept
    {
        if(obj_ != other.obj_) {
            internal::ObjPtrAccess::decref(obj_);
            obj_ = other.obj_;
            internal::ObjPtrAccess::incref(obj_);
        }
        return *this;
    }

    /// Moves the given ObjPtr<T> to a new ObjPtr<T>.
    ///
    ObjPtr(ObjPtr&& other) noexcept : obj_(other.obj_)
    {
        other.obj_ = nullptr;
    }

    /// Moves the given ObjPtr<Y> to a new ObjPtr<T>. This template overload
    /// doesn't participate in overload resolution if Y* is not implicitly
    /// convertible to T*.
    ///
    template<typename Y, typename = Compatible_<Y>>
    ObjPtr(ObjPtr<Y>&& other) noexcept : obj_(other.obj_)
    {
        other.obj_ = nullptr;
    }

    /// Moves the given ObjPtr<T> to this ObjPtr<T>.
    ///
    ObjPtr& operator=(ObjPtr&& other) noexcept
    {
        if (*this != other) {
            internal::ObjPtrAccess::decref(obj_);
            obj_ = other.obj_;
            other.obj_ = nullptr;
        }
        return *this;
    }

    /// Moves the given ObjPtr<Y> to this ObjPtr<T>. This template overload
    /// doesn't participate in overload resolution if Y* is not implicitly
    /// convertible to T*.
    ///
    template<typename Y, typename = Compatible_<Y>>
    ObjPtr& operator=(ObjPtr<Y>&& other) noexcept
    {
        if (*this != other) {
            internal::ObjPtrAccess::decref(obj_);
            obj_ = other.obj_;
            other.obj_ = nullptr;
        }
        return *this;
    }

    /// Destroys this ObjPtr<T>, destroying the managed object if its reference
    /// count reaches zero.
    ///
    ~ObjPtr()
    {
        internal::ObjPtrAccess::decref(obj_);
    }

    /// Accesses a member of the object managed by this ObjPtr<T>. Throws
    /// NotAliveError if this ObjPtr is null or references a not-alive Object.
    ///
    T* operator->() const
    {
        return getAlive_();
    }

    /// Returns a reference to the object managed by this ObjPtr<T>. Throws
    /// NotAliveError if this ObjPtr is null or references a not-alive Object.
    ///
    T& operator*() const
    {
        return *getAlive_();
    }

    /// Returns a pointer to the object managed by this ObjPtr<T>. This method
    /// doesn't throw, buy may return a null pointer or a not-alive object.
    ///
    T* get() const noexcept
    {
        return obj_;
    }

    /// Returns whether the object managed by this ObjPtr<T> is a non-null and
    /// alive object. This method doesn't throw.
    ///
    bool isAlive() const noexcept
    {
        return obj_ && obj_->isAlive();
    }

    /// Returns whether the object managed by this ObjPtr<T> is a non-null and
    /// alive object. This is equivalent to isAlive(), and is provided for
    /// convenience for use in boolean expression. This method doesn't throw.
    ///
    operator bool() const noexcept
    {
        return obj_ && obj_->isAlive();
    }

    /// Returns the refCount of the object managed by this ObjPtr<T>. Returns
    /// -1 if this ObjPtr<T> is null. This method doesn't throw.
    ///
    Int64 refCount() const noexcept
    {
        return obj_ ? obj_->refCount() : -1;
    }

private:
    T* obj_;

    // Returns a pointer to the object managed by this ObjPtr<T>.
    // Throws NotAliveError if this ObjPtr is null or references
    // a not-alive Object.
    //
    T* getAlive_() const
    {
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
inline bool operator==(const ObjPtr<T>& a, const ObjPtr<U>& b) noexcept
{
    return a.get() == b.get();
}

/// Returns whether the two given ObjPtrs manage a different object.
///
template<typename T, typename U>
inline bool operator!=(const ObjPtr<T>& a, const ObjPtr<U>& b) noexcept
{
    return a.get() != b.get();
}

} // namespace core
} // namespace vgc

/// Forward-declares the given object subclass and its related smart pointers.
/// More specifically, the following code:
///
/// ```cpp
/// VGC_DECLARE_OBJECT(Foo);
/// ```
///
/// expands to:
///
/// ```cpp
/// class Foo;
/// using FooPtr = vgc::core::ObjPtr<Foo>;
/// using FooConstPtr = vgc::core::ObjPtr<const Foo>;
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
#define VGC_DECLARE_OBJECT(T)                      \
    class T;                                       \
    using T##Ptr      = vgc::core::ObjPtr<T>;      \
    using T##ConstPtr = vgc::core::ObjPtr<const T>

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
/// For now, it simply ensures that the destructor is protected. In the future,
/// it may also provide some introspection features.
///
#define VGC_OBJECT(T)                                      \
    protected:                                             \
        ~T() override = default;                           \
    private:

/// This macro ensures that unsafe base protected methods are not accessible in
/// subclasses. This macro should typically be added to a private section of
/// direct sublasses of Object, but not to indirect subclasses.
///
#define VGC_PRIVATIZE_OBJECT_TREE_MUTATORS                 \
    private:                                               \
        using Object::destroyObject_;                      \
        using Object::destroyChildObject_;                 \
        using Object::appendChildObject_;                  \
        using Object::prependChildObject_;                 \
        using Object::insertChildObject_;                  \
        using Object::removeChildObject_;                  \
        using Object::appendObjectToParent_;               \
        using Object::prependObjectToParent_;              \
        using Object::insertObjectToParent_;               \
        using Object::removeObjectFromParent_;

namespace vgc {
namespace core {

VGC_DECLARE_OBJECT(Object);

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
///     VGC_OBJECT(Foo)
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
/// The `VGC_OBJECT(Foo)` macro adds a few definitions common to all objects,
/// for example, it makes the destructor protected.
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
/// The ownership model of Object instances is a little peculiar: child objects
/// are uniquely owned by their parent, but the ownership or root objects is
/// shared among all ObjPtrs referencing them or any of their descendants.
///
/// One of the main reasons behind this design is to improve interoperability
/// between C++ (which doesn't have a garbage collector), and Python (which
/// does have a garbage collector).
///
/// In Python, all variables bound to VGC objects are using ObjPtrs under the
/// hood. This ensures that a root object isn't destroyed as long as at least
/// one of its descendant (including itself) is still referenced:
///
/// ```python
/// root = vgc.ui.Widget()
/// child = vgc.ui.Widget(root)
/// root = None     # Both root and child are kept alive
/// child = None    # Destroys child, then destroys root
/// ```
///
/// However, note that an ObjPtr referencing a child object doesn't guarantee
/// that the child object itself is kept alive: it only guarantees that its
/// root is kept alive. This is because parents have unique ownership of their
/// children, which means that they have full power to destroy any of their
/// children if they want to.
///
/// When trying to dereference an ObjPtr whose underlying object has already
/// been destroyed by its parent, a NotAliveError is raised.
///
/// Calling Conventions
/// -------------------
///
/// Functions manipulating instances of vgc::core::Object should be passed
/// these objects by raw pointers (that is, unless they participate in
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
private:
    // Disable move and copy
    Object(const Object&) = default;
    Object(Object&&) = default;
    Object& operator=(const Object&) = default;
    Object& operator=(Object&&) = default;

public:
    /// Returns how many ObjPtrs are currently referencing this Object or any
    /// of its descendants.
    ///
    /// Below is an example of an Object tree consisting of 8 Objects, where 6
    /// ObjPtrs are referencing some Object in the tree. Each `[i]` represents
    /// an Object whose refCount is `i`.
    ///
    /// ```
    /// [6]         <- ObjPtr
    ///  ├─[3]
    ///  │  ├─[1]   <- ObjPtr
    ///  │  ├─[2]   <- ObjPtr <- ObjPtr
    ///  │  └─[0]
    ///  ├─[0]
    ///  └─[2]      <- ObjPtr
    ///     └─[1]   <- ObjPtr
    /// ```
    ///
    /// The refCount of an Object is always equal to the number of ObjPtrs
    /// referencing this Object, plus the sum of the refCounts of all of its
    /// direct children.
    ///
    /// Note that the role of ObjPtrs is two-fold:
    /// 1. Keep root Objects alive.
    /// 2. Throw an exception when trying to access a non-alive Object.
    ///
    /// However, the role of ObjPtrs is NOT to keep child Objects alive.
    /// Instead, child Objects are uniquely owned by their parent. This means
    /// that parent Objects can decide to destroy their children regardless of
    /// their refCount, and child Objects aren't automatically destroyed when
    /// their refCount reaches zero.
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
    /// ObjPtr, otherwise it will cause memory leaks.
    ///
    /// - In Python, all Objects are under the hood wrapped in an ObjPtr. This
    /// is likely to cause cyclic references, but it's okay since the garbage
    /// collector of Python will detect those and prevent memory leaks.
    ///
    Int64 refCount() const
    {
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
    bool isAlive() const
    {
        return refCount_ >= 0;
    }

    /// Returns the parent of this Object, or nullptr if this object has no
    /// parent. An Object without parent is called a "root Object". An Object
    /// with a parent is called a "child Object".
    ///
    Object* parentObject() const
    {
        return parentObject_;
    }

    /// Returns the first child of this Object, of nullptr if this object has
    /// no children.
    ///
    Object* firstChildObject() const
    {
        return firstChildObject_;
    }

    /// Returns the last child of this Object, of nullptr if this object has
    /// no children.
    ///
    Object* lastChildObject() const
    {
        return lastChildObject_;
    }

    /// Returns the next sibling of this Object, of nullptr if this Object has
    /// no parent or is the last child of its parent.
    ///
    Object* nextSiblingObject() const
    {
        return nextSiblingObject_;
    }

    /// Returns the previous sibling of this Object, of nullptr if this Object has
    /// no parent or is the first child of its parent.
    ///
    Object* previousSiblingObject() const
    {
        return previousSiblingObject_;
    }

    /// Returns whether this Object is a descendant of the given \p other
    /// Object. Returns true in the special case where `this == other`. Returns
    /// false if \p other is nullptr.
    ///
    bool isDescendantObject(const Object* other) const;

    /// Prints this object tree to stdout, typically for debugging purposes.
    ///
    void dumpObjectTree() const;

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
    /// subclasses as a helper method to implement their API. This method is
    /// automatically made private by the VGC_OBJECT macro. This design allows
    /// subclasses to decide whether they wish to expose a destroy() method or
    /// not. For example, a Foo subclass may, or may not, choose to define:
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

    /// Destroys a child of this Object. If child is null or isn't a child of
    /// this Object, then NotAChildError is raised. See destroyObject_() for
    /// details.
    ///
    /// This is a low-level method which should only be used by direct
    /// subclasses as a helper method to implement their API. This method is
    /// automatically made private by the VGC_OBJECT macro.
    ///
    void destroyChildObject_(Object* child);

    /// Inserts the given Object as the last child of this Object. If child is
    /// null, then NullError is raised.
    ///
    /// This is a low-level method which should only be used by direct
    /// subclasses as a helper method to implement their API. This method is
    /// automatically made private by the VGC_OBJECT macro. This design allows
    /// subclasses to add additional restrictions on which types of children
    /// are allowed. For example, a Foo subclass which only allows Foo children
    /// may define:
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
    /// subclasses as a helper method to implement their API. This method is
    /// automatically made private by the VGC_OBJECT macro. This design allows
    /// subclasses to add additional restrictions on which types of children
    /// are allowed. For example, a Foo subclass which only allows Foo children
    /// may define:
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
    /// subclasses as a helper method to implement their API. This method is
    /// automatically made private by the VGC_OBJECT macro. This design allows
    /// subclasses to add additional restrictions on which types of children
    /// are allowed. For example, a Foo subclass which only allows Foo children
    /// may define:
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
    /// subclasses to implement their API. This method is automatically made
    /// private by the VGC_OBJECT macro. This design allows subclasses to add
    /// additional restrictions on which types of children are allowed, and
    /// whether removing children is allowed for this subclass of Object. For
    /// example, a Foo subclass which only allows Foo children may define:
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
    /// subclasses to implement their API. This method is automatically made
    /// private by the VGC_OBJECT macro.
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
    /// subclasses to implement their API. This method is automatically made
    /// private by the VGC_OBJECT macro.
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
    /// subclasses to implement their API. This method is automatically made
    /// private by the VGC_OBJECT macro.
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
    /// subclasses to implement their API. This method is automatically made
    /// private by the VGC_OBJECT macro.
    ///
    ObjectPtr removeObjectFromParent_();

private:
    friend class internal::ObjPtrAccess;
    mutable Int64 refCount_; // If >= 0: isAlive = true, refCount = refCount_
                             // If < 0:  isAlive = false, refCount = refCount_ - Int64Min
    Object* parentObject_;
    Object* firstChildObject_;
    Object* lastChildObject_;
    Object* previousSiblingObject_;
    Object* nextSiblingObject_;

    void destroyObjectImpl_();
};

namespace internal {

inline void ObjPtrAccess::incref(const Object* obj, Int64 k)
{
    while (obj) {
        obj->refCount_ += k;
        obj = obj->parentObject_;
    }
}

inline void ObjPtrAccess::decref(const Object* obj, Int64 k)
{
    const Object* root = nullptr;
    while (obj) {
        root = obj;
        obj->refCount_ -= k;
        obj = obj->parentObject_;
    }
    if (root) {
        if (root->refCount_ == 0) {
            // Here, isAlive() == true and refCount() == 0.
            // See implementation of destroyObjectImpl_() for details on the
            // const-cast and other subtleties of the following two lines.
            Object* root_ = const_cast<Object*>(root);
            root_->destroyObjectImpl_();
        }
        else if (root->refCount_ == Int64Min) {
            // Here, isAlive() == false and refCount() == 0.
            delete root;
        }
    }
}

} // namespace internal

} // namespace core
} // namespace vgc

#endif // VGC_CORE_OBJECT_H
