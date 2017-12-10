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
    splines_.clear();
    Q_EMIT changed();
}

void Scene::startCurve(const geometry::Vector2d& p)
{
    splines_.push_back(geometry::BezierSpline2d());
    continueCurve(p);
}

void Scene::continueCurve(const geometry::Vector2d& p)
{
    if (splines_.size() == 0) {
        return;
    }

    std::vector<geometry::Vector2d>& d = splines_.back().data();
    if (d.size() == 0) {
        d.push_back(p);
    }
    else {
        // Note: copy of q required because push_back may invalidate ref
        geometry::Vector2d q = d.back();
        d.push_back(q + 0.33 * (p-q));
        d.push_back(q + 0.67 * (p-q));
        d.push_back(p);
    }

    Q_EMIT changed();
}

void Scene::addPoint()
{
    addPoint(geometry::Point());
}

void Scene::addPoint(const geometry::Point& point)
{
    points_.push_back(point);
    Q_EMIT changed();
}

void Scene::addPoint(double x, double y)
{
    addPoint(geometry::Point(x, y));
}

void Scene::setPoints(const std::vector<geometry::Point>& points)
{
    points_ = points;
    Q_EMIT changed();
}

} // namespace scene
} // namespace vgc
