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
#include <vgc/style/strings.h>

namespace vgc::style {

StyleValue parseColor(StyleTokenIterator begin, StyleTokenIterator end) {
    try {
        std::string str(begin->begin, (end - 1)->end);
        core::Color color = core::parse<core::Color>(str);
        return StyleValue::custom(color);
    }
    catch (const core::ParseError&) {
        return StyleValue::invalid();
    }
    catch (const core::RangeError&) {
        return StyleValue::invalid();
    }
}

StyleValue parseLength(StyleTokenIterator begin, StyleTokenIterator end) {
    // For now, we only support a unique Dimension token with a "dp" unit
    if (begin == end) {
        return StyleValue::invalid();
    }
    else if (
        begin->type == StyleTokenType::Dimension //
        && begin->codePointsValue == "dp"        //
        && begin + 1 == end) {

        return StyleValue::number(begin->toFloat());
    }
    else {
        return StyleValue::invalid();
    }
}

StyleValue parseIdentifierAmong(
    StyleTokenIterator begin,
    StyleTokenIterator end,
    std::initializer_list<core::StringId> list) {

    if (end == begin + 1) {
        StyleTokenType t = begin->type;
        if (t == StyleTokenType::Identifier) {
            for (core::StringId id : list) {
                if (id == begin->codePointsValue) {
                    return StyleValue::identifier(begin->codePointsValue);
                }
            }
        }
    }
    return StyleValue::invalid();
}

} // namespace vgc::style
