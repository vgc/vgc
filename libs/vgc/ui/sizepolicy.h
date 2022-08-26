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

#ifndef VGC_UI_SIZEPOLICY_H
#define VGC_UI_SIZEPOLICY_H

#include <vgc/core/arithmetic.h>
#include <vgc/ui/api.h>

namespace vgc::ui {

/// \enum vgc::ui::PreferredSizeType
/// \brief Encode whether a PreferredSize is "auto", and if not, what unit is used.
///
// TODO: support "Percentage" and all the dimension units of Android, they are great:
// https://developer.android.com/guide/topics/resources/more-resources.html#Dimension
//
// Could it also be useful to have max-content, min-content, or fit-content from CSS?
// https://developer.mozilla.org/en-US/docs/Web/CSS/width
//
enum class PreferredSizeType {
    Auto,
    Dp
};

/// \class vgc::ui:::PreferredSize
/// \brief Encode the value of "preferred-width" or "preferred-height".
///
class VGC_UI_API PreferredSize {
public:
    /// Creates a PreferredSize with the given type and value.
    ///
    PreferredSize(PreferredSizeType type = PreferredSizeType::Auto, float value = 0)
        : type_(type)
        , value_(value) {
    }

    /// Returns the PreferredSizeType of this PreferredSize.
    ///
    PreferredSizeType type() const {
        return type_;
    }

    /// Sets the PreferredSizeType of this PreferredSize.
    ///
    void setType(PreferredSizeType type) {
        type_ = type;
    }

    /// Returns the value of this PreferredSize.
    ///
    float value() const {
        return value_;
    }

    /// Sets the value of this PreferredSize.
    ///
    void setValue(float value) {
        value_ = value;
    }

    /// Returns true if the PreferredSizeType of this PreferredSize is
    /// PreferredSizeType::Auto.
    ///
    bool isAuto() const {
        return type_ == PreferredSizeType::Auto;
    }

    /// Returns whether the two PreferredSize are equal.
    ///
    /// Two PreferredSize are considered equal if and only if:
    /// 1. they have the same type, and
    /// 2. if the type is not Auto, they have the same value.
    ///
    /// In particular, note that no unit conversion in performed to determined
    /// equality.
    ///
    friend bool operator==(const PreferredSize& p1, const PreferredSize& p2);

    /// Returns whether the two SizePolicy are different.
    ///
    friend bool operator!=(const PreferredSize& p1, const PreferredSize& p2);

private:
    PreferredSizeType type_;
    float value_;
};

inline bool operator==(const PreferredSize& p1, const PreferredSize& p2) {
    // XXX Should we instead use exact equality? And implement a separate isNear() method?
    const float eps = 1e-6f;
    return p1.type() == p2.type()
           && (p1.type() == PreferredSizeType::Auto
               || core::isNear(p1.value(), p2.value(), eps));
}

inline bool operator!=(const PreferredSize& p1, const PreferredSize& p2) {
    return !(p1 == p2);
}

/// \class vgc::ui::SizePolicy
/// \brief Encode a preferred size (possibly auto), and whether the size
/// is allowed to stretch or shrink.
///
// TODO: Should we also have a minValue and maxValue? This might (?) be useful
// if the widget is shrinkable but can't be less than a given size, or if it is
// stretchable but can't be more than a given size. Or is it overkill? For
// example, users could set the policy to Stretchable(10, PreferredSizeType::Dp, 200)
// to have a minimum value of 200dp while being able to be bigger. Is it ever
// useful to have both a maximum and minimum? Or have a desired length
// different than this minimum or maximum? Maybe. But let's wait until an
// actual use case arises before implementing it.
//
class VGC_UI_API SizePolicy {
public:
    /// Creates a SizePolicy with the given type, value, stretch factor, and
    /// shrink factor.
    ///
    /// Note that we also provide convenient static functions which are often
    /// more concise and readable than this constructor. We encourage you to
    /// use them:
    ///
    /// - SizePolicy::AutoFlexible(stretch, shrink)
    /// - SizePolicy::AutoStretchable(stretch)
    /// - SizePolicy::AutoShrinkable(shrink)
    /// - SizePolicy::AutoFixed()
    /// - SizePolicy::Flexible(type, value, stretch, shrink)
    /// - SizePolicy::Stretchable(type, value, stretch)
    /// - SizePolicy::Shrinkable(type, value, shrink)
    /// - SizePolicy::Fixed(type, value)
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
    /// auto p = SizePolicy::AutoFlexible()
    ///
    /// // A policy for a widget that can stretch (but not shrink), and whose
    /// // default size is automatically computed based on its content.
    /// auto p = SizePolicy::AutoStretchable()
    ///
    /// // A policy for a widget that can shrink (but not stretch), and whose
    /// // default size is automatically computed based on its content.
    /// auto p = SizePolicy::AutoShrinkable()
    ///
    /// // A policy for a widget that can neither stretch or grow, and whose
    /// // default size is automatically computed based on its content.
    /// auto p = SizePolicy::AutoFixed()
    /// ```
    ///
    SizePolicy(
        PreferredSizeType type = PreferredSizeType::Auto,
        float value = 0,
        float stretch = 0,
        float shrink = 0)

