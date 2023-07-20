// Copyright 2023 The VGC Developers
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

/// \file vgc/core/stringutil.h
/// \brief Helper functions to process strings

#ifndef VGC_CORE_STRINGUTIL_H
#define VGC_CORE_STRINGUTIL_H

#include <algorithm>
#include <string>
#include <string_view>

#include <vgc/core/api.h>
#include <vgc/core/array.h>

namespace vgc::core {

/// Convenient alias for `vgc::core::Array<std::string>`.
///
using StringArray = core::Array<std::string>;

/// Convenient alias for `vgc::core::Array<std::string_view>`.
///
using StringViewArray = core::Array<std::string_view>;

/// Returns whether the string `s` starts with the given `prefix`.
///
/// Note: this is equivalent to `s.starts_with(prefix)` in C++20.
///
constexpr bool startsWith(std::string_view s, std::string_view prefix) {
    size_t n = s.size();
    size_t k = prefix.size();
    return n >= k && s.compare(0, k, prefix) == 0;
}

/// Returns whether the string `s` ends with the given `suffix`.
///
/// Note: this is equivalent to `s.ends_with(suffix)` in C++20.
///
constexpr bool endsWith(std::string_view s, std::string_view suffix) {
    size_t n = s.size();
    size_t k = suffix.size();
    return n >= k && s.compare(n - k, k, suffix) == 0;
}

/// Returns whether the string `s` contains the given `substring`.
///
/// Note: this is equivalent to `s.contains(substring)` in C++23.
///
constexpr bool contains(std::string_view s, std::string_view substring) noexcept {
    return s.find(substring) != std::string_view::npos;
}

/// Returns whether the string `s` contains the character `c`
///
/// Note: this is equivalent to `s.contains(c)` in C++23.
///
constexpr bool contains(std::string_view s, char c) noexcept {
    return s.find(c) != std::string_view::npos;
}

/// Returns the given string `s` without the given leading or trailing characters.
///
/// By default, this removes the whitespace characters ` `, `\t`, `\n`, and `\r`.
///
/// ```cpp
/// std::string_view s = "  hello world  ";
/// std::string_view t = trimmed(s); // => "hello world"
/// ```
///
constexpr std::string_view
trimmed(std::string_view s, std::string_view trimChars = " \t\n\r") {
    size_t first = s.find_first_not_of(trimChars);
    if (first == std::string_view::npos) {
        return "";
    }
    size_t last = s.find_last_not_of(trimChars);
    return std::string_view(s.data() + first, last - first + 1);
}

/// Splits the given string `s` each time the character `c` is encountered,
/// returning an array with all the substrings inbetween.
///
/// Empty strings are preserved, that is, the returned array has a length of `n
/// + 1`, where `n` is the number of times the character `c` appears in the
/// given string.
///
/// ```cpp
/// split(":hello::world:", ':'); // => ["", "hello", "", "world", ""]
/// ```
///
// TODO: Make Array constructors and emplaceLast constexpr so that this
//       function can be constexpr too.
//
inline core::Array<std::string_view> split(std::string_view s, char c) {
    core::Array<std::string_view> res;
    size_t begin = 0;
    size_t end = 0;
    while (end < s.size()) {
        if (s[end] == c) {
            res.emplaceLast(s.data() + begin, end - begin);
            begin = end + 1;
        }
        ++end;
    }
    res.emplaceLast(s.data() + begin, end - begin);
    return res;
}

/// Returns a copy of the string `s` where all occurences of
/// `from` are replaced by `to`.
///
VGC_CORE_API
std::string replace(std::string_view s, std::string_view from, std::string_view to);

} // namespace vgc::core

#endif // VGC_CORE_STRINGUTIL_H
