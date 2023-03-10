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

#include <vgc/ui/qtutil.h>

#include <cmath>

#include <QGuiApplication>
#include <QMouseEvent>
#include <QTabletEvent>

#include <vgc/core/algorithm.h>
#include <vgc/ui/keyevent.h>
#include <vgc/ui/mouseevent.h>

namespace vgc::ui {

QString toQt(std::string_view s) {
    int size = vgc::core::int_cast<int>(s.size());
    return QString::fromUtf8(s.data(), size);
}

std::string fromQt(const QString& s) {
    QByteArray a = s.toUtf8();
    size_t size = vgc::core::int_cast<size_t>(s.size());
    return std::string(a.data(), size);
}

QColor toQt(const core::Color& c) {
    return QColor(
        core::Color::mapTo255(c.r()),
        core::Color::mapTo255(c.g()),
        core::Color::mapTo255(c.b()),
        core::Color::mapTo255(c.a()));
}

core::Color fromQt(const QColor& c) {
    return core::Color(
        static_cast<float>(c.redF()),
        static_cast<float>(c.greenF()),
        static_cast<float>(c.blueF()),
        static_cast<float>(c.alphaF()));
}

QPointF toQt(const geometry::Vec2d& v) {
    return QPointF(v[0], v[1]);
}

QPointF toQt(const geometry::Vec2f& v) {
    return QPointF(v[0], v[1]);
}

geometry::Vec2d fromQtd(const QPointF& v) {
    return geometry::Vec2d(static_cast<double>(v.x()), static_cast<double>(v.y()));
}

geometry::Vec2d fromQtd(const QPoint& v) {
    return geometry::Vec2d(static_cast<double>(v.x()), static_cast<double>(v.y()));
}

geometry::Vec2f fromQtf(const QPointF& v) {
    return geometry::Vec2f(static_cast<float>(v.x()), static_cast<float>(v.y()));
}

geometry::Vec2f fromQtf(const QPoint& v) {
    return geometry::Vec2f(static_cast<float>(v.x()), static_cast<float>(v.y()));
}

Qt::KeyboardModifiers toQt(const ui::ModifierKeys& modifierKeys) {
    Qt::KeyboardModifiers modifiers = Qt::NoModifier;
    if (modifierKeys.has(ui::ModifierKey::Shift)) {
        modifiers.setFlag(Qt::ShiftModifier);
    }
    if (modifierKeys.has(ui::ModifierKey::Ctrl)) {
        modifiers.setFlag(Qt::ControlModifier);
    }
    if (modifierKeys.has(ui::ModifierKey::Alt)) {
        modifiers.setFlag(Qt::AltModifier);
    }
    if (modifierKeys.has(ui::ModifierKey::Meta)) {
        modifiers.setFlag(Qt::MetaModifier);
    }
    return modifiers;
}

ModifierKeys fromQt(const Qt::KeyboardModifiers& modifiers) {
    ModifierKeys modifierKeys;
    if (modifiers.testFlag(Qt::ShiftModifier)) {
        modifierKeys.set(ModifierKey::Shift);
    }
    if (modifiers.testFlag(Qt::ControlModifier)) {
        modifierKeys.set(ModifierKey::Ctrl);
    }
    if (modifiers.testFlag(Qt::AltModifier)) {
        modifierKeys.set(ModifierKey::Alt);
    }
    if (modifiers.testFlag(Qt::MetaModifier)) {
        modifierKeys.set(ModifierKey::Meta);
    }
    return modifierKeys;
}

MouseEventPtr fromQt(QMouseEvent* event) {

    // Button
    Qt::MouseButton qbutton = event->button();
    MouseButton button = static_cast<MouseButton>(qbutton);

    // Position
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    const QPointF& p = event->localPos();
#else
    const QPointF& p = event->position();
#endif

    // Modidier keys
    ModifierKeys modifierKeys = fromQt(event->modifiers());

    return MouseEvent::create(button, fromQtf(p), modifierKeys);
}

MouseEventPtr fromQt(QTabletEvent* event) {

    // Button
    Qt::MouseButton qbutton = event->button();
    MouseButton button = static_cast<MouseButton>(qbutton);

    // Position
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    const QPointF& p = event->posF();
#else
    const QPointF& p = event->position();
#endif

    // Modidier keys
    // Note: we don't use event->modifiers() or QGuiApplication::keyboardModifiers()
    // because they're broken for tablet events; at least in Qt 5.6 and Linux/X11,
    // they always returns NoModifier.
    ModifierKeys modifierKeys = fromQt(QGuiApplication::queryKeyboardModifiers());

    // Pressure
    double pressure = event->pressure();

    return MouseEvent::create(button, fromQtf(p), modifierKeys, 0, pressure, true);
}

KeyEventPtr fromQt(QKeyEvent* event) {

    // Key
    int qkey = event->key();
    Key key = static_cast<Key>(qkey);

    // Text
    std::string text = event->text().toStdString();

    // Modidier keys
    ModifierKeys modifierKeys = fromQt(event->modifiers());

    return KeyEvent::create(key, std::move(text), modifierKeys);
}

// clang-format off

QMatrix4x4 toQt(const geometry::Mat4f& m) {
    return QMatrix4x4(
        m(0,0), m(0,1), m(0,2), m(0,3),
        m(1,0), m(1,1), m(1,2), m(1,3),
        m(2,0), m(2,1), m(2,2), m(2,3),
        m(3,0), m(3,1), m(3,2), m(3,3));
}

QMatrix4x4 toQt(const geometry::Mat4d& m) {
    return QMatrix4x4(
        static_cast<float>(m(0,0)), static_cast<float>(m(0,1)), static_cast<float>(m(0,2)), static_cast<float>(m(0,3)),
        static_cast<float>(m(1,0)), static_cast<float>(m(1,1)), static_cast<float>(m(1,2)), static_cast<float>(m(1,3)),
        static_cast<float>(m(2,0)), static_cast<float>(m(2,1)), static_cast<float>(m(2,2)), static_cast<float>(m(2,3)),
        static_cast<float>(m(3,0)), static_cast<float>(m(3,1)), static_cast<float>(m(3,2)), static_cast<float>(m(3,3)));
}

// clang-format on

} // namespace vgc::ui
