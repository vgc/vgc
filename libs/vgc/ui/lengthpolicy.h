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

#ifndef VGC_UI_LENGTHPOLICY_H
#define VGC_UI_LENGTHPOLICY_H

#include <vgc/ui/api.h>
#include <vgc/ui/lengthtype.h>

namespace vgc {
namespace ui {

/// \class vgc::ui::LengthPolicy
/// \brief Encode whether a desired length is specified, and whether the length
/// is allowed to stretch or shrink.
///
// TODO: Should we also have a minValue and maxValue? This might (?) be useful
// if the widget is shrinkable but can't be less than a given size, or if it is
// stretchable but can't be more than a given size. Or is it overkill? For
// example, users could set the policy to Stretchable(10, LengthType::Dp, 200)
// to have a minimum value of 200dp while being able to be bigger. Is it ever
// useful to have both a maximum and minimum? Or have a desired length
// different than this minimum or maximum? Maybe. But let's wait until an
// actual use case arises before implementing it.
//
class VGC_UI_API LengthPolicy {
public:
    /// Creates a LengthPolicy with the given type, value, stretch factor, and
    /// shrink factor.
    ///
    /// Note that we also provide convenient static functions which are often
    /// more concise and readable than this constructor. We encourage you to
    /// use them:
    ///
    /// - LengthPolicy::AutoFlexible(stretch, shrink)
    /// - LengthPolicy::AutoStretchable(stretch)
    /// - LengthPolicy::AutoShrinkable(shrink)
    /// - LengthPolicy::AutoFixed()
    /// - LengthPolicy::Flexible(type, value, stretch, shrink)
    /// - LengthPolicy::Stretchable(type, value, stretch)
    /// - LengthPolicy::Shrinkable(type, value, shrink)
    /// - LengthPolicy::Fixed(type, value)
    ///
    /// In all the methods where type/value is not an available argument, then it is set to auto/0.
    ///
    /// In all the methods where stretch or shrink is not an available argument, then it is set to 0.
    ///
    /// In all the methods where stretch or shrink is an available argument, then it is optional and defaults to 1.
    ///
    /// Examples:
    ///
    /// ```cpp
    /// // A policy for a widget that can stretch and shrink, and whose default
    /// // size is automatically computed based on its content.
    /// auto p = LengthPolicy::AutoFlexible()
    ///
    /// // A policy for a widget that can stretch (but not shrink), and whose
    /// // default size is automatically computed based on its content.
    /// auto p = LengthPolicy::AutoStretchable()
    ///
    /// // A policy for a widget that can shrink (but not stretch), and whose
    /// // default size is automatically computed based on its content.
    /// auto p = LengthPolicy::AutoShrinkable()
    ///
    /// // A policy for a widget that can neither stretch or grow, and whose
    /// // default size is automatically computed based on its content.
    /// auto p = LengthPolicy::AutoFixed()
    /// ```
    ///
    LengthPolicy(LengthType type = LengthType::Auto, float value = 0,
                 float stretch = 0, float shrink = 0) :
        type_(type), value_(value), stretch_(stretch), shrink_(shrink) {

    }

    /// Creates a LengthPolicy of type LengthType::Auto with the given stretch
    /// factor and shrink factor.
    ///
    static LengthPolicy AutoFlexible(float stretch = 1, float shrink = 1) {
        return LengthPolicy(LengthType::Auto, 0, stretch, shrink);
    }

    /// Creates a LengthPolicy of type LengthType::Auto with the given stretch
    /// factor, and a shrink factor set to zero.
    ///
    static LengthPolicy AutoStretchable(float stretch = 1) {
        return LengthPolicy(LengthType::Auto, 0, stretch, 0);
    }

    /// Creates a LengthPolicy of type LengthType::Auto with the given shrink
    /// factor, and a stretch factor set to zero.
    ///
    static LengthPolicy AutoShrinkable(float shrink = 1) {
        return LengthPolicy(LengthType::Auto, 0, 0, shrink);
    }

    /// Creates a LengthPolicy of type LengthType::Auto with the shrink factor
    /// and stretch factor both set to zero.
    ///
    static LengthPolicy AutoFixed() {
        return LengthPolicy(LengthType::Auto, 0, 0, 0);
    }

    /// Creates a LengthPolicy with the given type, the given stretch factor,
    /// and the given shrink factor.
    ///
    /// This method is meant to be used for creating a LengthPolicy whose type
    /// is not Auto. If the type is auto, you may want to use AutoFlexible()
    /// instead: it is more concise and readable.
    ///
    static LengthPolicy Flexible(LengthType type, float value,
                                 float stretch = 1, float shrink = 1) {
        return LengthPolicy(type, value, stretch, shrink);
    }

    /// Creates a LengthPolicy with the given type, the given stretch factor,
    /// and a shrink factor set to zero.
    ///
    static LengthPolicy Stretchable(LengthType type, float value,
                                    float stretch = 1) {
        return LengthPolicy(type, value, stretch, 0);
    }

    /// Creates a LengthPolicy with the given type, the given shrink factor,
    /// and a stretch factor set to zero.
    ///
    static LengthPolicy Shrinkable(LengthType type, float value,
                                   float shrink = 1) {
        return LengthPolicy(type, value, 0, shrink);
    }

    /// Creates a LengthPolicy with the given type, and a shrink factor and
    /// stretch factor both set to zero.
    ///
    static LengthPolicy Fixed(LengthType type, float value) {
        return LengthPolicy(type, value, 0, 0);
    }

    /// Returns the LenghType of this LengthPolicy.
    ///
    LengthType type() const {
        return type_;
    }

    /// Sets the LenghType of this LengthPolicy.
    ///
    void setType(LengthType type) {
        type_ = type;
    }

    /// Returns the value of this LengthPolicy.
    ///
    float value() const {
        return value_;
    }

    /// Sets the value of this LengthPolicy.
    ///
    void setValue(float value) {
        value_ = value;
    }

    /// Returns the stretch factor of this LengthPolicy.
    ///
    float stretch() const {
        return stretch_;
    }

    /// Sets the stretch factor of this LengthPolicy.
    ///
    void setStretch(float stretch) {
        stretch_ = stretch;
    }

    /// Returns the shrink factor of this LengthPolicy.
    ///
    float shrink() const {
        return shrink_;
    }

    /// Sets the value of this LengthPolicy.
    ///
    void setShrink(float shrink) {
        shrink_ = shrink;
    }

private:
    LengthType type_;
    float value_;
    float stretch_;
    float shrink_;
};

} // namespace ui
} // namespace vgc

#endif // VGC_UI_LENGTHPOLICY_H
