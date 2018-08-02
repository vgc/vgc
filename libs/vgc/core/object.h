// Copyright 2018 The VGC Developers
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

#include <memory>
#include <vgc/core/api.h>

#define VGC_CORE_DECLARE_PTRS(T)                        \
    class T;                                            \
    using T##SharedPtr      = std::shared_ptr<T>;       \
    using T##WeakPtr        = std::weak_ptr<T>;         \
    using T##ConstSharedPtr = std::shared_ptr<const T>; \
    using T##ConstWeakPtr   = std::weak_ptr<const T>

#define VGC_CORE_OBJECT_SHARED_PTR_(T)                                  \
    /** Returns a shared_ptr to this object. Assumes that the object */ \
    /** is indeed managed via shared_ptr.                            */ \
    std::shared_ptr<T> sharedPtr() {                                    \
        return std::static_pointer_cast<T>(this->shared_from_this());   \
    }

#define VGC_CORE_OBJECT_WEAK_PTR_(T)                                  \
    /** Returns a weak_ptr to this object. Assumes that the object */ \
    /** is indeed managed via shared_ptr.                          */ \
    std::weak_ptr<T> weakPtr() {                                      \
        return sharedPtr(); /* Note: C++17 has weak_from_this */      \
    }

#define VGC_CORE_OBJECT_CONST_SHARED_PTR_(T)                                  \
    /** Returns a const shared_ptr to this object. Assumes that the object */ \
    /** is indeed managed via shared_ptr.                                  */ \
    std::shared_ptr<const T> sharedPtr() const {                              \
        return std::static_pointer_cast<const T>(this->shared_from_this());   \
    }

#define VGC_CORE_OBJECT_CONST_WEAK_PTR_(T)                                  \
    /** Returns a const weak_ptr to this object. Assumes that the object */ \
    /** is indeed managed via shared_ptr.                                */ \
    std::weak_ptr<const T> weakPtr() const {                                \
        return sharedPtr(); /* Note: C++17 has weak_from_this */            \
    }

#define VGC_CORE_OBJECT(T)               \
public:                                  \
    VGC_CORE_OBJECT_SHARED_PTR_(T)       \
    VGC_CORE_OBJECT_WEAK_PTR_(T)         \
    VGC_CORE_OBJECT_CONST_SHARED_PTR_(T) \
    VGC_CORE_OBJECT_CONST_WEAK_PTR_(T)   \
private:

