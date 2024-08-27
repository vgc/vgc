// Copyright 2024 The VGC Developers
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

#ifndef VGC_CANVAS_VIEWSETTINGS_H
#define VGC_CANVAS_VIEWSETTINGS_H

#include <vgc/canvas/api.h>
#include <vgc/canvas/displaymode.h>

namespace vgc::canvas {

/// \class vgc::canvas::ViewSettings
/// \brief Stores settings related to rendering the Workspace.
///
class VGC_CANVAS_API ViewSettings {
public:
    /// Creates a `ViewSettings` with default settings.
    ///
    ViewSettings() {
    }

    /// Returns the `DisplayMode` that should be used.
    ///
    /// The default value is `DisplayMode::Normal`.
    ///
    /// \sa `setDisplayMode()`.
    ///
    DisplayMode displayMode() const {
        return displayMode_;
    }

    /// Changes the value of `displayMode()`.
    ///
    void setDisplayMode(DisplayMode value) {
        displayMode_ = value;
    }

    /// Returns whether the wireframe mode is enabled.
    ///
    /// The default value is `false`.
    ///
    /// \sa `setWireframeMode()`.
    ///
    bool isWireframeMode() const {
        return isWireframeMode_;
    }

    /// Changes the value of `isWireframeMode()`.
    ///
    void setWireframeMode(bool isWireframeMode) {
        isWireframeMode_ = isWireframeMode;
    }

    /// Returns whether the control points of all curves are visible.
    ///
    /// The default value is `false`.
    ///
    /// \sa s`etControlPointsVisible()`.
    ///
    bool areControlPointsVisible() const {
        return areControlPointsVisible_;
    }

    /// Changes the value of `areControlPointsVisible()`.
    ///
    void setControlPointsVisible(bool areControlPointsVisible) {
        areControlPointsVisible_ = areControlPointsVisible;
    }

    /// Returns whether the IDs of objects should be displayed.
    ///
    /// The default value is `false`.
    ///
    /// \sa `setShowObjectIds()`.
    ///
    bool showObjectIds() const {
        return showObjectIds_;
    }

    /// Changes the value of `showVertexId()`.
    ///
    void setShowObjectIds(bool value) {
        showObjectIds_ = value;
    }

    /// Returns whether the two view settings are equal.
    ///
    friend bool operator==(const ViewSettings& s1, const ViewSettings& s2) {
        return s1.displayMode_ == s2.displayMode_
               && s1.isWireframeMode_ == s2.isWireframeMode_
               && s1.areControlPointsVisible_ == s2.areControlPointsVisible_
               && s1.showObjectIds_ == s2.showObjectIds_;
    }

    /// Returns whether the two view settings are different.
    ///
    friend bool operator!=(const ViewSettings& s1, const ViewSettings& s2) {
        return !(s1 == s2);
    }

private:
    DisplayMode displayMode_ = DisplayMode::Normal;
    bool isWireframeMode_ = false;
    bool areControlPointsVisible_ = false;
    bool showObjectIds_ = false;
};

} // namespace vgc::canvas

#endif // VGC_CANVAS_VIEWSETTINGS_H
