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
#include <vgc/core/format.h>
#include <vgc/core/stringid.h>

#include <vgc/core/detail/signal.h>

namespace vgc::core {

class Object;
using ConnectionHandle = detail::ConnectionHandle;

template<typename T>
class ObjPtr;

/// \enum vgc::core::ObjectStage
/// \brief The different stages of construction / destruction of an `Object`.
///
/// This enum describes the ordered sequence of stages an `Object` goes through
/// during its lifetime.
///
/// Note that the integer value of `Object::stage()` is guaranteed to be
/// increasing for a given object, so in order to test whether an object is at
/// least at a given stage, you can use expressions such as:
///
/// ```cpp
/// if (stage() >= ObjectStage::ChildrenDestroyed) {
///     ...
/// }
/// ```
///
/// Or equivalently, you can simply use
/// `hasReachedStage(ObjectStage::ChildrenDestroyed)`.
///
/// # Description of the different stages
///
/// Initially, an `Object` is `Constructing`, which is when we are still in the
/// constructor of the object.
///
/// At the end of the constructor, the stage becomes `Constructed`. This change
/// of stage is performed by the function `createObject<T>`, which is called
/// under the hood by any creation of new `Object`.
///
/// When its `destroyObject_()` method is called (or equivalently, if it is a
/// root object, when its refCount reaches zero), then the object is
/// immediately marked as `AboutToBeDestroyed`, and the signal
/// `aboutToBeDestroyed()` is emitted.
///
/// Then, its child objects are recursively destroyed, each of them going
/// through all the stages of destruction.
///
/// Once all its child objects are `Destroyed`, the object is
/// marked as `ChildrenDestroyed`.
///
/// Then, all incoming signals are disconnected.
///
/// Then, the `onDestroyed()` virtual method is called.
///
/// Finally, all outgoing signals are disconnected, and the object is marked as
/// `Destroyed`.
///
/// # Signals and slots
///
/// Note that up to the `ChildrenDestroyed` stage, all the signal-slot
/// connections of an object are kept intact. In particular, this means that
/// during its own destruction, an object can still receive the
/// `aboutToBeDestroyed()` signals from its children.
///
/// Therefore, if you implement an object (A) that listens to the
/// `aboutToBeDestroyed()` signal of one of its children (B), make sure
/// to take into account the fact that the signal can be received while
/// (A) is already partially destructed.
///
enum class ObjectStage : UInt8 {
    Constructing = 0,
    Constructed,
    AboutToBeDestroyed,
    ChildrenDestroyed,
    Destroyed
};

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
    // Using `void*` instead of `const Object*` makes it possible to call the
    // destructor of an ObjPtr with just a forward declaration. This is useful
    // for classes holding an ObjPtr<T> as data member with just a forward
    // declaration of T: they would otherwise have to define a destructor in
    // the .cpp file, after including the full definition of T. This design is
    // still relatively safe since we do have a `const Object* obj` signature
    // for incref(), which verifies that T derives from Object whenever we
    // construct an ObjPtr<T>.
    //
    static void decref(const void* obj, Int64 k = 1);

    // Creates an object.
    //
    template<typename T, typename... Args>
    static ObjPtr<T> createObject(Args&&... args) {
        typename T::CreateKey key = {};
        T* obj = new T(key, std::forward<Args>(args)...); // refCount = 1
        obj->refCount_ -= 1;                              // decref without destroying
        obj->stage_ = ObjectStage::Constructed;           // set as Constructed
        return ObjPtr<T>(obj);                            // re-incref
    }
};

} // namespace detail

