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

#include <vgc/core/algorithm.h>
#include <vgc/geometry/vec2d.h>

namespace vgc {
namespace geometry {

namespace {

int numSamples_(const std::vector<double>& data) {
    return data.size() / 2;
}

Vec2d position_(const std::vector<double>& data, int i) {
    return Vec2d(data[2*i], data[2*i+1]);
}

Vec2d tangent_(const std::vector<double>& data, int i) {
    int n = numSamples_(data);
    assert(n > 1);

    Vec2d res;
    if (i==0)
        res = position_(data, 1) - position_(data, 0);
    else if (i == n-1)
        res = position_(data, n-1) - position_(data, n-2);
    else
        res = position_(data, i+1) - position_(data, i-1);
    res.normalize();

    return res;
}

Vec2d normal_(const std::vector<double>& data, int i) {
    Vec2d t = tangent_(data, i);
    return Vec2d(-t[1], t[0]);
}

} // namespace

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
    widthVariability_(AttributeVariability::Constant),
    widthData_(1, constantWidth) // = vector containing a single element
{

}

double Curve::width() const
{
    return core::average(widthData_);
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

std::vector<Vec2d> Curve::triangulate() const
{
    // XXX Stupid implementation for now.
    // TODO adaptive sampling, etc.

    std::vector<Vec2d> res; // XXX Should we allow to pass this as output param to reuse capacity?

    const int n = numSamples_(positionData_);
    if (n < 2) {
        return res;
    }

    for (int i = 0; i < n; ++i)
    {
        // Get position and normal
        Vec2d normal = normal_(positionData_, i);
        Vec2d position = position_(positionData_, i);

        // Get width for this sample
        double w;
        switch(widthVariability()) {
        case AttributeVariability::Constant:
            w = widthData()[0];
            break;
        case AttributeVariability::PerSample:
            w = widthData()[i];
            break;
        }

        // Get left and right points of triangle strip
        double halfwidth = 0.5 * w;
        Vec2d leftPos  = position + halfwidth * normal;
        Vec2d rightPos = position - halfwidth * normal;

        // Add vertices to list of vertices
        res.push_back(leftPos);
        res.push_back(rightPos);
    }

    return res;
}

} // namespace geometry
} // namespace vgc
