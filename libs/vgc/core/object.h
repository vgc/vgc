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

#define VGC_CORE_OBJECT_MAKE_(T)                                      \
    /** Constructs an object of type T managed by a shared pointer */ \
    template <typename... Args>                                       \
    static std::shared_ptr<T> make(Args... args) {                    \
        return std::make_shared<T>(args...);                          \
    }

#define VGC_CORE_OBJECT_SHARED_PTR_(T)                                   \
    /** Returns a shared_ptr managing this object. Assumes the object */ \
    /** was created via the make() statid method.                     */ \
    std::shared_ptr<T> sharedPtr() {                                     \
        return std::static_pointer_cast<T>(this->shared_from_this());    \
    }

#define VGC_CORE_OBJECT_WEAK_PTR_(T)                             \
    /** Returns a weak_ptr to this object. Assumes the object */ \
    /** was created via the make() statid method.             */ \
    std::weak_ptr<T> weakPtr() {                                 \
        return sharedPtr(); /* Note: C++17 has weak_from_this */ \
    }

#define VGC_CORE_OBJECT_CONST_SHARED_PTR_(T)                                \
    /** Returns a shared_ptr managing this object. Assumes the object */    \
    /** was created via the make() statid method.                     */    \
    std::shared_ptr<const T> sharedPtr() const {                            \
        return std::static_pointer_cast<const T>(this->shared_from_this()); \
    }

#define VGC_CORE_OBJECT_CONST_WEAK_PTR_(T)                       \
    /** Returns a weak_ptr to this object. Assumes the object */ \
    /** was created via the make() statid method.             */ \
    std::weak_ptr<const T> weakPtr() const {                     \
        return sharedPtr(); /* Note: C++17 has weak_from_this */ \
    }

#define VGC_CORE_OBJECT(T)               \
    VGC_CORE_OBJECT_MAKE_(T)             \
    VGC_CORE_OBJECT_SHARED_PTR_(T)       \
    VGC_CORE_OBJECT_WEAK_PTR_(T)         \
    VGC_CORE_OBJECT_CONST_SHARED_PTR_(T) \
    VGC_CORE_OBJECT_CONST_WEAK_PTR_(T)

namespace vgc {
namespace core {

/// \class vgc::core::Object
/// \brief Base class of most classes in the VGC codebase.
///
/// This base class allows the VGC codebase to be more readable, consistent,
/// maintainable, thread-safe, and Python-binding-friendly at only a very small
/// performance cost. It is strongly recommended that most classes derive from
/// Object. In particular any class that contains a `std::vector`, a
/// `std::string`, any other container, or performs some sort of memory
/// allocation (e.g., pimpl idiom) should either inherit from Object or
/// document clearly why they do not. Notable exceptions include small
/// struct-like classes such as `Vec2d`. Indeed, these do not perform any
/// dynamic allocation, are expected to be potentially instanciated more than a
/// million times in a session, and are expected or measured to be a bottleneck
/// if they would derive from Object.
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
/// In the example above, the macro VGC_CORE_OBJECT(Foo) defines the following
/// member methods:
///
/// \code
/// static FooSharedPtr make();
/// static FooSharedPtr make(int x);
/// FooSharedPtr sharedPtr();
/// FooWeakPtr weakPtr();
/// FooConstSharedPtr sharedPtr() const;
/// FooConstWeakPtr weakPtr() const;
/// \endcode
///
/// Despite having public constructors, objects should always be instanciated
/// via their make() static method, like so:
///
/// \code
/// FooSharedPtr foo1 = Foo::make();
/// FooSharedPtr foo2 = Foo::make(42);
/// \endcode
///
/// This is equivalent to `std::make_shared<Foo>(...)`, and ensures that the
/// object is dynamically allocated and managed by a `std::shared_ptr`. In most
/// cases, ownership is not actually "shared", and one may think that using
/// `unique_ptr` or simply allocating on the stack would be preferred. However,
/// always using `shared_ptr` provides consistency, and always allow observers
/// to hold `weak_ptr` to these objects if they need to, by calling
/// `foo->weakPtr()`.
///
/// Despite being managed by a `shared_ptr`, objects should typically be passed
/// by raw pointers. Indeed, this makes the code more performant, flexible, and
/// readable. Unless stated otherwise, any function that takes an Object* as
/// parameter assumes that the following is true:
///
/// 1. The pointer is non-null.
///
/// 2. The object was created via make(), that is, the function can called
///    obj->weakPtr() whithout checking for exceptions.
///
/// 3. The lifetime of the object exceeds the lifetime of the function.
///
/// This follows the "keep it simple" advice from Herb Sutter, and I strongly
/// encourage any VGC developer to watch the following talk before
/// contributing, which basically states that there's nothing wrong with raw
/// pointers:
///
/// https://www.youtube.com/watch?v=xnqTKD8uD64
///
/// If a function desires to keep a pointer to the passed object for
/// observation purposes, then it should hold a `weak_ptr` to this object.
/// Indeed, holding the raw pointer is unsafe (the object might be deleted
/// later), and holding a `shared_ptr` would extend the lifetime of the
/// observed object, which is usually not desired. Instead, by using a
/// `weak_ptr` the function does not extend the lifetime of the object, but can
/// still check later whether the object still exists.
///
/// If an object is passed to a function by shared_ptr, this typically indicates
/// that the function takes ownership of the object. In this case, pass the
/// shared_ptr directly by value, like so:
///
/// \code
/// myFunction(FooSharedPtr foo);
/// \endcode
///
/// In some very rare cases, typically for performance, you may still want to
/// allocate Object instances on the stack, by directly using the public
/// constructors instead of make(). It's okay to do so, but then do not pass
/// these objects to generic functions taking objects by raw pointers.
/// Remember: unless stated otherwise, functions assume that it is safe to call
/// obj->weakPtr(). Therefore, if you take the risk of creating objects on the
/// stack, only call functions which you control, and which provide weaker
/// assumptions.
///
/// Note that since the destructor of Object is virtual, there is no need to
/// explicitly declare a (virtual) destructor in derived classes. In many
/// (most?) cases, the default destructor is the most appropriate. In general,
/// derived classes should strive for "the rule of zero": no custom destructor,
/// copy constructor, move constructor, assignment operator, and move
/// assignment operator. If you declare any of these, please declare all of
/// these. See also:
///
/// http://en.cppreference.com/w/cpp/language/rule_of_three
/// https://blog.rmf.io/cxx11/rule-of-zero
/// http://scottmeyers.blogspot.fr/2014/03/a-concern-about-rule-of-zero.html
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