/// \class ObjPtr<T>
/// \brief Smart pointer for managing the lifetime of Object instances.
///
/// See documentation of `Object` for more details.
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
    /// Creates a null `ObjPtr<T>`, that is, an `ObjPtr<T>` which doesn't manage
    /// any object.
    ///
    ObjPtr() noexcept
        : obj_(nullptr) {

        // No-op, but verifies at compile time that T derives from Object.
        detail::ObjPtrAccess::incref(obj_);
    }

    /// Creates an `ObjPtr<T>` managing the given object.
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

    /// Creates a copy of the given `ObjPtr<T>`.
    ///
    ObjPtr(const ObjPtr& other) noexcept
        : obj_(other.obj_) {

        detail::ObjPtrAccess::incref(obj_);
    }

    /// Creates a copy of the given `ObjPtr<Y>`. This template overload doesn't
    /// participate in overload resolution if `Y*` is not implicitly convertible
    /// to `T*`.
    ///
    template<typename Y, VGC_REQUIRES(isCompatible_<Y>)>
    ObjPtr(const ObjPtr<Y>& other) noexcept
        : obj_(other.obj_) {

        detail::ObjPtrAccess::incref(obj_);
    }

    /// Assigns the given `T*` to this `ObjPtr<T>`.
    ///
    ObjPtr& operator=(T* obj) noexcept {
        if (obj_ != obj) {
            detail::ObjPtrAccess::decref(obj_);
            obj_ = obj;
            detail::ObjPtrAccess::incref(obj_);
        }
        return *this;
    }

    /// Assigns the given `ObjPtr<T>` to this `ObjPtr<T>`.
    ///
    ObjPtr& operator=(const ObjPtr& other) noexcept {
        if (obj_ != other.obj_) {
            detail::ObjPtrAccess::decref(obj_);
            obj_ = other.obj_;
            detail::ObjPtrAccess::incref(obj_);
        }
        return *this;
    }

    /// Assigns the given `ObjPtr<Y>` to this `ObjPtr<T>`. This template overload
    /// doesn't participate in overload resolution if `Y*` is not implicitly
    /// convertible to `T*`.
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

    /// Moves the given `ObjPtr<T>` to a new `ObjPtr<T>`.
    ///
    ObjPtr(ObjPtr&& other) noexcept
        : obj_(other.obj_) {

        other.obj_ = nullptr;
    }

    /// Moves the given `ObjPtr<Y>` to a new `ObjPtr<T>`. This template overload
    /// doesn't participate in overload resolution if `Y*` is not implicitly
    /// convertible to `T*`.
    ///
    template<typename Y, VGC_REQUIRES(isCompatible_<Y>)>
    ObjPtr(ObjPtr<Y>&& other) noexcept
        : obj_(other.obj_) {

        other.obj_ = nullptr;
    }

    /// Moves the given `ObjPtr<T>` to this `ObjPtr<T>`.
    ///
    ObjPtr& operator=(ObjPtr&& other) noexcept {
        if (*this != other) {
            detail::ObjPtrAccess::decref(obj_);
            obj_ = other.obj_;
            other.obj_ = nullptr;
        }
        return *this;
    }

    /// Moves the given `ObjPtr<Y>` to this `ObjPtr<T>`. This template overload
    /// doesn't participate in overload resolution if `Y*` is not implicitly
    /// convertible to `T*`.
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

    /// Destroys this `ObjPtr<T>`, destroying the managed object if its reference
    /// count reaches zero.
    ///
    ~ObjPtr() {
        detail::ObjPtrAccess::decref(obj_);
    }

    /// Accesses a member of the object managed by this `ObjPtr<T>`. Throws
    /// `NotAliveError` if this `ObjPtr` is null or references a not-alive object.
    ///
    T* operator->() const {
        return getAlive_();
    }

    /// Returns a reference to the object managed by this `ObjPtr<T>`. Throws
    /// `NotAliveError` if this `ObjPtr` is null or references a not-alive object.
    ///
    T& operator*() const {
        return *getAlive_();
    }

    /// Returns a pointer to the object managed by this `ObjPtr<T>` if it is alive.
    /// Otherwise returns a null pointer.
    ///
    T* getIfAlive() const noexcept {
        return isAlive() ? obj_ : nullptr;
    }

    /// Returns a pointer to the object managed by this `ObjPtr<T>`. This method
    /// doesn't throw, buy may return a null pointer or a not-alive object.
    ///
    T* get() const noexcept {
        return obj_;
    }

    /// Returns whether the object managed by this `ObjPtr<T>` is a non-null and
    /// alive object. This method doesn't throw.
    ///
    bool isAlive() const noexcept {
        return obj_ && obj_->isAlive();
    }

    /// Returns whether the object managed by this `ObjPtr<T>` is a non-null and
    /// alive object. This is equivalent to `isAlive()`, and is provided for
    /// convenience for use in boolean expression. This method doesn't throw.
    ///
    operator bool() const noexcept {
        return obj_ && obj_->isAlive();
    }

    /// Returns the reference count of the object managed by this `ObjPtr<T>`. Returns
    /// `-1` if this `ObjPtr<T>` is null. This method doesn't throw.
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

/// Returns whether the two given `ObjPtr` manage the same object.
///
template<typename T, typename U>
inline bool operator==(const ObjPtr<T>& a, const ObjPtr<U>& b) noexcept {
    return a.get() == b.get();
}

