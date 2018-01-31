// Copyright 2017 The VGC Developers
// See the COPYRIGHT file at the top-level directory of this distribution
// and at https://github.com/vgc.io/vgc/blob/master/COPYRIGHT
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

namespace vgc {
namespace scene {

Scene::Scene()
{

}

void Scene::clear()
{
    curves_.clear();
    Q_EMIT changed();
}

void Scene::startCurve(const geometry::Vec2d& p, double width)
{
    curves_.push_back(geometry::Curve());
    continueCurve(p, width);
}

void Scene::continueCurve(const geometry::Vec2d& p, double width)
{
    if (curves_.size() == 0) {
        return;
    }
    curves_.back().addControlPoint(p, width);
    Q_EMIT changed();
}

} // namespace scene
} // namespace vgc
