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

// XXX This whole 'scene' library will be soon deleted; it is temporarily kept
// here for documentation of the signal mechanism which yet has to be ported to
// 'dom'

#ifndef VGC_SCENE_SCENE_H
#define VGC_SCENE_SCENE_H

#include <memory>
#include <vector>
#include <vgc/core/color.h>
#include <vgc/core/object.h>
#include <vgc/core/signal.h>
#include <vgc/core/vec2d.h>
#include <vgc/geometry/curve.h>
#include <vgc/scene/api.h>

namespace vgc {
namespace scene {

VGC_DECLARE_OBJECT(Scene);

class VGC_SCENE_API Scene : public core::Object {
private:
    VGC_OBJECT(Scene)
    VGC_PRIVATIZE_OBJECT_TREE_MUTATORS

protected:
    /// Creates a new Scene.
    ///
    Scene();

public:
    /// Clears the scene.
    ///
    void clear();

    // XXX This is a temporary test. Final API will be different
    void startCurve(const core::Vec2d& p, double width = 1.0);
    void continueCurve(const core::Vec2d& p, double width = 1.0);
    const std::vector<geometry::Curve>& curves() const {
        return curves_;
    }
    void setNewCurveColor(const core::Color& color) {
        newCurveColor_ = color;
    }

    /// Adds a curve to the Scene.
    ///
    void addCurve(const geometry::Curve& curve);

    /// This signal is emitted when the scene has changed.
    ///
    const core::Signal<> changed;

    /// Temporarily postpones the Scene::changed signals from being emitted.
    /// This may improve performance if you are planning to modify the scene
    /// many times but only need to notify the observers at the end.
    ///
    /// Note that unlike QObject::blockSignals(), the signals are buffered and
    /// still emitted when resumeSignals() is called, possibly aggregated. This
    /// means that the performance gain comes from aggretating the signals, not
    /// because they are not sent.
    ///
    /// Example:
    ///
    /// \code
    /// scene->pauseSignals();
    /// for (int i = 0; i < 10000; ++i)
    ///     scene->addCurve(makeCurve(i));
    /// scene->resumeSignals();
    /// \endcode
    ///
    /// \sa resumeSignals()
    ///
    void pauseSignals();

    /// Emits the signals that have been postponed since pauseSignals() has
    /// been called. If \p aggregate is true, then the signals are aggregated
    /// for performance. Then, resumes normal emission of signals.
    ///
    /// \sa pauseSignals()
    ///
    void resumeSignals(bool aggregate = true);

private:
    core::Color newCurveColor_;
    std::vector<geometry::Curve> curves_;

    // Signal pausing
    void emitChanged_();
    bool areSignalPaused_;
    int numChangedEmittedDuringPause_;
};

} // namespace scene
} // namespace vgc

#endif // VGC_SCENE_SCENE_H