/// Returns whether the two given `ObjPtr` manage a different object.
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
/// of any `Object` subclass.
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
                                                                                         \
    /*static_assert(std::is_base_of_v<SuperClass, ThisClass>,             */             \
    /*    "ThisClass is expected to inherit from SuperClass.");           */             \
    static_assert(                                                                       \
        ::vgc::core::isObject<SuperClass>,                                               \
        "Superclass must inherit from Object and use VGC_OBJECT(...).");                 \
                                                                                         \
    static ::vgc::core::StringId staticClassName() {                                     \
        static ::vgc::core::StringId res(#T);                                            \
        return res;                                                                      \
    }                                                                                    \
                                                                                         \
    ::vgc::core::StringId className() const override {                                   \
        return ThisClass::staticClassName();                                             \
    }                                                                                    \
                                                                                         \
protected:                                                                               \
    ~T() = default;                                                                      \
                                                                                         \
private:                                                                                 \
    friend ::vgc::core::detail::ObjPtrAccess;

/// This macro ensures that unsafe base protected methods of `Object` are not
/// accessible in subclasses. This macro should typically be added to a private
/// section of direct sublasses of `Object`, but not to indirect subclasses.
///
#define VGC_PRIVATIZE_OBJECT_TREE_MUTATORS                                               \
private:                                                                                 \
    using Object::destroyObject_;                                                        \
    using Object::destroyAllChildObjects_;                                               \
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

/// Creates an object.
///
template<typename T, typename... Args>
ObjPtr<T> createObject(Args&&... args) {
    static_assert(
        isObject<T>,
        "Cannot call createObject<T>(...) because T does not inherit from "
        "vgc::core::Object.");
    return detail::ObjPtrAccess::createObject<T>(std::forward<Args>(args)...);
}

/// \class vgc::core::Object
/// \brief Provides a common API for object-based tree hierarchies.
///
/// `Object` is the base class of some of the most important classes in the VGC
/// codebase, such as `ui::Widget` and `dom::Node`. The Object class helps
/// implementing tree hierarchies where the destruction of an `Object`
/// automatically causes the destruction of its children.
///
/// Defining an `Object` subclass
/// -----------------------------
///
/// `Object` subclasses should typically be declared as follows:
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
/// These `VGC_DECLARE_OBJECT` and `VGC_OBJECT` macros may seem strange, so
/// let's provide some explanations below as to what they do.
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
/// The ownership of root objects is shared among all `ObjPtr` referencing them,
/// but child objects are uniquely owned by their parent.
///
/// This means that if a root object is destroyed, then all its children are
/// also destroyed, even if there was some `ObjPtr` referencing these children.
/// This design make it possible to create data structures with strong
/// hierarchical invariants. For example, some objects can have a guarantee
/// that, if alive, they always have a parent.
///
/// An `ObjPtr` to a child object behaves like a weak pointer: it doesn't keep
/// the child alive, but it keeps some of its memory allocated in order to be
/// able to check whether it is still alive. When trying to dereference an
/// `ObjPtr` whose referenced object has already been destroyed, a
/// `NotAliveError` is raised.
///
/// In Python, all variables bound to VGC objects are using `ObjPtr` under the
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
/// If a function needs an instance of `Object` as argument, it is recommended
/// to pass this object by raw pointer (unless they participate in ownership,
/// which is rare). Unless stated otherwise, any function that takes an
/// `Object*` as argument assumes that the following is true:
///
/// 1. The pointer is non-null.
///
/// 2. The lifetime of the object exceeds the lifetime of the function.
///
/// The rationale behind the first convention is for performance: it avoids
/// having to check in every function in the call stack whether its parameter
/// is null, and having to handle this case. A common alternative is to use
/// references instead of pointers for this use case, but always using pointers
/// for all `Object` instances makes the code more homogeneous and reinforce
/// the idea that an object is always allocated on the heap and has an
/// "identity".
///
/// The rationale for the second convention (as well as the rationale for using
/// raw non-owning pointers as parameters) is well explained starting at 14:48
/// in the following Herb Sutter talk:
///
/// https://youtu.be/xnqTKD8uD64?t=888
///
/// If an object is passed to a function by `ObjPtr`, this typically indicates
/// that the function takes ownership of the object. In this case, pass the
/// shared pointer directly by value, like so:
///
/// ```cpp
/// myFunction(FooPtr foo);
/// ```cpp
///
/// Miscellaneous Notes
/// -------------------
///
/// Since the destructor of `Object` is virtual, there is no need to explicitly
/// declare a (virtual) destructor in derived classes. In most cases, the
/// default destructor is the most appropriate.
///
/// http://en.cppreference.com/w/cpp/language/rule_of_three
/// https://blog.rmf.io/cxx11/rule-of-zero
/// http://scottmeyers.blogspot.fr/2014/03/a-concern-about-rule-of-zero.html
///
/// Note that the copy constructor, move constructor, assignement operator, and
/// move assignement operators of `Object` are all declared private. This means
/// that objects cannot be copied. If you desire to introduce copy-semantics to
/// a specific subclass of Object, you must manually define a `clone()` method.
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
    static StringId staticClassName() {
        static StringId res("Object");
        return res;
    }

    virtual StringId className() const {
        return ThisClass::staticClassName();
    }

    /// Returns how many `ObjPtr` are currently referencing this `Object`.
    ///
    /// If this object is a root object, then this count represents a "strong
    /// reference count". The root object is kept alive as long as its count
    /// is greater than zero, and the root object is automatically destructed
    /// when its count becomes zero.
    ///
    /// If this object is a child object, then this count represents a "weak
    /// reference count". The child object is uniquely owned by its parent:
    /// this means it can be "destroyed" (i.e., isAlive() becomes false) even
    /// if its count is greater than zero. However, the C++ object is not
    /// actually destructed until the count becomes zero, so that observers
    /// holding an `ObjPtr` to the child object can safely check the value of
    /// `isAlive()`.
    ///
    /// One way to think about this ownership model is:
    /// - An `ObjPtr` is acting like a shared pointer for root objects.
    /// - An `ObjPtr` is acting like a weak pointer for child Objects.
    /// - A parent object is acting like a unique pointer for its child objectss.
    ///
    /// This may seem a little complicated, but is quite simple to use in
    /// practice:
    ///
    /// - In C++, just use raw pointers everywhere, except for root objects
    /// where you should use an `ObjPtr`. You must avoid cyclic references of
    /// `ObjPtr` of root objects, otherwise it will cause memory leaks.
    ///
    /// - In Python, all objects are under the hood wrapped in an `ObjPtr`. This
    /// is more likely to cause cyclic references of root objects, but it's
    /// okay since the garbage collector of Python will detect those and
    /// prevent memory leaks.
    ///
    Int64 refCount() const {
        return refCount_;
    }

    /// Returns the current `ObjectStage` of this object.
    ///
    /// \sa `hasReachedStage()`, `isAlive()`, `isDestroyed()`.
    ///
    ObjectStage stage() const {
        return stage_;
    }

    /// Returns whether the current `ObjectStage` of this object
    /// at least the given stage.
    ///
    /// \sa `stage()`, `isAlive()`, `isDestroyed()`.
    ///
    bool hasReachedStage(ObjectStage stage) const {
        return stage_ >= stage;
    }

    /// Returns whether this `Object` is alive. An `Object` becomes not-alive
    /// just before the `onDestroyed()` method is called. During the
    /// `onDestroyed()` call both `isAlive()` and `isDestroyed()` return false.
    ///
    /// This is equivalent to `!hasReachedStage(ObjectStage::ChildrenDestroyed)`.
    ///
    /// \sa `stage()`, `hasReachedStaged()`, `isDestroyed()`.
    ///
    bool isAlive() const {
        return !hasReachedStage(ObjectStage::ChildrenDestroyed);
    }

    /// Returns whether this `Object` is destroyed. An object becomes
    /// destroyed just after its `onDestroyed()` method is called. During the
    /// `onDestroyed()` call both `isAlive()` and `isDestroyed()` return false.
    ///
    /// This is equivalent to `hasReachedStage(ObjectStage::Destroyed)`.
    ///
    /// \sa `stage()`, `hasReachedStaged()`, `isAlive()`.
    ///
    bool isDestroyed() const {
        return hasReachedStage(ObjectStage::Destroyed);
    }

    /// Returns the parent of this `Object`, or nullptr if this object has no
    /// parent. An object without parent is called a "root object". An object
    /// with a parent is called a "child object".
    ///
    Object* parentObject() const {
        return parentObject_;
    }

    /// Returns the first child of this `Object`, or `nullptr` if this
    /// object has no children.
    ///
    Object* firstChildObject() const {
        return firstChildObject_;
    }

    /// Returns the last child of this `Object`, or `nullptr` if this object has
    /// no children.
    ///
    Object* lastChildObject() const {
        return lastChildObject_;
    }

    /// Returns the next sibling of this `Object`, or `nullptr` if this object has
    /// no parent or is the last child of its parent.
    ///
    Object* nextSiblingObject() const {
        return nextSiblingObject_;
    }

    /// Returns the previous sibling of this `Object`, or `nullptr` if this object has
    /// no parent or is the first child of its parent.
    ///
    Object* previousSiblingObject() const {
        return previousSiblingObject_;
    }

    /// Returns a range that iterates over all child objects of this `Object`.
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

    /// Returns the number of child objects of this `Object`.
    ///
    Int numChildObjects() const;

    /// Returns whether this `Object` is a descendant of the given `other`
    /// object. Returns true in the special case where `this == other`. Returns
    /// false if `other` is `nullptr`.
    ///
    bool isDescendantObjectOf(const Object* other) const;

    /// Prints this object tree to the standard output, typically for debugging
    /// purposes.
    ///
    void dumpObjectTree() const;

    /// Returns the source of the current nested-most signal `emit()` call.
    ///
    static Object* emitter() {
        return detail::currentEmitter();
    }

    /// Removes the signal-slot connection corresponding to the given `handle`.
    ///
    /// Returns `true` and invalidates `handle` if the connection is successfully
    /// removed.
    /// Returns `false` if the connection wasn't an existing connection of this
    /// `Object`, and therefore couldn't be removed.
    ///
    bool disconnect(ConnectionHandle& handle) const {
        if (detail::SignalHub::disconnect(this, handle)) {
            handle.invalidate();
            return true;
        }
        return false;
    }

    /// Removes all the signal-slot connections between this `Object` and
    /// the given `receiver`.
    ///
    /// Returns `true` if at least one connection was removed.
    ///
    bool disconnect(const Object* receiver) const {
        return detail::SignalHub::disconnect(this, receiver);
    }

    /// Disconnects all signals from this `Object` to any slots.
    ///
    void disconnect() const {
        detail::SignalHub::disconnectSignals(this);
    }

    /// Returns the number of outbound signal-slot connections.
    ///
    Int numConnections() const {
        return detail::SignalHub::numOutboundConnections(this);
    }

    /// This signal is emitted by this `Object` just before it is destroyed.
    /// The object is still alive, its children have not yet been recursively
    /// destroyed, and none of the signals and slots have been automatically
    /// disconnected.
    ///
    VGC_SIGNAL(aboutToBeDestroyed, (Object*, object));

protected:
    /// This callback method is invoked when this object has just been
    /// destroyed, that is, just after `isAlive()` has switched from `true` to
    /// `false`. However, note that its C++ destructor has not been called yet:
    /// the destructor will be called when `refCount()` reaches zero, and for
    /// now `refCount()` is still at least one.
    ///
    /// Implementing this callback is useful when you can release expensive
    /// resources early. For example, it is often a good idea to release
    /// allocated memory here: there is no need to keep it around until the
    /// destructor is called. The destructor will be called when all
    /// `ObjectPtr` observers (for example, a python variable pointing to this
    /// object) are themselves destroyed.
    ///
    /// Note that when this method is called, all the children of this `Object`
    /// have already been destroyed, and if this `Object` previsouly had a parent,
    /// then it has already been removed from this parent.
    ///
    /// If you override this method, do not forget to call `onDestroyed()` of the
    /// base class, preferrably at the end.
    ///
    virtual void onDestroyed();

    /// This callback method is invoked whenever a child has been added to this
    /// `Object`. When this method is called, the child has already been added
    /// to its parent, that is, `child->parent() == this`
    ///
    /// `wasOnlyReordered` is true if it was already a child but got reordered.
    ///
    virtual void onChildAdded(Object* child, bool wasOnlyReordered);

    /// This callback method is invoked whenever a child has been removed from
    /// this `Object`. When this function is called, the child has already been removed
    /// from its parent, that is, `child->parent() == nullptr`.
    ///
    virtual void onChildRemoved(Object* child);

protected:
    /// Enforces that objects are always created via `createObject<T>()`.
    ///
    class CreateKey {
    private:
        friend detail::ObjPtrAccess;
        CreateKey() = default;
    };

    /// Constructs an `Object`.
    ///
    Object(CreateKey key);

    /// Calls the destructor of this `Object`. Note that you should never call
    /// this method or `delete` directly. Under normal usage, the objects
    /// should be automatically destroyed for you: child objects are
    /// automatically destroyed by their parent, and root objects are
    /// automatically destroyed by the smart pointers managing them.
    ///
    /// Note that some `Object` subclasses may choose to expose a safe
    /// `destroy()` method in their public API, but it is optional.
    ///
    /// \sa `destroyObject()`.
    ///
    virtual ~Object();

    /// Destroys this `Object`.
    ///
    /// This is a low-level method which should only be used by direct
    /// subclasses as a helper method to implement their API.
    ///
    /// Use `VGC_PRIVATIZE_OBJECT_TREE_MUTATORS` to privatize this function
    /// in your subclass.
    ///
    /// This design allows subclasses to decide whether they wish to expose a
    /// `destroy()` method or not. For example, a `Foo` subclass may, or may not,
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
    /// 1. Recursively destroy the children of this `Object`.
    ///
    /// 2. Call the `onDestroyed()` callback method. At this stage, `isAlive()` is
    ///    still true, and `parentObject()` is still accessible.
    ///
    /// 3. Remove from parent. Just after removal, a side effect might be to
    ///    destroy the root of the parent, in case its `refCount()` becomes zero.
    ///
    /// 4. Set `isAlive()` to false.
    ///
    /// 5. Deallocate memory, in case the `refCount()` of this Object is zero.
    ///
    void destroyObject_();

    /// Destroys all children of this `Object`.
    ///
    /// This is a low-level method which should only be used by direct
    /// subclasses as a helper method to implement their API.
    ///
    /// Use `VGC_PRIVATIZE_OBJECT_TREE_MUTATORS` to privatize this function
    /// in your subclass.
    ///
    void destroyAllChildObjects_();

    /// Destroys the given `child` of this `Object`. If `child` is `nullptr` or
    /// isn't a child of this object, then `NotAChildError` is raised. See
    /// `destroyObject_()` for details.
    ///
    /// This is a low-level method which should only be used by direct
    /// subclasses as a helper method to implement their API.
    ///
    /// Use `VGC_PRIVATIZE_OBJECT_TREE_MUTATORS` to privatize this function
    /// in your subclass.
    ///
    void destroyChildObject_(Object* child);

    /// Inserts the given `child` as the last child of this Object. If `child`
    /// is `nullptr`, then `NullError` is raised.
    ///
    /// This is a low-level method which should only be used by direct
    /// subclasses as a helper method to implement their API.
    ///
    /// Use `VGC_PRIVATIZE_OBJECT_TREE_MUTATORS` to privatize this function
    /// in your subclass.
    ///
    /// This design allows subclasses to add additional restrictions on which
    /// types of children are allowed. For example, a `Foo` subclass which only
    /// allows `Foo` children may define:
    ///
    /// ```cpp
    /// void Foo::appendChild(Foo* child)
    /// {
    ///     Object::appendChildObject_(child);
    /// }
    /// ```
    ///
    /// Very importantly, you must NOT use this method to append an object
    /// whose subclass isn't your own or closely related (= friend classes, or
    /// classes developed in the same cpp file or perhaps module). Indeed, each
    /// `Object` subclass defines its own invariants regarding which `Object`
    /// subclasses they allow as children or parent. For example the children
    /// of a `vgc::dom::Node` are always other `vgc::dom::Node` objects, and a
    /// `vgc::dom::Document` is guaranteed to always be a root object.
    /// Therefore, using `appendChildObject_()` in `Foo` to modify a hierachy
    /// of `dom::Node` objects would break these invariants.
    ///
    void appendChildObject_(Object* child);

    /// Inserts the given `child` as the first child of this `Object`. If
    /// `child` is `nullptr`, then `NullError` is raised.
    ///
    /// This is a low-level method which should only be used by direct
    /// subclasses as a helper method to implement their API.
    ///
    /// Use `VGC_PRIVATIZE_OBJECT_TREE_MUTATORS` to privatize this function
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
    /// Very importantly, you must NOT use this method to prepend an object
    /// whose subclass isn't your own or closely related (= friend classes, or
    /// classes developed in the same cpp file or perhaps module). Indeed, each
    /// `Object` subclass defines its own invariants regarding which `Object`
    /// subclasses they allow as children or parent. For example the children
    /// of a `vgc::dom::Node` are always other `vgc::dom::Node` objects, and a
    /// `vgc::dom::Document` is guaranteed to always be a root object.
    /// Therefore, using `prependChildObject_()` in `Foo` to modify a hierachy
    /// of `dom::Node` objects would break these invariants.
    ///
    void prependChildObject_(Object* child);

    /// Inserts the given `child` as a child of this `Object`, just before
    /// `nextSibling`. If `child` is `nullptr`, then `NullError` is raised. If
    /// `nextSibling` is `nullptr`, then it is inserted last. If `nextSibling`
    /// is not `nullptr`, but isn't a child of this object, then
    /// `NotAChildError` is raised.
    ///
    /// This is a low-level method which should only be used by direct
    /// subclasses as a helper method to implement their API.
    ///
    /// Use `VGC_PRIVATIZE_OBJECT_TREE_MUTATORS` to privatize this function
    /// in your subclass.
    ///
    /// This design allows subclasses to add additional restrictions on which
    /// types of children are allowed. For example, a Foo subclass which only
    /// allows `Foo` children may define:
    ///
    /// ```cpp
    /// void Foo::insertChildBefore(Foo* child, Foo* nextSibling)
    /// {
    ///     Object::insertChildObject_(child, nextSibling);
    /// }
    /// ```
    ///
    /// Very importantly, you must NOT use this method to insert an object
    /// whose subclass isn't your own or closely related (= friend classes, or
    /// classes developed in the same cpp file or perhaps module). Indeed, each
    /// `Object` subclass defines its own invariants regarding which `Object`
    /// subclasses they allow as children or parent. For example the children
    /// of a `vgc::dom::Node` are always other `vgc::dom::Node` objects, and a
    /// `vgc::dom::Document` is guaranteed to always be a root object.
    /// Therefore, using `insertChildObject_()` in `Foo` to modify a hierachy
    /// of `dom::Node` objects would break these invariants.
    ///
    void insertChildObject_(Object* nextSibling, Object* child);

    /// Removes the given `child` from the list of children of this `Object`.
    /// Returns an `ObjPtr<Object>` managing `child` which becomes a root
    /// object and therefore not uniquely owned by its parent. If `child` is
    /// `nullptr` or isn't a child of this object, then `NotAChildError` is
    /// raised.
    ///
    /// This is a low-level method which should only be used by direct
    /// subclasses to implement their API.
    ///
    /// Use `VGC_PRIVATIZE_OBJECT_TREE_MUTATORS` to privatize this function
    /// in your subclass.
    ///
    /// This design allows subclasses to add additional restrictions on which
    /// types of children are allowed, and whether removing children is allowed
    /// for this subclass of `Object`. For example, a `Foo` subclass which only
    /// allows `Foo` children may define:
    ///
    /// ```cpp
    /// void Foo::removeChild(Foo* child)
    /// {
    ///     Object::removeChildObject_(child);
    /// }
    /// ```
    ///
    /// Very importantly, you must NOT use this method to remove an object
    /// whose subclass isn't your own or closely related (= friend classes, or
    /// classes developed in the same cpp file or perhaps module). Indeed, each
    /// `Object` subclass defines its own invariants regarding which `Object`
    /// subclasses they allow as children or parent. For example the children
    /// of a `vgc::dom::Node` are always other `vgc::dom::Node` objects, and a
    /// `vgc::dom::Document` is guaranteed to always be a root object.
    /// Therefore, using `removeChildObject_()` in `Foo` to modify a hierachy
    /// of `dom::Node` objects would break these invariants.
    ///
    ObjectPtr removeChildObject_(Object* child);

    /// Inserts this `Object` as the last child of the given `parent` object.
    /// If `parent` is `nullptr`, this function is equivalent to
    /// `removeObjectFromParent_()`.
    ///
    /// This is a low-level method which should only be used by direct
    /// subclasses to implement their API.
    ///
    /// Use `VGC_PRIVATIZE_OBJECT_TREE_MUTATORS` to privatize this function
    /// in your subclass.
    ///
    /// Very importantly, you must NOT use this method to append an object
    /// whose subclass isn't your own or closely related (= friend classes, or
    /// classes developed in the same cpp file or perhaps module). Indeed, each
    /// `Object` subclass defines its own invariants regarding which `Object`
    /// subclasses they allow as children or parent. For example the children
    /// of a `vgc::dom::Node` are always other `vgc::dom::Node` objects, and a
    /// `vgc::dom::Document` is guaranteed to always be a root object.
    /// Therefore, using `appendObjectToParent_()` in `Foo` to modify a hierachy
    /// of `dom::Node` objects would break these invariants.
    ///
    void appendObjectToParent_(Object* parent);

    /// Inserts this `Object` as the first child of the given `parent` object.
    /// If `parent` is `nullptr`, this function is equivalent to
    /// `removeObjectFromParent_()`.
    ///
    /// This is a low-level method which should only be used by direct
    /// subclasses to implement their API.
    ///
    /// Use `VGC_PRIVATIZE_OBJECT_TREE_MUTATORS` to privatize this function
    /// in your subclass.
    ///
    /// Very importantly, you must NOT use this method to prepend an object
    /// whose subclass isn't your own or closely related (= friend classes, or
    /// classes developed in the same cpp file or perhaps module). Indeed, each
    /// `Object` subclass defines its own invariants regarding which `Object`
    /// subclasses they allow as children or parent. For example the children
    /// of a `vgc::dom::Node` are always other `vgc::dom::Node` objects, and a
    /// `vgc::dom::Document` is guaranteed to always be a root object.
    /// Therefore, using `prependObjectToParent_()` in `Foo` to modify a hierachy
    /// of `dom::Node` objects would break these invariants.
    ///
    void prependObjectToParent_(Object* parent);

    /// Inserts this `Object` as a child of the given `parent` object, just
    /// before `nextSibling`. If `nextSibling` is `nullptr`, then it is
    /// inserted last. If `nextSibling` is not `nullptr` but isn't a child of
    /// the given `parent`, then `NotAChildError` is raised. If `parent` is
    /// `nullptr`, this function is equivalent to `removeObjectFromParent_()`.
    ///
    /// This is a low-level method which should only be used by direct
    /// subclasses to implement their API.
    ///
    /// Use `VGC_PRIVATIZE_OBJECT_TREE_MUTATORS` to privatize this function
    /// in your subclass.
    ///
    /// Very importantly, you must NOT use this method to insert an object
    /// whose subclass isn't your own or closely related (= friend classes, or
    /// classes developed in the same cpp file or perhaps module). Indeed, each
    /// `Object` subclass defines its own invariants regarding which `Object`
    /// subclasses they allow as children or parent. For example the children
    /// of a `vgc::dom::Node` are always other `vgc::dom::Node` objects, and a
    /// `vgc::dom::Document` is guaranteed to always be a root object.
    /// Therefore, using `insertObjectToParent_()` in `Foo` to modify a hierachy
    /// of `dom::Node` objects would break these invariants.
    ///
    void insertObjectToParent_(Object* parent, Object* nextSibling);

    /// Removes this `Object` from the list of children of its parent. Returns
    /// an `ObjPtr<Object>` managing this object which becomes a root object
    /// and therefore not uniquely owned by its parent. Nothing happens if this
    /// object is already a root object.
    ///
    /// This is a low-level method which should only be used by direct
    /// subclasses to implement their API.
    ///
    /// Use `VGC_PRIVATIZE_OBJECT_TREE_MUTATORS` to privatize this function
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
    // The refCount_ is initially set to 1 so that creating a temporary
    // `ObjPtr` to an object (typically, to keep it alive) doesn't
    // inadvertantly destroy it when its stage is `Constructing`. The object
    // can be thought as of being owned by createObject<T>() during
    // construction.
    //
    friend detail::ObjPtrAccess;
    mutable Int64 refCount_ = 1;
    ObjectStage stage_ = ObjectStage::Constructing;

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

    void onChildAdded_(Object* child, bool wasOnlyReordered);
    void onChildRemoved_(Object* child);
};