namespace vgc {
namespace core {

VGC_CORE_DECLARE_PTRS(Object);

/// \class vgc::core::Object
/// \brief Base class of most classes in the VGC codebase.
///
/// This base class allows the VGC codebase to be more readable, consistent,
/// maintainable, thread-safe, and Python-binding-friendly at only a very small
/// performance cost. It is strongly recommended that most classes derive,
/// directly or indirectly, from Object. In particular any class that contains
/// a `std::vector`, a `std::string`, any other container, or performs any sort
/// of memory allocation should typically inherit from Object, since the
/// overhead cost is unlikely to be significant compared to the existing cost.
/// Notable exceptions include small struct-like classes such as `Vec2d`: these
/// do not perform any dynamic allocation, are expected to be potentially
/// instanciated more than a million times in a session, and are expected or
/// measured to be a bottleneck if they would derive from Object.
///
/// Classes deriving directly or indirectly from Object should typically be
/// declared as follows, where Bar is either Object or one of its subclasses:
///
/// \code
/// class Foo: public Bar
/// {
///     VGC_CORE_OBJECT(Foo)
///
/// protected:
///     Foo();      // some constructor
///     Foo(int x); // some other constructor
///
/// public:
///     static ObjectSharedPtr create();      // Use make_shared or allocate_shared
///     static ObjectSharedPtr create(int x); // Use make_shared or allocate_shared
/// };
/// \endcode
///
/// The macro VGC_CORE_OBJECT(Foo) defines the following public methods:
///
/// \code
/// FooSharedPtr sharedPtr();
/// FooWeakPtr weakPtr();
/// FooConstSharedPtr sharedPtr() const;
/// FooConstWeakPtr weakPtr() const;
/// \endcode
///
/// These methods can be used to convert a raw pointer into a smart pointer.
/// This is possible since vgc::core::Object inherits from
/// `std::enable_shared_from_this`.
///
/// Ownership Conventions
/// ---------------------
///
/// Despite being managed by shared pointers, objects should typically have a
/// clearly identified unique owner. This unique owner creates the object via
/// Foo::create(), stores the shared pointer as a member variable `FooSharedPtr
/// foo_`, but then always passes the raw pointer foo_.get() to other methods
/// and clients (= "observers").
///
/// Observers should typically access and manipulate observed objects via
/// raw pointers. If they desire to "remember" an object, the recommended
/// way is to simply store the raw pointer as a member variable, and register
/// for signals to be notified in case the object is destructed. If the
/// observed object is guaranteed to outlive the observer via other contracts,
/// just store the raw pointer without registering for signals. At a last
/// resort, if it isn't possible or practical to register for signals, and if
/// you do not have guarantees on the lifetime of the observed object, then you
/// may call obj->weakPtr() and store the weak pointer as a member variable,
/// which allows you to check for expiration at a later point:
///
/// \code
/// class Bar {
///     FooWeakPtr foo_;
///
/// public:
///     void Bar::setFoo(foo* foo) { foo_ = foo->weakPtr(); }
///
///     void Bar::bar() {
///         if (Foo* foo = foo_.lock()) {
///             /* do something */
///         }
///         else {
///             foo_.reset();
///         }
///     }
/// };
/// \endcode
///
/// Under almost no circumstances you should convert a raw pointer to a shared
/// pointer and hold this shared pointer as a member variable (that is, unless
/// you actually desire to share ownership, which is rare). Indeed, holding
/// such shared pointer would defer the destruction of the object, and goes
/// against good RAII design. A notable exception are Python bindings: many
/// Python bindings of C++ classes internally hold a shared_ptr, because the
/// concept of weak reference is not very pythonic. Indeed, most Python
/// developers expect that objects stay alive as long as at least one Python
/// variable references the object.
///
/// Of course, never call `new` or `delete` to manually manage the lifetime of
/// vgc::core::Object instances.
///
/// Calling Conventions
/// -------------------
///
/// Functions manipuling instances of vgc::core::Object should be passed these
/// objects by raw pointers (that is, unless they participate in ownership,
/// which is rare). Unless stated otherwise, any function that takes an Object*
/// as argument assumes that the following is true:
///
/// 1. The pointer is non-null.
///
/// 2. The lifetime of the object exceeds the lifetime of the function.
///
/// 3. Converting to smart pointers is safe.
///
/// Please watch the first 30min of following talk by Herb Sutter for a
/// rationale (or consult the C++ Core Guidelines):
///
/// https://www.youtube.com/watch?v=xnqTKD8uD64
///
/// If an object is passed to a function by shared pointer, this typically
/// indicates that the function takes ownership of the object. In this case,
/// pass the shared pointer directly by value, like so:
///
/// \code
/// myFunction(FooSharedPtr foo);
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
/// In some cases, destructions of objects with deeply nested ownership of
/// other pointers may run into stack overflow issues. In these cases, it may
/// be appropriate to implement a custom destructor in order to derecursify the
/// destructions. See the following example by Herb Sutter in the case
/// `unique_ptr`, but the advice applies equally well to `shared_ptr`:
///
/// https://youtu.be/JfmTagWcqoE?t=12m20s
///
class VGC_CORE_API Object: public std::enable_shared_from_this<Object>
{
    VGC_CORE_OBJECT(Object)
    Object(const Object&) = default;
    Object(Object&&) = default;
    Object& operator=(const Object&) = default;
    Object& operator=(Object&&) = default;

protected:
    /// Constructs an Object.
    ///
    Object();

public:
    /// Destructs the Object. Never call this manually, and instead let the
    /// shared pointers do the work for you.
    ///
    /// This ought to be a protected method to avoid accidental misuse, but it
    /// is currently kept public due to a limitation of pybind11 (see
    /// https://github.com/pybind/pybind11/issues/114), and possibly other
    /// related issues.
    ///
    virtual ~Object();

    /// Creates an object.
    ///
    static ObjectSharedPtr create();
};

} // namespace core
} // namespace vgc

#endif // VGC_CORE_OBJECT_H
