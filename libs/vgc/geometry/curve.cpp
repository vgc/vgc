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

#include <vgc/geometry/curve.h>
#include <vgc/geometry/vec2d.h>

namespace vgc {
namespace geometry {

Curve::Curve(Type type) :
    type_(type),
    positionData_(),
    widthVariability_(AttributeVariability::PerSample),
    widthData_()
{

}

Curve::Curve(double constantWidth, Type type) :
    type_(type),
    positionData_(),
    widthVariability_(AttributeVariability::PerSample),
    widthData_(1, constantWidth) // = vector containing a single element
{

}

void Curve::addSample(double x, double y)
{
    // Set position
    positionData_.push_back(x);
    positionData_.push_back(y);

    // Set width
    if (widthVariability() == AttributeVariability::PerSample) {
        double width = 1.0;
        if (widthData_.size() > 0) {
            width = widthData_.back();
        }
        widthData_.push_back(width);
    }
}

void Curve::addSample(const Vec2d& position)
{
    addSample(position.x(), position.y());
}

void Curve::addSample(double x, double y, double width)
{
    // Set position
    positionData_.push_back(x);
    positionData_.push_back(y);

    // Set width
    if (widthVariability() == AttributeVariability::PerSample) {
        widthData_.push_back(width);
    }
}

void Curve::addSample(const Vec2d& position, double width)
{
    addSample(position.x(), position.y(), width);
}

} // namespace geometry
} // namespace vgc
