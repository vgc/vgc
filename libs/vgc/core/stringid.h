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

#ifndef VGC_CORE_STRINGID_H
#define VGC_CORE_STRINGID_H

#include <functional> // hash
#include <string>
#include <vgc/core/api.h>

namespace vgc::core {

/// \class vgc::core::StringId
/// \brief Represents a fast-to-compare and cheap-to-copy immutable string.
///
/// This class implements a technique called "string interning":
///
/// https://en.wikipedia.org/wiki/String_interning
///
/// The idea is that for fixed string values which you expect to use very
/// frequently (e.g., XML element names), it is quite inefficient to store and
/// compare multiple copies of the same exact std::string. Instead, it may be
/// preferable to store the std::string only once in a global pool, and then
/// simply keep a pointer to this std::string. This is what StringId does for
/// you.
///
/// StringId instances are extremely fast to compare and cheap to copy, but are
/// slower to instanciate than a regular std::string, due to the need of a
/// lookup in the global pool and a mutex lock for thread safety.
///
/// Please keep in mind that in most cases, you should simply use an
/// std::string instead of a StringId. In particular, you should NOT use a
/// StringId to store temporary strings generated at run-time. Indeed, the
/// underlying std::string will never be deallocated, that is, the global pool
/// of strings only keeps growing, making further instanciations of StringId
/// slower.
///
/// A StringId can be constructed as follows:
///
/// \code
/// StringId s("my string");
/// \endcode
///
/// The underlying std::string referred to by the StringId can be accessed via
/// StringId::string().
///
/// StringId instances can be safely copied and assigned, like so:
///
/// \code
/// StringId s1("some string");
/// StringId s2(s1);
/// StringId s3 = s1;
/// \endcode
///
/// StringId instances can also be compared like so:
///
/// \code
/// StringId s1("some string");
/// StringId s2("some other string");
/// StringId s3("some string");
/// assert(s1 != s2);
/// assert(s1 == s3);
/// \endcode
///
/// Since constructing StringId instances is slow, but copying and comparing
/// them is extremely fast, it is a good practice to define StringId instances
/// with static storage duration (= "global variables") and re-use them. In order
/// to prevent the "static initialization order fiasco", the safe way to do this
/// is via static local objects:
///
/// \code
/// // foo.h
/// StringId someString();
///
/// // foo.cpp
/// StringId someString() {
///     static StringId s("some string");
///     return s;
/// }
/// \endcode
///
/// Note that StringId is trivially destructible, therefore there is no need to
/// worry about static destruction order, which is why the above code is safe,
/// and even thread-safe since C++11. See the following for details:
/// - https://isocpp.org/wiki/faq/ctors#construct-on-first-use-v2
/// - https://google.github.io/styleguide/cppguide.html#Static_and_Global_Variables
///
class VGC_CORE_API StringId {
public:
    // Constructs a StringId representing the empty string.
    //
    StringId();

    /// Constructs a StringId representing the given string \p s.
    ///
    /// This constructor is explicit in order to avoid interning strings by
    /// mistake, and resolve ambiguity of overloaded operator<<. This means
    /// that if you define a function foo(StringId), and wish to allow clients
    /// to call foo(std::string) without explicit cast, you need to explicitly
    /// define foo(std::string) and explicitly perform the cast there.
    ///
    explicit StringId(const std::string& s);

    /// Constructs a StringId representing the given string \p s.
    ///
    explicit StringId(const char s[])
        : StringId(std::string(s)) {
    }

    /// Returns the string represented by this StringId.
    ///
    const std::string& string() const {
        return *stringPtr_;
    }

    /// Allows implicit conversion from StringId to std::string.
    ///
    operator const std::string&() const {
        return string();
    }

    /// Compares the two StringId. This doesn't have any meaning and is
    /// typically only useful for storing StringId instances in a map. This is
    /// NOT the alphabetical order.
    ///
    bool operator<(const StringId& other) const {
        return stringPtr_ < other.stringPtr_;
    }

    /// Returns whether the two StringId are equal. This is equivalent to
    /// whether their underlying strings are equals.
    ///
    bool operator==(const StringId& other) const {
        return stringPtr_ == other.stringPtr_;
    }

    /// Returns whether this StringId is equal to the given string.
    ///
    bool operator==(const std::string& other) const {
        // Note: comparing two std::strings is typically faster than building
        // a StringId from an std::string, so we choose to do the former.
        return *stringPtr_ == other;
    }

    /// Returns whether the two StringId are different.
    ///
    bool operator!=(const StringId& other) const {
        return stringPtr_ != other.stringPtr_;
    }

    /// Returns whether this StringId is different from the given string.
    ///
    bool operator!=(const std::string& other) const {
        return *stringPtr_ != other;
    }

private:
    friend struct std::hash<StringId>;
    const std::string* stringPtr_;
};

namespace strings {

VGC_CORE_API extern const StringId empty;

} // namespace strings

/// Returns whether the given std::string is equal to the given StringId.
///
inline bool operator==(const std::string& s1, const StringId& s2) {
    return s2 == s1;
}

/// Returns whether the given std::string is different from the given StringId.
///
inline bool operator!=(const std::string& s1, const StringId& s2) {
    return s2 != s1;
}

} // namespace vgc::core

/// Writes the underlying string of the given \p stringId to the given output
/// stream \p out.
///
template<typename OutputStream>
OutputStream& operator<<(OutputStream& out, vgc::core::StringId stringId) {
    out << stringId.string();
    return out;
}

// Implement hash function to make StringId compatible with std::unordered_map
namespace std {
template<>
struct hash<vgc::core::StringId> {
    std::size_t operator()(const vgc::core::StringId& s) const {
        return std::hash<const std::string*>()(s.stringPtr_);
    }
};
} // namespace std

#endif // VGC_CORE_STRINGID_H
