// Copyright 2017 The VGC Developers
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

#define VGC_CORE_OBJECT_DEFINE_MAKE(T)                                     \
    /** Constructs an object of type T and wraps it in a shared pointer */ \
    template <typename... Args>                                            \
    static std::shared_ptr<T> make(Args... args) {                         \
        return std::make_shared<T>(args...);                               \
    }

namespace vgc {
namespace core {

/// \class vgc::core::Object
/// \brief Base class of most classes in the VGC codebase.
///
/// This base class allows the VGC codebase to be more readable, consistent,
/// maintainable, thread-safe, and Python-binding-friendly at only a very small
/// performance cost. Classes deriving from Object should be declared as
/// follows:
///
/// \code
/// class Foo: public vgc::core::Object
/// {
/// public:
///
/// //...
/// };
/// \endcode
///
/// It is strongly recommended that new classes derive from
/// Object. In particular any class that contains a std::vector, a std::string,
/// any other container, or performs some sort of memory allocation (e.g.,
/// pimpl idiom) should inherit Object or document clearly why they do not.
/// Notable exceptions include small struct-like classes that do not perform
/// any dynamic allocation, are expected to be potentially instanciated more
/// than a million times in a session, and are expected or measured to be a
/// bottleneck if they would derive from Object (example: Vec2d).
///
/// Typically, objects whose class derives from Object should be instanciated
/// via their make() static method, which return a std::shared_ptr. In most
/// cases, ownership is not actually "shared", but still we always use
/// shared_ptr instead of unique_ptr in order to allow observers to hold
/// weak_ptr to these object, which requires reference counting.
///
/// Even though object owners hold shared_ptrs, prefer passing these objects by
/// raw pointers. This is simpler, more efficient, more readable, and more
/// flexible. Unless stated otherwise, all functions taking Objects by pointers
/// make the following assumptions:
///
/// 1. The pointer is non-null
///
/// 2. The object was created via make(), that is, the function can called
///    obj->ptr() to retrieve a shared pointer from the raw pointer.
///
/// 3. The lifetime of the object exceeds the lifetime of the function.
///
/// This follows the "keep it simple" advice from Herb Sutter, and I strongly
/// encourage any VGC developer to watch this talk before contributing:
///
/// https://www.youtube.com/watch?v=xnqTKD8uD64
///
/// If a function desires to keep a pointer to the passed object for
/// observation purposes, then it should hold a weak_ptr to this object. This
/// way, the lifetime of the observed object is not extended (as it would if
/// the function would hold a shared_ptr instead), and the function can check
/// later if the object still exists (which coudn't be done if the function
/// would hold a raw pointer instead).
///
/// If an object is passed to a function by shared_ptr, this typically indicates
/// that the function takes ownership of the object. In this case, pass the
/// shared_ptr directly by value, like so:
///
/// \code
/// myFunction(FooSharedPtr foo);
/// \endcode
///
class Object: public std::enable_shared_from_this<Object>
{
public:
    VGC_CORE_OBJECT_DEFINE_MAKE(Object)

    /// Returns an std::shared_ptr to this object.
    /// This method is a convenient wrapper around shared_from_this.
    ///
    std::shared_ptr<Object> ptr() {
        return this->shared_from_this();
    }

    /// Destructs the object. Since this destructor is virtual, there is no
    /// need to explicitly declare a (virtual) destructor in derived classes.
    /// In many (most?) cases, the default destructor is the most appropriate.
    /// In general, derived classes should strive for "the rule of zero": no
    /// custom destructor, copy constructor, move constructor, assignment
    /// operator, and move assignment operator. If you declare any of these,
    /// please declare all of these. See also:
    ///
    /// http://en.cppreference.com/w/cpp/language/rule_of_three
    /// https://blog.rmf.io/cxx11/rule-of-zero
    /// http://scottmeyers.blogspot.fr/2014/03/a-concern-about-rule-of-zero.html
    ///
    virtual ~Object() = default;

    // Declare other special member functions as defaults.
    Object() = default;
    Object(const Object&) = default;
    Object(Object&&) = default;
    Object& operator=(const Object&) = default;
    Object& operator=(Object&&) = default;
};

} // namespace core
} // namespace vgc

#endif // VGC_CORE_OBJECT_H
