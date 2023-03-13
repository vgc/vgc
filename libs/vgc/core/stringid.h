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
#include <string_view>

#include <vgc/core/api.h>
#include <vgc/core/format.h>

namespace vgc::core {

/// \class vgc::core::StringId
/// \brief Represents a fast-to-compare and cheap-to-copy immutable string.
///
/// This class implements a technique called "string interning":
///
/// https://en.wikipedia.org/wiki/String_interning
///
/// The idea is that for fixed string values which are expected to be used and
/// compared frequently (e.g., XML element names), it is inefficient to store
/// and compare multiple copies of the same exact `std::string`. Instead, you
/// can use a `StringId`, which instanciates a `std::string` in a global pool
/// the first time it encounters a new string value, and then simply keep a
/// pointer to this `std::string`.
///
/// `StringId` instances are extremely fast to compare and cheap to copy, but
/// are slower to instanciate than a regular `std::string`, due to the need of
/// a lookup in the global pool and a mutex lock for thread safety.
///
/// Keep in mind that in many cases, using a `std::string` instead of a
/// `StringId` is still the best choice. In particular, you should NOT use a
/// `StringId` to store temporary strings generated at run-time. Indeed, the
/// underlying `std::string` will never be deallocated, that is, the global
/// pool of strings only keeps growing, making further instanciations of
/// `StringId` slower.
///
/// A `StringId` can be constructed as follows:
///
/// ```cpp
/// StringId s("my string");
/// ```
///
/// The underlying `std::string` referred to by the StringId can be accessed via
/// `StringId::string()`.
///
/// `StringId` instances can be safely copied and assigned, like so:
///
/// ```cpp
/// StringId s1("some string");
/// StringId s2(s1);
/// StringId s3 = s1;
/// ```
///
/// `StringId` instances can also be compared like so:
///
/// ```cpp
/// StringId s1("some string");
/// StringId s2("some other string");
/// StringId s3("some string");
/// assert(s1 != s2);
/// assert(s1 == s3);
/// ```
///
/// Since constructing `StringId` instances is slow, but copying and comparing
/// them is extremely fast, it is a good practice to define `StringId` instances
/// with static storage duration (= "global variables") and re-use them. In order
/// to prevent the "static initialization order fiasco", the safe way to do this
/// is via static local objects:
///
/// ```cpp
/// // foo.h
/// StringId someString();
///
/// // foo.cpp
/// StringId someString() {
///     static StringId s("some string");
///     return s;
/// }
/// ```
///
/// Note that `StringId` is trivially destructible, therefore there is no need to
/// worry about static destruction order, which is why the above code is safe,
/// and even thread-safe since C++11. See the following for details:
/// - https://isocpp.org/wiki/faq/ctors#construct-on-first-use-v2
/// - https://google.github.io/styleguide/cppguide.html#Static_and_Global_Variables
///
class VGC_CORE_API StringId {
public:
    // Constructs a `StringId` representing the empty string.
    //
    constexpr StringId() noexcept
        : stringPtr_(nullptr) {
    }

    /// Constructs a `StringId` representing the given string_view `s`.
    ///
    /// This constructor is explicit in order to avoid interning strings by
    /// mistake, and resolve ambiguity of overloaded `operator<<`. This means
    /// that if you define a function `foo(StringId)`, but also want this
    /// function to be callable by passing an `std::string_view`, you need to
    /// explicitly define the overload `foo(std::string_view)`, otherwise
    /// clients have to perform the explicit cast themselves.
    ///
    explicit StringId(std::string_view s)
        : stringPtr_(nullptr) {

        if (!s.empty()) {
            init_(s);
        }
    }

    /// Returns the string represented by this `StringId`.
    ///
    const std::string& string() const noexcept {
        static const std::string empty;
        if (stringPtr_) {
            return *stringPtr_;
        }
        else {
            return empty;
        }
    }

    /// Allows implicit conversion from `StringId` to `std::string_view`.
    ///
    operator std::string_view() const noexcept {
        return string();
    }

    /// Returns whether the string represented by this `StringId` is the empty
    /// string.
    ///
    constexpr bool isEmpty() const noexcept {
        return stringPtr_ == nullptr;
    }

    /// Returns whether the two `StringId` are equal. This is equivalent to
    /// whether their underlying strings are equals.
    ///
    bool operator==(StringId other) const noexcept {
        return stringPtr_ == other.stringPtr_;
    }

    /// Returns whether this `StringId` is equal to the given `std::string_view`.
    ///
    bool operator==(std::string_view other) const noexcept {
        // Note: comparing two std::strings is typically faster than building
        // a StringId from an std::string, so we choose to do the former.
        return string() == other;
    }

    /// Returns whether the two `StringId` are different.
    ///
    bool operator!=(StringId other) const noexcept {
        return stringPtr_ != other.stringPtr_;
    }

    /// Returns whether this `StringId` is different from the given `std::string_view`.
    ///
    bool operator!=(std::string_view other) const noexcept {
        return string() != other;
    }

    /// Compares the two `StringId`. This doesn't have any meaning and is
    /// typically only useful for storing `StringId` instances in a map. This is
    /// NOT the alphabetical order.
    ///
    bool operator<(StringId other) const noexcept {
        return stringPtr_ < other.stringPtr_;
    }

    /// Returns the result of a lexicographical comparison with the `other` string.
    ///
    int compare(StringId other) const noexcept {
        return string().compare(other.string());
    }

    /// Returns the result of a lexicographical comparison with the `other` string.
    ///
    int compare(std::string_view other) const noexcept {
        return string().compare(other);
    }

private:
    friend struct std::hash<StringId>;
    const std::string* stringPtr_;
    void init_(std::string_view s);
};

/// Returns whether the given `std::string_view` is equal to the given `StringId`.
///
inline bool operator==(std::string_view s1, StringId s2) noexcept {
    return s2 == s1;
}

/// Returns whether the given `std::string_view` is different from the given `StringId`.
///
inline bool operator!=(std::string_view s1, StringId s2) noexcept {
    return s2 != s1;
}

/// Writes the underlying string of the given `stringId` to the given output
/// stream `out`.
///
template<typename OutputStream>
OutputStream& operator<<(OutputStream& out, StringId stringId) {
    out << stringId.string();
    return out;
}

/// Writes the given StringId to the output stream.
///
template<typename OStream>
void write(OStream& out, StringId x) {
    write(out, std::string_view(x));
}

} // namespace vgc::core

namespace std {

// Implement hash function to make StringId compatible with std::unordered_map
template<>
struct hash<vgc::core::StringId> {
    std::size_t operator()(vgc::core::StringId s) const {
        return std::hash<const std::string*>()(s.stringPtr_);
    }
};

} // namespace std

template<>
struct fmt::formatter<vgc::core::StringId> : fmt::formatter<std::string_view> {
    template<typename FormatContext>
    auto format(vgc::core::StringId s, FormatContext& ctx) {
        return fmt::formatter<std::string_view>::format(s.string(), ctx);
    }
};

#endif // VGC_CORE_STRINGID_H
