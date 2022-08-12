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

#include <vgc/dom/value.h>

#include <vgc/dom/exceptions.h>

namespace vgc::dom {

const Value& Value::none() {
    // trusty leaky singleton
    static const Value* v = new Value(ValueType::None);
    return *v;
}

const Value& Value::invalid() {
    // trusty leaky singleton
    static const Value* v = new Value(ValueType::Invalid);
    return *v;
}

void Value::clear() {
    type_ = ValueType::None;
    var_ = std::monostate{};
}

void Value::shrinkToFit() {
    switch (type_) {
    case ValueType::DoubleArray:
        std::get<std::shared_ptr<core::DoubleArray>>(var_)->shrinkToFit();
        break;
    case ValueType::Vec2dArray:
        std::get<std::shared_ptr<geometry::Vec2dArray>>(var_)->shrinkToFit();
        break;
    default:
        break;
    }
}

namespace {

void checkExpectedString_(const std::string& s, const char* expected) {
    core::StringReader in(s);
    core::skipExpectedString(in, expected);
    skipWhitespaceCharacters(in);
    skipExpectedEof(in);
}

} // namespace

Value parseValue(const std::string& s, ValueType t) {
    try {
        switch (t) {
        case ValueType::None:
            checkExpectedString_(s, "None");
            return Value();
        case ValueType::Invalid:
            checkExpectedString_(s, "Invalid");
            return Value::invalid();
        case ValueType::Color:
            return Value(core::parse<core::Color>(s));
        case ValueType::DoubleArray:
            return Value(core::parse<core::DoubleArray>(s));
        case ValueType::Vec2dArray:
            return Value(core::parse<geometry::Vec2dArray>(s));
        }
    }
    catch (const core::ParseError& e) {
        throw VgcSyntaxError(
            "Failed to convert '" + s + "' into a Value of type " + core::toString(t)
            + " for the following reason: " + e.what());
    }
    return Value::invalid(); // Silence "not all control paths return a value" in MSVC
}

} // namespace vgc::dom
