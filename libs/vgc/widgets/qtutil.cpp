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

#include <vgc/widgets/qtutil.h>

#include <cmath>
#include <vgc/core/algorithm.h>

namespace {
int to256(double x) {
    int i = static_cast<int>(std::round(x * 256));
    return vgc::core::clamp(i, 0, 255);
}
} // namespace

namespace vgc {
namespace widgets {

QString toQt(const std::string& s)
{
    return QString::fromUtf8(s.data(), s.size());
}

std::string fromQt(const QString& s)
{
    QByteArray a = s.toUtf8();
    return std::string(a.data(), a.size());
}

QColor toQt(const core::Color& c)
{
    return QColor(to256(c.r()),
                  to256(c.g()),
                  to256(c.b()),
                  to256(c.a()));
}

core::Color fromQt(const QColor& c)
{
    return core::Color(c.redF(),
                       c.greenF(),
                       c.blueF(),
                       c.alphaF());
}

QPointF toQt(const core::Vec2d& v)
{
    return QPointF(v[0], v[1]);
}

QPointF toQt(const core::Vec2f& v)
{
    return QPointF(v[0], v[1]);
}

core::Vec2d fromQtd(const QPointF& v)
{
    return core::Vec2d(v.x(), v.y());
}

core::Vec2f fromQtf(const QPointF& v)
{
    return core::Vec2f(v.x(), v.y());
}

} // namespace widgets
} // namespace vgc