/// \class vgc::core::ObjRawPtr<T>
/// \brief A non-smart pointer holding an `Object*` without ownership.
///
/// This template class is useful whenever you'd like to pass a raw pointer
/// `Object*` (or a subclass) to a template function, but said template
/// function disallows the use of raw pointers (e.g., `format()`).
///
/// By wrapping the raw pointer inside an `ObjRawPtr<T>`, it becomes an
/// instance of a separate non-pointer type for which template specializations
/// can be defined.
///
/// \sa `ptr(const Object*)`.
///
template<typename T>
class ObjRawPtr {
private:
    template<typename Y>
    static constexpr bool isCompatible_ = std::is_convertible_v<Y*, T*>;

public:
    /// Create an `ObjRawPtr<T>` storing the given object pointer.
    ///
    ObjRawPtr(T* obj)
        : obj_(obj) {
    }

    /// Creates a copy of the given `ObjRawPtr<Y>` as a `ObjRawPtr<T>`. This
    /// template overload doesn't participate in overload resolution if `Y*` is
    /// not implicitly convertible to `T*`.
    ///
    template<typename Y, VGC_REQUIRES(isCompatible_<Y>)>
    ObjRawPtr(const ObjPtr<Y>& other) noexcept
        : obj_(other.obj_) {
    }

