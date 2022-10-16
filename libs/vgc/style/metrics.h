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

#ifndef VGC_STYLE_METRICS_H
#define VGC_STYLE_METRICS_H

#include <vgc/core/array.h>
#include <vgc/core/format.h>

#include <vgc/style/api.h>

namespace vgc::style {

/// \enum vgc::style::Metrics
/// \brief The metrics required to convert between style units
///
struct Metrics {
public:
    /// Creates a `Metrics` with a scale factor of 1.
    ///
    Metrics()
        : scaleFactor_(1) {
    }

    /// Creates a `Metrics` with the given scale factor.
    ///
    explicit Metrics(float scaleFactor)
        : scaleFactor_(scaleFactor) {
    }

    /// Returns the scale factor, that is how many `px` is there in one `dp`:
    ///
    /// ```cpp
    /// float valueInPx = metrics.scaleFactor() * valueInDp;
    /// ```
    ///
    float scaleFactor() const {
        return scaleFactor_;
    }

    /// Sets the scale factor.
    ///
    void setScaleFactor(float scaleFactor) {
        scaleFactor_ = scaleFactor;
    }

private:
    float scaleFactor_;
};

} // namespace vgc::style

#endif // VGC_STYLE_METRICS_H
