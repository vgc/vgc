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

#ifndef VGC_STYLE_TYPES_H
#define VGC_STYLE_TYPES_H

#include <vgc/core/arithmetic.h>
#include <vgc/style/api.h>
#include <vgc/style/style.h>

namespace vgc::style {

/// \enum vgc::style::LengthUnit
/// \brief The unit of a length property
///
enum class LengthUnit : UInt8 {
    Dp
};

/// \class vgc::style::Length
/// \brief A length value
///
class VGC_STYLE_API Length {
public:
    /// Constructs a length of zero dp.
    ///
    Length() {
    }

    /// Constructs a length with the given value and unit.
    ///
    Length(double value, LengthUnit unit)
        : value_(value)
        , unit_(unit) {
    }

    /// Returns the value of the length.
    ///
    double value() const {
        return value_;
    }

    /// Returns the unit of the length.
    ///
    LengthUnit unit() const {
        return unit_;
    }

    static StyleValue parse(StyleTokenIterator begin, StyleTokenIterator end);

private:
    double value_ = 0;
    LengthUnit unit_ = LengthUnit::Dp;
};

} // namespace vgc::style

#endif // VGC_STYLE_STYLE_H