    /// Returns the stored object pointer.
    ///
    T* get() const {
        return obj_;
    }

    /// Accesses a member of the stored object pointer. This is undefined
    /// behavior if the stored object pointer is `nullptr` or dangling.
    ///
    T* operator->() const {
        return obj_;
    }

    /// Returns a reference to the stored object pointer. This is undefined
    /// behavior if the stored object pointer is `nullptr` or dangling.
    ///
    T& operator*() const {
        return *obj_;
    }

private:
    T* obj_;
};

/// Returns whether the two given `ObjRawPtr` point the same object.
///
template<typename T, typename U>
inline bool operator==(const ObjRawPtr<T>& a, const ObjRawPtr<U>& b) noexcept {
    return a.get() == b.get();
}

/// Returns whether the two given `ObjRawPtr` point to different objects.
///
template<typename T, typename U>
inline bool operator!=(const ObjRawPtr<T>& a, const ObjRawPtr<U>& b) noexcept {
    return a.get() != b.get();
}

/// Performs a static cast between `ObjRawPtr` types.
///
template<typename T, typename U>
ObjRawPtr<T> static_pointer_cast(const ObjRawPtr<U>& r) noexcept {
    return ObjRawPtr<T>(static_cast<T*>(r.get()));
}

/// Performs a dynamic cast between `ObjRawPtr` types.
///
template<typename T, typename U>
ObjRawPtr<T> dynamic_pointer_cast(const ObjRawPtr<U>& r) noexcept {
    return ObjRawPtr<T>(dynamic_cast<T*>(r.get()));
}

