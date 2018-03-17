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

#ifndef VGC_CORE_STRINGID_H
#define VGC_CORE_STRINGID_H

#include <string>
#include <vgc/core/api.h>

namespace vgc {
namespace core {

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
/// StringId myString() {
///     static StringId s("my string");
///     return s;
/// }
/// \endcode
///
/// Note that StringId is trivially destructible, therefore there is no need to
/// worry about static destruction order. See the following for details:
/// - https://isocpp.org/wiki/faq/ctors#construct-on-first-use-v2
/// - https://google.github.io/styleguide/cppguide.html#Static_and_Global_Variables
///
class VGC_CORE_API StringId
{
public:
    /// Constructs a StringId representing the given string \p s.
    ///
    StringId(const std::string& s = "");

    /// Returns the string represented by this StringId.
    ///
    std::string string() const {
        return *stringPtr_;
    }

private:
    const std::string* stringPtr_;
};

} // namespace core
} // namespace vgc

#endif // VGC_CORE_STRINGID_H