        : preferred_(type, value)
        , stretch_(stretch)
        , shrink_(shrink) {
    }

    /// Creates a SizePolicy of type PreferredSizeType::Auto with the given stretch
    /// factor and shrink factor.
    ///
    static SizePolicy AutoFlexible(float stretch = 1, float shrink = 1) {
        return SizePolicy(PreferredSizeType::Auto, 0, stretch, shrink);
    }

    /// Creates a SizePolicy of type PreferredSizeType::Auto with the given stretch
    /// factor, and a shrink factor set to zero.
    ///
    static SizePolicy AutoStretchable(float stretch = 1) {
        return SizePolicy(PreferredSizeType::Auto, 0, stretch, 0);
    }

    /// Creates a SizePolicy of type PreferredSizeType::Auto with the given shrink
    /// factor, and a stretch factor set to zero.
    ///
    static SizePolicy AutoShrinkable(float shrink = 1) {
        return SizePolicy(PreferredSizeType::Auto, 0, 0, shrink);
    }

    /// Creates a SizePolicy of type PreferredSizeType::Auto with the shrink factor
    /// and stretch factor both set to zero.
    ///
    static SizePolicy AutoFixed() {
        return SizePolicy(PreferredSizeType::Auto, 0, 0, 0);
    }

    /// Creates a SizePolicy with the given type, the given stretch factor,
    /// and the given shrink factor.
    ///
    /// This method is meant to be used for creating a SizePolicy whose type
    /// is not Auto. If the type is auto, you may want to use AutoFlexible()
    /// instead: it is more concise and readable.
    ///
    static SizePolicy
    Flexible(PreferredSizeType type, float value, float stretch = 1, float shrink = 1) {
        return SizePolicy(type, value, stretch, shrink);
    }

    /// Creates a SizePolicy with the given type, the given stretch factor,
    /// and a shrink factor set to zero.
    ///
    static SizePolicy
    Stretchable(PreferredSizeType type, float value, float stretch = 1) {
        return SizePolicy(type, value, stretch, 0);
    }

    /// Creates a SizePolicy with the given type, the given shrink factor,
    /// and a stretch factor set to zero.
    ///
    static SizePolicy Shrinkable(PreferredSizeType type, float value, float shrink = 1) {
        return SizePolicy(type, value, 0, shrink);
    }

    /// Creates a SizePolicy with the given type, and a shrink factor and
    /// stretch factor both set to zero.
    ///
    static SizePolicy Fixed(PreferredSizeType type, float value) {
        return SizePolicy(type, value, 0, 0);
    }

    /// Returns the PreferredSize of this SizePolicy.
    ///
    PreferredSize preferredSize() const {
        return preferred_;
    }

    /// Sets the PreferredSize of this SizePolicy.
    ///
    void setPreferredSize(PreferredSize preferred) {
        preferred_ = preferred;
    }

    /// Returns the PreferredSizeType of this SizePolicy.
    ///
    PreferredSizeType preferredSizeType() const {
        return preferred_.type();
    }

    /// Sets the PreferredSizeType of this SizePolicy.
    ///
    void setPreferredSizeType(PreferredSizeType type) {
        preferred_.setType(type);
    }

    /// Returns the PreferredSize's value of this SizePolicy.
    ///
    float preferredSizeValue() const {
        return preferred_.value();
    }

    /// Sets the PreferredSize's value of this SizePolicy.
    ///
    void setPreferredSizeValue(float value) {
        preferred_.setValue(value);
    }

    /// Returns the stretch factor of this SizePolicy.
    ///
    float stretch() const {
        return stretch_;
    }

    /// Sets the stretch factor of this SizePolicy.
    ///
    void setStretch(float stretch) {
        stretch_ = stretch;
    }

    /// Returns the shrink factor of this SizePolicy.
    ///
    float shrink() const {
        return shrink_;
    }

    /// Sets the value of this SizePolicy.
    ///
    void setShrink(float shrink) {
        shrink_ = shrink;
    }

    /// Returns whether the two SizePolicy are equal.
    ///
    friend bool operator==(const SizePolicy& p1, const SizePolicy& p2);

    /// Returns whether the two SizePolicy are different.
    ///
    friend bool operator!=(const SizePolicy& p1, const SizePolicy& p2);

private:
    PreferredSize preferred_;
    float stretch_;
    float shrink_;
};

inline bool operator==(const SizePolicy& p1, const SizePolicy& p2) {
    // XXX Should we instead use exact equality? And implement a separate isNear() method?
    const float eps = 1e-6f;
    return p1.preferredSize() == p2.preferredSize()
           && core::isNear(p1.stretch(), p2.stretch(), eps)
           && core::isNear(p1.shrink(), p2.shrink(), eps);
}

inline bool operator!=(const SizePolicy& p1, const SizePolicy& p2) {
    return !(p1 == p2);
}

} // namespace vgc::ui

#endif // VGC_UI_SIZEPOLICY_H