/// Performs a const cast between `ObjRawPtr` types.
///
template<typename T, typename U>
ObjRawPtr<T> const_pointer_cast(const ObjRawPtr<U>& r) noexcept {
    return ObjRawPtr<T>(const_cast<T*>(r.get()));
}

using ObjectRawPtr = ObjRawPtr<Object>;
using ObjectConstRawPtr = ObjRawPtr<const Object>;

/// Wraps the given `const Object*` in an `ObjectConstRawPtr`.
///
/// This function is useful for string formatting, since `fmt::formatter`
/// cannot be specialized for actual raw pointers, so as a workaround we
/// specialized it for `ObjectConstRawPtr` instead.
///
/// ```cpp
/// Object* obj = getSomeObject();
/// Object* parent = obj ? obj->parentObject() : nullptr;
/// print("The parent of {} is {}", ptr(obj), ptr(parent));
/// ```
///
/// Possible output:
///
/// ```
/// "The parent of <Button @ 0x7fca717ed080> is <Null Object>"
/// ```
///
/// \sa `ptr(const ObjPtr<T>&)`.
///
VGC_CORE_API
inline ObjectConstRawPtr ptr(const Object* obj) {
    return ObjectConstRawPtr(obj);
}

/// \overload
template<typename T>
ObjectConstRawPtr ptr(const ObjPtr<T>& objPtr) {
    return ptr(objPtr.get());
}

} // namespace vgc::core

