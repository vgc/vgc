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

#include <vgc/scene/scene.h>
#include <vgc/core/colors.h>

namespace vgc {
namespace scene {

Scene::Scene() :
    Object(),
    newCurveColor_(core::colors::black),
    areSignalPaused_(false)
{

}

void Scene::clear()
{
    curves_.clear();
    emitChanged_();
}

void Scene::startCurve(const core::Vec2d& p, double width)
{
    curves_.push_back(geometry::Curve());
    curves_.back().setColor(newCurveColor_);
    continueCurve(p, width);
}

void Scene::continueCurve(const core::Vec2d& p, double width)
{
    if (curves_.size() == 0) {
        return;
    }
    curves_.back().addControlPoint(p, width);
    emitChanged_();
}

void Scene::addCurve(const geometry::Curve& curve)
{
    curves_.push_back(curve);
    emitChanged_();
}

void Scene::pauseSignals()
{
    areSignalPaused_ = true;
    numChangedEmittedDuringPause_ = 0;
}

void Scene::resumeSignals(bool aggregate)
{
    areSignalPaused_ = false;
    if (numChangedEmittedDuringPause_ > 0) {
        int n = aggregate ? 1 : numChangedEmittedDuringPause_;
        for (int i = 0; i < n; ++i) {
            emitChanged_();
        }
    }

    // Note: For now, aggregation is trivial since the only signal is a global
    // "changed". Later, signals will be more specific than this, such as
    // layerChanged(), or, cellChanged(), etc. In this case, we want to be
    // smart about aggregation to make sure that observers take into account
    // what changed without having to blow out their whole cache.
}

void Scene::emitChanged_()
{
    if (areSignalPaused_) {
        ++numChangedEmittedDuringPause_;
    }
    else {
        changed();
    }
}

} // namespace scene
} // namespace vgc
