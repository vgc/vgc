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

#ifndef VGC_DOM_ATTRIBUTE_H
#define VGC_DOM_ATTRIBUTE_H

#include <vgc/core/object.h>
#include <vgc/core/stringid.h>
#include <vgc/dom/api.h>
#include <vgc/dom/value.h>

namespace vgc::dom {

/// \class vgc::dom::AuthoredAttribute
/// \brief Holds the data of an authored attribute.
///
class VGC_DOM_API AuthoredAttribute {
public:
    /// Creates an authored attribute.
    ///
    AuthoredAttribute(core::StringId name, const Value& value)
        : name_(name)
        , value_(value) {
    }

    /// Returns the name of this authored attribute.
    ///
    core::StringId name() const {
        return name_;
    }

    /// Returns the value of this authored attribute.
    ///
    const Value& value() const {
        return value_;
    }

    /// Sets the value of this authored attribute.
    ///
    void setValue(const Value& value) {
        value_ = value;
    }

    /// Sets the value of this authored attribute.
    ///
    void setValue(Value&& value) {
        value_ = std::move(value);
    }

    /// Returns the ValueType of this authored attribute.
    ///
    ValueType valueType() const {
        return value_.type();
    }

private:
    core::StringId name_;
    Value value_;
};

} // namespace vgc::dom

#endif // VGC_DOM_ATTRIBUTE_H