template<>
struct fmt::formatter<vgc::core::ObjectConstRawPtr> : fmt::formatter<std::string_view> {
    // TODO: add formatting options, e.g., "{:a}" to only print the address
    template<typename FormatContext>
    auto format(vgc::core::ObjectConstRawPtr objPtr, FormatContext& ctx) {
        std::string res = "<Null Object>";
        const vgc::core::Object* obj = objPtr.get();
        if (obj) {
            res = fmt::format("<{} @ {}>", obj->className(), fmt::ptr(obj));
        }
        return fmt::formatter<std::string_view>::format(res, ctx);
    }
};

namespace vgc::core {

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

inline void ObjPtrAccess::decref(const void* obj_, Int64 k) {
    const Object* obj = static_cast<const Object*>(obj_);
    if (obj) {
        obj->refCount_ -= k;
        bool isRoot = (obj->parentObject_ == nullptr);
        if (isRoot && obj->refCount_ == 0) {
            if (obj->isAlive()) {
                // See implementation of destroyObjectImpl_() for details on the
                // const-cast and other subtleties of the following two lines.
                Object* root_ = const_cast<Object*>(obj);
                root_->destroyObjectImpl_();
            }
            else {
                delete obj;
            }
        }
    }
}

} // namespace detail

/// \class vgc::core::ObjListIterator
/// \brief Iterates over an `ObjList`
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
/// \brief Stores a list of child objects of a given type `T`.
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
    ObjList(CreateKey key)
        : Object(key) {
    }

