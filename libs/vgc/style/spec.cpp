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

void Value::parse_(const PropertySpec* spec) {
    const detail::UnparsedValue* v = std::any_cast<detail::UnparsedValue>(&value_);
    PropertyParser parser = spec ? spec->parser() : &parseStyleDefault;
    Value parsed = parser(v->tokens().begin(), v->tokens().end());
    if (parsed.type() == ValueType::Invalid) {
        VGC_WARNING(
            LogVgcStyle,
            "Failed to parse attribute '{}' defined as '{}'.",
            spec->name(),
            v->rawString());
        *this = Value::none();
    }
    else {
        *this = parsed;
    }
}

Value parseStyleDefault(TokenIterator begin, TokenIterator end) {
    if (end == begin + 1) {
        TokenType t = begin->type();
        if (t == TokenType::Identifier) {
            return Value::identifier(begin->stringValue());
        }
    }
    return Value::invalid();
}

void SpecTable::insert(
    core::StringId attributeName,
    const Value& initialValue,
    bool isInherited,
    PropertyParser parser) {

    if (get(attributeName)) {
        VGC_WARNING(
            LogVgcStyle,
            "Attempting to insert a property spec for the attribute '{}', which is "
            "already registered. Aborted.",
            attributeName);
        return;
    }
    PropertySpec spec(attributeName, initialValue, isInherited, parser);
    map_.insert({attributeName, spec});
}

bool SpecTable::setRegistered(const core::ObjectType& objectType) {
    auto res = registeredObjectTypes_.insert(objectType);
    return res.second;
}

} // namespace vgc::style
