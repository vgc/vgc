// Copyright 2017 The VGC Developers
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

#include <vgc/widgets/pointingdeviceevent.h>

#include <vgc/widgets/qtutil.h>

namespace vgc {
namespace widgets {

PointingDeviceEvent::PointingDeviceEvent(QMouseEvent* event) :
    type_(event->type()),
    modifiers_(event->modifiers()),
    timestamp_(event->timestamp()),
    button_(event->button()),
    buttons_(event->buttons()),
    pos_(fromQt(event->localPos())),
    hasPressure_(false),
    pressure_(0.0)
{

}

PointingDeviceEvent::PointingDeviceEvent(QTabletEvent* event) :
    type_(event->type()),
    modifiers_(event->modifiers()),
    timestamp_(event->timestamp()),
    button_(event->button()),
    buttons_(event->buttons()),
    pos_(fromQt(event->posF())),
    hasPressure_(true),
    pressure_(event->pressure())
{

}

QEvent::Type PointingDeviceEvent::type() const
{
    return type_;
}

bool PointingDeviceEvent::isMouseEvent() const
{
    return type_ == QEvent::MouseButtonPress ||
           type_ == QEvent::MouseMove ||
           type_ == QEvent::MouseButtonRelease;
}

bool PointingDeviceEvent::isTabletEvent() const
{
    return type_ == QEvent::TabletPress ||
           type_ == QEvent::TabletMove ||
           type_ == QEvent::TabletRelease;
}

Qt::KeyboardModifiers PointingDeviceEvent::modifiers() const
{
    return modifiers_;
}

unsigned long PointingDeviceEvent::timestamp() const
{
    return timestamp_;
}

Qt::MouseButton PointingDeviceEvent::button() const
{
    return button_;
}

Qt::MouseButtons PointingDeviceEvent::buttons() const
{
    return buttons_;
}

geometry::Vec2d PointingDeviceEvent::pos() const
{
    return pos_;
}

bool PointingDeviceEvent::hasPressure() const
{
    return hasPressure_;
}

double PointingDeviceEvent::pressure() const
{
    return pressure_;
}

} // namespace widgets
} // namespace vgc