public:
    static ObjList* create(Object* parent) {
        ObjPtr<ObjList> list = createObject<ObjList>();
        list->appendObjectToParent_(parent);
        return list.get();
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

    void insert(T* nextSibling, T* child) {
        insertChildObject_(nextSibling, child);
    }

    void insert(Int i, T* child) {
        if (i < 0 || i > numChildObjects()) {
            throw IndexError(format("Cannot insert child in list at index {}.", i));
        }
        Object* nextSibling = firstChildObject();
        for (; i > 0; --i) {
            nextSibling = nextSibling->nextSiblingObject();
        }
        insertChildObject_(nextSibling, child);
    }

    ObjPtr<T> remove(T* child) {
        ObjPtr<Object> removed = removeChildObject_(child);
        ObjPtr<T> res(static_cast<T*>(removed.get()));
        return res;

        // Note: if we were using std::shared_ptr, then we would have to use
        // std::static_pointer_cast instead, otherwise the reference counting
        // block wouldn't be shared. But in our case, the above static_cast
        // works since our core::ObjPtr use intrusive reference counting.
    }

    VGC_SIGNAL(childAdded, (T*, child), (bool, wasOnlyReordered));
    VGC_SIGNAL(childRemoved, (T*, child));

protected:
    void onChildAdded(Object* child, bool wasOnlyReordered) override {
        childAdded().emit(static_cast<T*>(child), wasOnlyReordered);
    }

    void onChildRemoved(Object* child) override {
        childRemoved().emit(static_cast<T*>(child));
    }
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
    using T##Ptr = ::vgc::core::ObjPtr<T>;                                               \
    using T##ConstPtr = ::vgc::core::ObjPtr<const T>;                                    \
    using T##List = ::vgc::core::ObjList<T>;                                             \
    using T##ListIterator = ::vgc::core::ObjListIterator<T>;                             \
    using T##ListView = ::vgc::core::ObjListView<T>;

namespace vgc::core {

VGC_DECLARE_OBJECT(Object);

} // namespace vgc::core

namespace vgc::core::detail {

VGC_DECLARE_OBJECT(ConstructibleTestObject);

// XXX add a create() in Object directly?
class VGC_CORE_API ConstructibleTestObject : public Object {
    VGC_OBJECT(ConstructibleTestObject, Object)

protected:
    ConstructibleTestObject(CreateKey key)
        : Object(key) {
    }

public:
    static ConstructibleTestObjectPtr create();
};

VGC_DECLARE_OBJECT(SignalTestObject);

class VGC_CORE_API SignalTestObject : public Object {
    VGC_OBJECT(SignalTestObject, Object)

protected:
    SignalTestObject(CreateKey key)
        : Object(key) {
    }

public:
    static SignalTestObjectPtr create() {
        return createObject<SignalTestObject>();
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
