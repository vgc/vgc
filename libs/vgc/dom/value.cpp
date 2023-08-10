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

#include <vgc/dom/document.h>
#include <vgc/dom/element.h>
#include <vgc/dom/exceptions.h>
#include <vgc/dom/path.h>

namespace vgc::dom {

VGC_DEFINE_ENUM(
    ValueType,
    (None, "None"),
    (Invalid, "Invalid"),
    (String, "String"),
    (StringId, "StringId"),
    (Int, "Int"),
    (IntArray, "IntArray"),
    (Double, "Double"),
    (DoubleArray, "DoubleArray"),
    (Color, "Color"),
    (ColorArray, "ColorArray"),
    (Vec2d, "Vec2d"),
    (Vec2dArray, "Vec2dArray"),
    (Path, "Path"),
    (NoneOrPath, "Path"),
    (PathArray, "PathArray"),
    (Custom, "Custom"))

const Value& Value::none() {
    // trusty leaky singleton
    static const Value* v = new Value();
    return *v;
}

const Value& Value::invalid() {
    // trusty leaky singleton
    static const Value* v = new Value(InvalidValue{});
    return *v;
}

void Value::clear() {
    var_ = NoneValue{};
}

void Value::preparePathsForUpdate_(const Element* owner) const {
    switch (type()) {
    case ValueType::None:
    case ValueType::Invalid:
    case ValueType::String:
    case ValueType::StringId:
    case ValueType::Int:
    case ValueType::IntArray:
    case ValueType::Double:
    case ValueType::DoubleArray:
    case ValueType::Color:
    case ValueType::ColorArray:
    case ValueType::Vec2d:
    case ValueType::Vec2dArray:
        break;
    case ValueType::Path: {
        const Path& p = std::get<Path>(var_);
        detail::preparePathForUpdate(p, owner);
        break;
    }
    case ValueType::NoneOrPath: {
        const NoneOr<Path>& np = std::get<NoneOr<Path>>(var_);
        if (np.has_value()) {
            detail::preparePathForUpdate(np.value(), owner);
        }
        break;
    }
    case ValueType::PathArray: {
        const PathArray& pa = std::get<PathArray>(var_);
        for (const Path& p : pa) {
            detail::preparePathForUpdate(p, owner);
        }
        break;
    }
    case ValueType::Custom: {
        const CustomValue* cv = getCustomValuePtr();
        if (cv->hasPaths_) {
            cv->preparePathsForUpdate_(owner);
        }
        break;
    }
    case ValueType::End_:
        break;
    }
}

void Value::updatePaths_(const Element* owner, const PathUpdateData& data) {
    switch (type()) {
    case ValueType::None:
    case ValueType::Invalid:
    case ValueType::String:
    case ValueType::StringId:
    case ValueType::Int:
    case ValueType::IntArray:
    case ValueType::Double:
    case ValueType::DoubleArray:
    case ValueType::Color:
    case ValueType::ColorArray:
    case ValueType::Vec2d:
    case ValueType::Vec2dArray:
        break;
    case ValueType::Path: {
        Path& p = std::get<Path>(var_);
        detail::updatePath(p, owner, data);
        break;
    }
    case ValueType::NoneOrPath: {
        NoneOr<Path>& np = std::get<NoneOr<Path>>(var_);
        ;
        if (np.has_value()) {
            detail::updatePath(np.value(), owner, data);
        }
        break;
    }
    case ValueType::PathArray: {
        PathArray& pa = std::get<PathArray>(var_);
        for (Path& p : pa) {
            detail::updatePath(p, owner, data);
        }
        break;
    }
    case ValueType::Custom: {
        CustomValue* v = const_cast<CustomValue*>(getCustomValuePtr());
        if (v->hasPaths_) {
            v->updatePaths_(owner, data);
        }
        break;
    }
    case ValueType::End_:
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

void parseValue(Value& value, const std::string& s) {
    ValueType t = value.type();
    try {
        switch (t) {
        case ValueType::None:
            checkExpectedString_(s, "None");
            break;
        case ValueType::Invalid:
            checkExpectedString_(s, "Invalid");
            break;
        case ValueType::String:
            value.set(s);
            break;
        case ValueType::StringId:
            value.set(core::StringId(s));
            break;
        case ValueType::Int:
            value.set(core::parse<Int>(s));
            break;
        case ValueType::IntArray:
            value.set(core::parse<core::IntArray>(s));
            break;
        case ValueType::Double:
            value.set(core::parse<double>(s));
            break;
        case ValueType::DoubleArray:
            value.set(core::parse<core::DoubleArray>(s));
            break;
        case ValueType::Color:
            value.set(core::parse<core::Color>(s));
            break;
        case ValueType::ColorArray:
            value.set(core::parse<core::ColorArray>(s));
            break;
        case ValueType::Vec2d:
            value.set(core::parse<geometry::Vec2d>(s));
            break;
        case ValueType::Vec2dArray:
            value.set(core::parse<geometry::Vec2dArray>(s));
            break;
        case ValueType::Path:
            value.set(core::parse<Path>(s));
            break;
        case ValueType::NoneOrPath:
            value.set(core::parse<NoneOr<Path>>(s));
            break;
        case ValueType::PathArray:
            value.set(core::parse<PathArray>(s));
            break;
        case ValueType::Custom: {
            StreamReader sr(s);
            const_cast<CustomValue*>(value.getCustomValuePtr())->read(sr);
            break;
        }
        case ValueType::End_:
            break;
        }
    }
    catch (const core::ParseError& e) {
        throw VgcSyntaxError(core::format(
            "Failed to convert '{}' into a Value of type {} for the following reason: {}",
            s,
            t,
            e.what()));
    }
    return;
}

void readTo(detail::CustomValueHolder& v, StreamReader& in) {
    v->read(in);
}

} // namespace vgc::dom
