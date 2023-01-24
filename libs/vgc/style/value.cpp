// Copyright 2022 The VGC Developers
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

#include <vgc/style/value.h>

namespace vgc::style {

VGC_DEFINE_ENUM(
    StyleValueType,
    (None, "None"),
    (Unparsed, "Unparsed"),
    (Invalid, "Invalid"),
    (Inherit, "Inherit"),
    (Identifier, "Identifier"),
    (Number, "Number"),
    (String, "String"),
    (Custom, "Custom"))

StyleValue StyleValue::unparsed(StyleTokenIterator begin, StyleTokenIterator end) {
    return StyleValue(StyleValueType::Unparsed, detail::UnparsedValue(begin, end));
}

namespace detail {

UnparsedValue::UnparsedValue(StyleTokenIterator begin, StyleTokenIterator end)
    : rawString_(initRawString(begin, end))
    , tokens_(begin, end) {

    remapPointers_();
}

UnparsedValue::UnparsedValue(const UnparsedValue& other)
    : rawString_(other.rawString_)
    , tokens_(other.tokens_) {

    remapPointers_();
}

UnparsedValue::UnparsedValue(UnparsedValue&& other)
    : rawString_(std::move(other.rawString_))
    , tokens_(std::move(other.tokens_)) {

    remapPointers_();
}

UnparsedValue& UnparsedValue::operator=(const UnparsedValue& other) {
    if (this != &other) {
        rawString_ = other.rawString_;
        tokens_ = other.tokens_;
        remapPointers_();
    }
    return *this;
}

UnparsedValue& UnparsedValue::operator=(UnparsedValue&& other) {
    if (this != &other) {
        rawString_ = std::move(other.rawString_);
        tokens_ = std::move(other.tokens_);
        remapPointers_();
    }
    return *this;
}

std::string
UnparsedValue::initRawString(StyleTokenIterator begin, StyleTokenIterator end) {
    if (begin == end) {
        return "";
    }
    else {
        return std::string(begin->begin(), (end - 1)->end());
    }
}

// Ensures that the `const char*` pointers in `tokens_` points to the
// copied data in `rawString_`.
//
// Note that remapping is still needed even for the move constructor and
// move assignment operator, due to small-string optimization: a moved
// string may still have its data at a new location.
//
void UnparsedValue::remapPointers_() {
    if (!tokens_.isEmpty()) {
        const char* oldBegin = tokens_.begin()->begin();
        const char* newBegin = rawString_.data();
        std::ptrdiff_t offset = newBegin - oldBegin;
        if (offset != 0) {
            for (StyleToken& token : tokens_) {
                token.begin_ += offset;
                token.end_ += offset;
            }
        }
    }
}

} // namespace detail

} // namespace vgc::style
