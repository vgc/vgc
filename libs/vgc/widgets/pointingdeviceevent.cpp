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

#include <vgc/widgets/pointingdeviceevent.h>

#include <QGuiApplication>

#include <vgc/ui/qtutil.h>

namespace vgc::widgets {

PointingDeviceEvent::PointingDeviceEvent(QMouseEvent* event)
    : type_(event->type())
    , modifiers_(event->modifiers())
    , timestamp_(event->timestamp())
    , button_(event->button())
    , buttons_(event->buttons())
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    , pos_(ui::fromQtd(event->localPos()))
#else
    , pos_(ui::fromQtd(event->position()))
#endif
    , hasPressure_(false)
    , pressure_(0.0) {
}

PointingDeviceEvent::PointingDeviceEvent(QTabletEvent* event)
    : type_(event->type())
    , modifiers_(QGuiApplication::queryKeyboardModifiers())
    // Note: we don't use event->modifiers() or QGuiApplication::keyboardModifiers()
    // because they're broken for tablet events; at least in Qt 5.6 and Linux/X11,
    // they always returns NoModifier.
    , timestamp_(event->timestamp())
    , button_(event->button())
    , buttons_(event->buttons())
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    , pos_(ui::fromQtd(event->posF()))
#else
    , pos_(ui::fromQtd(event->position()))
#endif
    , hasPressure_(true)
    , pressure_(event->pressure()) {
}

QEvent::Type PointingDeviceEvent::type() const {
    return type_;
}

bool PointingDeviceEvent::isMouseEvent() const {
    return type_ == QEvent::MouseButtonPress //
           || type_ == QEvent::MouseMove     //
           || type_ == QEvent::MouseButtonRelease;
}

bool PointingDeviceEvent::isTabletEvent() const {
    return type_ == QEvent::TabletPress   //
           || type_ == QEvent::TabletMove //
           || type_ == QEvent::TabletRelease;
}

Qt::KeyboardModifiers PointingDeviceEvent::modifiers() const {
    return modifiers_;
}

unsigned long PointingDeviceEvent::timestamp() const {
    return timestamp_;
}

Qt::MouseButton PointingDeviceEvent::button() const {
    return button_;
}

Qt::MouseButtons PointingDeviceEvent::buttons() const {
    return buttons_;
}

geometry::Vec2d PointingDeviceEvent::pos() const {
    return pos_;
}

bool PointingDeviceEvent::hasPressure() const {
    return hasPressure_;
}

double PointingDeviceEvent::pressure() const {
    return pressure_;
}

} // namespace vgc::widgets
