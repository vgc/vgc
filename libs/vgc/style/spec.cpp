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

#include <vgc/style/spec.h>

#include <vgc/style/logcategories.h>

namespace vgc::style {

void StyleValue::parse_(const StylePropertySpec* spec) {
    const detail::UnparsedValue* v = std::any_cast<detail::UnparsedValue>(&value_);
    StylePropertyParser parser = spec ? spec->parser() : &parseStyleDefault;
    StyleValue parsed = parser(v->tokens().begin(), v->tokens().end());
    if (parsed.type() == StyleValueType::Invalid) {
        VGC_WARNING(
            LogVgcStyle,
            "Failed to parse attribute '{}' defined as '{}'.",
            spec->name(),
            v->rawString());
        *this = StyleValue::none();
    }
    else {
        *this = parsed;
    }
}

StyleValue parseStyleDefault(StyleTokenIterator begin, StyleTokenIterator end) {
    if (end == begin + 1) {
        StyleTokenType t = begin->type();
        if (t == StyleTokenType::Identifier) {
            return StyleValue::identifier(begin->stringValue());
        }
    }
    return StyleValue::invalid();
}

} // namespace vgc::style