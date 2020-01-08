// Copyright 2020 The VGC Developers
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

#ifndef VGC_DOM_VALUE_H
#define VGC_DOM_VALUE_H

#include <vgc/core/color.h>
#include <vgc/core/doublearray.h>
#include <vgc/core/vec2d.h>
#include <vgc/core/vec2darray.h>
#include <vgc/dom/api.h>

namespace vgc {
namespace dom {

/// \enum vgc::dom::ValueType
/// \brief Specifies the type of an attribute Value
///
/// Although W3C DOM attribute values are always strings, VGC DOM attribute
/// values are typed, and this class enumerates all allowed types. The type of
/// a given attribute Value can be retrieved at runtime via Value::type().
///
/// In order to write out compliant XML files, we define conversion rules
/// from/to strings in order to correcty encode and decode the types of
/// attributes.
///
/// Typically, in the XML file, types are not explicitly specified, since
/// built-in element types (e.g., "path") have built-in attributes (e.g.,
/// "positions") with known built-in types (in this case, "Vec2dArray").
/// Therefore, the VGC parser already knows what types all of these
/// built-in attributes have.
///
/// However, it is also allowed to add custom data to any element, and the type
/// of this custom data must be encoded in the XML file by following a specific
/// naming scheme. More specifically, the name of custom attributes must be
/// data-<t>-<name>, where <name> is a valid XML attribute name chosen by the
/// user (which cannot be equal to any of the builtin attribute names of this
/// element), and <t> is a one-letter code we call "type-hint". This type-hint
/// allows the parser to unambiguously deduce the type of the attribute from
/// the XML string value. The allowed type-hints are:
///
/// - c: color
/// - d: 64bit floating point
/// - i: 32bit integer
/// - s: string
///
/// For example, if you wish to store a custom sequence of indices (that is,
/// some data of type IntArray) into a given path, you can do as follows:
///
/// \code
/// <path
///     data-i-myIndices="[12, 42, 10]"
/// />
/// \endcode
///
/// If you wish to store a custom sequence of 2D coordinates (Vec2dArray):
///
/// \code
/// <path
///     data-d-myCoords="[(0, 0), (12, 42), (10, 34)]"
/// />
/// \endcode
///
/// Note how in the example above, the type "Vec2dArray" is automatically
/// deduced from the type-hint 'd' and the XML string value. However, without
/// the type-hint, the parser wouldn't be able to determine whether the
/// attribute has the type Vec2dArray or Vec2iArray.
///
/// XXX What if it's an empty array? data-d-myData="[]"? How to determine the
/// type from the type-hint? -> maybe we should forget about this "type-hint"
/// idea altogether, and just have authors always give the full type:
/// data-Vec2dArray-myCoords="[]"
///
enum class ValueType {
    // XXX TODO: complete the list of types
    Invalid,
    Color,
    DoubleArray,
    Vec2dArray
};

/// Returns a string representation of the given ValueType.
///
VGC_DOM_API
std::string toString(ValueType v);

/// \class vgc::dom::Value
/// \brief Holds the value of an attribute
///
class VGC_DOM_API Value
{
public:
    /// Constructs an invalid Value.
    ///
    Value() :
        type_(ValueType::Invalid) {

    }

    /// Returns a const reference to an invalid value. This is useful for error
    /// handling in methods that must return a Value by const reference.
    ///
    static const Value& invalid();

    /// Constructs a Value holding a Color.
    ///
    Value(const core::Color& color) :
        type_(ValueType::Color),
        color_(color) {

    }

    /// Constructs a Value holding a DoubleArray.
    ///
    Value(const core::DoubleArray& doubleArray) :
        type_(ValueType::DoubleArray),
        doubleArray_(doubleArray) {

    }

    /// Constructs a Value holding a Vec2dArray.
    ///
    Value(const core::Vec2dArray& vec2dArray) :
        type_(ValueType::Vec2dArray),
        vec2dArray_(vec2dArray) {

    }

    /// Returns the ValueType of this Value.
    ///
    ValueType type() const {
        return type_;
    }

    /// Returns whether this Value is Valid, that is, whether type() is not
    /// ValueType::Invalid, which means that it does hold one of the correct
    /// values.
    ///
    bool isValid() const {
        return type() != ValueType::Invalid;
    }

    /// Stops holding any Value. This makes this Value invalid.
    ///
    void unset();

    /// Reclaims unused memory.
    ///
    void shrinkToFit();

    /// Returns the Color hold by this Value. The behavior is undefined if
    /// type() != ValueType::Color.
    ///
    core::Color getColor() const {
        return color_;
    }

    /// Returns the Color hold by this Value. The behavior is undefined if
    /// type() != ValueType::Color.
    ///
    void get(core::Color& color) const {
        color = color_;
    }

    /// Sets this Value to the given \p color.
    ///
    void set(const core::Color& color) {
        unset();
        type_ = ValueType::Color;
        color_ = color;
    }

    /// Returns the Vec2dArray hold by this Value. The behavior is undefined if
    /// type() != ValueType::Vec2dArray.
    ///
    const core::Vec2dArray& getVec2dArray() const {
        return vec2dArray_;
    }

    /// Returns the Vec2dArray hold by this Value. The behavior is undefined if
    /// type() != ValueType::Vec2dArray.
    ///
    void get(core::Vec2dArray& vec2dArray) const {
        vec2dArray = vec2dArray_;
    }

    /// Sets this value to the given \p vec2dArray.
    ///
    void set(const core::Vec2dArray& vec2dArray) {
        unset();
        type_ = ValueType::Vec2dArray;
        vec2dArray_ = vec2dArray;
    }

    /// Returns the DoubleArray hold by this Value. The behavior is undefined if
    /// type() != ValueType::DoubleArray.
    ///
    const core::DoubleArray& getDoubleArray() const {
        return doubleArray_;
    }

    /// Returns the DoubleArray hold by this Value. The behavior is undefined if
    /// type() != ValueType::DoubleArray.
    ///
    void get(core::DoubleArray& doubleArray) const {
        doubleArray = doubleArray_;
    }

    /// Sets this value to the given \p vec2dArray.
    ///
    void set(const core::DoubleArray& doubleArray) {
        unset();
        type_ = ValueType::DoubleArray;
        doubleArray_ = doubleArray;
    }

private:
    // XXX This is just a memory-wasteful temporary implementation to test the
    // rest of the architecture first. Later we'll probably use boost::variant
    // (or custom-made similar structure), then std::variant once we require
    // C++17.
    //
    ValueType type_;
    core::Color color_;
    core::DoubleArray doubleArray_;
    core::Vec2dArray vec2dArray_;
};

/// Returns a string representation of the given Value.
///
VGC_DOM_API
std::string toString(const Value& v);

/// Converts the given string into a Value. Raises vgc::dom::VgcSyntaxError if
/// the given string does not represent a Value of the given ValueType.
///
VGC_DOM_API
Value toValue(const std::string& s, ValueType t);

} // namespace dom
} // namespace vgc

#endif // VGC_DOM_VALUE_H
