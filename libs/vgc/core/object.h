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

/// \def VGC_CORE_OBJECT_DEBUG
/// If defined, then all constructions, assignments, and destructions of
/// vgc::core::Object instances are printed to the console.
///
#ifdef VGC_CORE_OBJECT_DEBUG
    #include <cstdio>
    #define VGC_CORE_OBJECT_DEFAULT_(s) \
        { printf("Object %p " s "\n", (void*) this); }
#else
    #define VGC_CORE_OBJECT_DEFAULT_(s) = default;
#endif

#define VGC_CORE_DECLARE_PTRS(T)                        \
    class T;                                            \
    using T##SharedPtr      = std::shared_ptr<T>;       \
    using T##WeakPtr        = std::weak_ptr<T>;         \
    using T##ConstSharedPtr = std::shared_ptr<const T>; \
    using T##ConstWeakPtr   = std::weak_ptr<const T>

#define VGC_CORE_OBJECT_CREATE_(T)                                \
    /** Constructs an object of type T managed by a shared_ptr */ \
    template <typename... Args>                                   \
    static std::shared_ptr<T> create(Args... args) {              \
        return std::make_shared<T>(args...);                      \
    }

#define VGC_CORE_OBJECT_SHARED_PTR_(T)                                   \
    /** Returns a shared_ptr managing this object. Assumes the object */ \
    /** was created via the create() static method.                   */ \
    std::shared_ptr<T> sharedPtr() {                                     \
        return std::static_pointer_cast<T>(this->shared_from_this());    \
    }

#define VGC_CORE_OBJECT_WEAK_PTR_(T)                             \
    /** Returns a weak_ptr to this object. Assumes the object */ \
    /** was created via the create() static method.           */ \
    std::weak_ptr<T> weakPtr() {                                 \
        return sharedPtr(); /* Note: C++17 has weak_from_this */ \
    }

#define VGC_CORE_OBJECT_CONST_SHARED_PTR_(T)                                \
    /** Returns a shared_ptr managing this object. Assumes the object */    \
    /** was created via the create() static method.                   */    \
    std::shared_ptr<const T> sharedPtr() const {                            \
        return std::static_pointer_cast<const T>(this->shared_from_this()); \
    }

#define VGC_CORE_OBJECT_CONST_WEAK_PTR_(T)                       \
    /** Returns a weak_ptr to this object. Assumes the object */ \
    /** was created via the create() static method.           */ \
    std::weak_ptr<const T> weakPtr() const {                     \
        return sharedPtr(); /* Note: C++17 has weak_from_this */ \
    }

#define VGC_CORE_OBJECT(T)               \
    VGC_CORE_OBJECT_CREATE_(T)           \
    VGC_CORE_OBJECT_SHARED_PTR_(T)       \
    VGC_CORE_OBJECT_WEAK_PTR_(T)         \
    VGC_CORE_OBJECT_CONST_SHARED_PTR_(T) \
    VGC_CORE_OBJECT_CONST_WEAK_PTR_(T)

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
/// Classes deriving from Object should be declared as follows:
///
/// \code
/// class Foo: public vgc::core::Object
/// {
/// public:
///     VGC_CORE_OBJECT(Foo)
///
///     Foo();      // some public constructor
///     Foo(int x); // some other public constructor
/// //...
/// };
/// \endcode
///
/// The macro VGC_CORE_OBJECT(Foo) defines the following member methods:
///
/// \code
/// template <typename... Args> static FooSharedPtr create(Args... args);
/// FooSharedPtr sharedPtr();
/// FooWeakPtr weakPtr();
/// FooConstSharedPtr sharedPtr() const;
/// FooConstWeakPtr weakPtr() const;
/// \endcode
///
/// The first method should be used to construct your objects, for example:
///
/// \code
/// FooSharedPtr foo1 = Foo::create();
/// FooSharedPtr foo2 = Foo::create(42);
/// \endcode
///
/// This is equivalent to `std::make_shared<Foo>(...)`, and ensures that the
/// object is dynamically allocated and managed by a `std::shared_ptr`.
///
/// The other two methods sharedPtr() and weakPtr() can be used to convert a
/// raw pointer of the object to a `std::shared_ptr` and `std::weak_ptr`,
/// respectively. This is possible because vgc::core::Object inherits from
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
/// against good RAII design. And of course, never call `new` or `delete` to
/// manage the lifetime of vgc::core::Object instances.
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
/// 3. Converting to a weak pointer via obj->weakPtr() does not throw.
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
/// default destructor is the most appropriate. In general, derived classes
/// should strive for "the rule of zero": no custom destructor, copy
/// constructor, move constructor, assignment operator, and move assignment
/// operator. See:
///
/// http://en.cppreference.com/w/cpp/language/rule_of_three
/// https://blog.rmf.io/cxx11/rule-of-zero
/// http://scottmeyers.blogspot.fr/2014/03/a-concern-about-rule-of-zero.html
///
/// In many cases, "copying" objects does not make sense, in which case you may
/// "delete" the copy-constructor and copy-assignement operator:
///
/// \code
/// Object(const Object&) = delete;
/// Object& operator=(const Object&) = delete;
/// \endcode
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
public:
    VGC_CORE_OBJECT(Object)

    /// Destructs an Object.
    ///
    virtual ~Object() VGC_CORE_OBJECT_DEFAULT_("destructed")

    /// Constructs an Object.
    ///
    Object() VGC_CORE_OBJECT_DEFAULT_("constructed")

    /// Copy-constructs an Object.
    ///
    Object(const Object&) VGC_CORE_OBJECT_DEFAULT_("copy-constructed")

    /// Move-constructs an Object.
    ///
    Object(Object&&) VGC_CORE_OBJECT_DEFAULT_("move-constructed")

    /// Copy-assigns the given Object to this Object.
    ///
    Object& operator=(const Object&) VGC_CORE_OBJECT_DEFAULT_("copy-assigned")

    /// Move-assigns the given Object to this Object.
    ///
    Object& operator=(Object&&) VGC_CORE_OBJECT_DEFAULT_("move-assigned")
};

} // namespace core
} // namespace vgc

#endif // VGC_CORE_OBJECT_H
