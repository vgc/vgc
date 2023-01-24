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

#include <vgc/style/parse.h>

#include <vgc/core/color.h>

#include <vgc/style/strings.h>

namespace vgc::style {

Value parseColor(TokenIterator begin, TokenIterator end) {
    if (end == begin + 1 && begin->type() == TokenType::Identifier
        && begin->stringValue() == "inherit") {
        return Value::inherit();
    }
    try {
        const char* b = begin->begin();
        const char* e = (end - 1)->end();
        std::string_view str(b, std::distance(b, e));
        core::Color color = core::parse<core::Color>(str);
        return Value::custom(color);
    }
    catch (const core::ParseError&) {
        return Value::invalid();
    }
    catch (const core::RangeError&) {
        return Value::invalid();
    }
}

Value parseLength(TokenIterator begin, TokenIterator end) {
    // For now, we only support a unique Dimension token with a "dp" unit
    if (begin == end) {
        return Value::invalid();
    }
    else if (
        begin->type() == TokenType::Dimension //
        && begin->stringValue() == "dp"       //
        && begin + 1 == end) {

        return Value::number(begin->floatValue());
    }
    else {
        return Value::invalid();
    }
}

Value parseIdentifierAmong(
    TokenIterator begin,
    TokenIterator end,
    std::initializer_list<core::StringId> list) {

    if (end == begin + 1) {
        TokenType t = begin->type();
        if (t == TokenType::Identifier) {
            for (core::StringId id : list) {
                if (id == begin->stringValue()) {
                    return Value::identifier(begin->stringValue());
                }
            }
        }
    }
    return Value::invalid();
}

} // namespace vgc::style
