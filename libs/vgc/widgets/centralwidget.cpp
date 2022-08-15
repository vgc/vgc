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

#include <vgc/widgets/centralwidget.h>

#include <QMouseEvent>
#include <QPainter>

#include <vgc/core/algorithm.h>
#include <vgc/core/arithmetic.h>
#include <vgc/core/logging.h>
#include <vgc/widgets/logcategories.h>
#include <vgc/widgets/toggleviewaction.h>

namespace vgc::widgets {

Splitter::Splitter(
    CentralWidget* parent,
    Direction direction,
    bool isResizable,
    int length,
    int minimumLength,
    int maximumLength)

    : QWidget(parent)
    , parent_(parent)
    , direction_(direction)
    , isResizable_(isResizable)
    , length_(length)
    , minimumLength_(minimumLength)
    , maximumLength_(maximumLength)
    , centerlineStartPos_(0, 0)
    , centerlineLength_(0)
    , grabWidth_(10)
    , highlightWidth_(4)
    , highlightColor_(Qt::transparent)
    , isHovered_(false)
    , isPressed_(false) {

    setAttribute(Qt::WA_Hover);
    setCursor_();

    // Ensure that we have 0 <= min <= max.
    //
    // As a side effect, this calls setLength(length_), which ensures that
    // length_ is in the range [min, max], and calls updateGeometry_() if it
    // wasn't.
    //
    setLengthRange(minimumLength, maximumLength);

    // Compute initial geometry, if not already done by the code above.
    if (length_ == length) {
        updateGeometry_();
    }
}

void Splitter::setResizable(bool isResizable) {
    if (isResizable_ != isResizable) {
        isResizable_ = isResizable;
        setCursor_();
        updateGeometry_();
    }
}

void Splitter::setLength(int length) {
    length = core::clamp(length, minimumLength_, maximumLength_);
    if (length_ != length) {
        length_ = length;
        updateGeometry_();
    }
}

void Splitter::setMinimumLength(int min) {
    int max = maximumLength();
    if (min < 0) {
        VGC_WARNING(
            LogVgcWidgetsSplitter,
            "vgc::widgets::Splitter::setMinimumLength(min={}) "
            "called with min < 0.",
            min);
        min = 0;
    }
    else if (min > max) {
        VGC_WARNING(
            LogVgcWidgetsSplitter,
            "vgc::widgets::Splitter::setMinimumLength(min={}) "
            "called with min > maximumLength() (={}).",
            min,
            max);
        min = max;
    }
    minimumLength_ = min;
    setLength(length_);
}

void Splitter::setMaximumLength(int max) {
    int min = minimumLength();
    if (max < min) {
        VGC_WARNING(
            LogVgcWidgetsSplitter,
            "vgc::widgets::Splitter::setMaximumLength(max={}) "
            "called with max < minimumLength() (={}).",
            max,
            min);
        max = min;
    }
    maximumLength_ = max;
    setLength(length_);
}

void Splitter::setLengthRange(int min, int max) {
    minimumLength_ = min;
    maximumLength_ = max;
    if (min < 0) {
        VGC_WARNING(
            LogVgcWidgetsSplitter,
            "vgc::widgets::Splitter::setLengthRange(min={}, max={}) "
            "called with min < 0.",
            min,
            max);
        minimumLength_ = 0;
    }
    if (max < 0) {
        VGC_WARNING(
            LogVgcWidgetsSplitter,
            "vgc::widgets::Splitter::setLengthRange(min={}, max={}) "
            "called with max < 0.",
            min,
            max);
        maximumLength_ = 0;
    }
    if (min > max) {
        VGC_WARNING(
            LogVgcWidgetsSplitter,
            "vgc::widgets::Splitter::setLengthRange(min={}, max={}) "
            "called with min > max.",
            min,
            max);
        std::swap(minimumLength_, maximumLength_);
    }
    setLength(length_);
}

void Splitter::setGrabWidth(int width) {
    grabWidth_ = width;
    if (grabWidth_ < 0) {
        grabWidth_ = 0;
    }
    if (grabWidth_ < highlightWidth_) {
        highlightWidth_ = grabWidth_;
    }
}

void Splitter::setHighlightWidth(int width) {
    highlightWidth_ = width;
    if (highlightWidth_ < 0) {
        highlightWidth_ = 0;
    }
    if (grabWidth_ < highlightWidth_) {
        grabWidth_ = highlightWidth_;
    }
}

void Splitter::setHighlightColor(QColor color) {
    highlightColor_ = color;
}

void Splitter::setGeometryFromCenterline(int x, int y, int l) {
    centerlineStartPos_ = QPoint(x, y);
    centerlineLength_ = l;
    updateGeometry_();
}

void Splitter::updateGeometry_() {
    if (isResizable()) {
        int x = centerlineStartPos_.x();
        int y = centerlineStartPos_.y();
        int l = centerlineLength_;

        // Separate widths into half-widths
        int hw1 = highlightWidth_ / 2;
        int hw2 = highlightWidth_ - hw1;
        int gw1 = grabWidth_ / 2;
        int gw2 = grabWidth_ - gw1;

        // Set the geometry of the splitter to the grab area
        if (orientation() == Qt::Horizontal) {
            setGeometry(x - gw1, y, grabWidth_, l);
        }
        else {
            setGeometry(x, y - gw1, l, grabWidth_);
        }

        // Mask the area of the splitter which is never drawn
        bool hasMask = grabWidth_ > highlightWidth_;
        setAttribute(Qt::WA_MouseNoMask, hasMask);
        if (hasMask) {
            if (orientation() == Qt::Horizontal) {
                setContentsMargins(gw1 - hw1, 0, gw2 - hw2, 0);
            }
            else {
                setContentsMargins(0, gw1 - hw1, 0, gw2 - hw2);
            }
            setMask(QRegion(contentsRect()));
        }
        else {
            setContentsMargins(0, 0, 0, 0);
            clearMask();
        }
    }
    else {
        setGeometry(0, 0, 0, 0);
        setAttribute(Qt::WA_MouseNoMask, false);
        clearMask();
    }
}

bool Splitter::event(QEvent* event) {
    switch (event->type()) {
    case QEvent::HoverEnter:
        isHovered_ = true;
        update();
        break;
    case QEvent::HoverLeave:
        isHovered_ = false;
        update();
        break;
    default:
        break;
    }
    return QWidget::event(event);
}

void Splitter::paintEvent(QPaintEvent*) {
    QPainter p(this);
    if (isHovered_) {
        p.fillRect(contentsRect(), highlightColor_);
    }
}

void Splitter::mousePressEvent(QMouseEvent* e) {
    if (e->button() == Qt::LeftButton) {
        isPressed_ = true;
        lengthOnPress_ = length_;
        zOnPress_ = z_(e->pos());
        update();
    }
}

void Splitter::mouseMoveEvent(QMouseEvent* e) {
    if (e->buttons() & Qt::LeftButton) {
        int offset = z_(e->pos()) - zOnPress_;
        if (direction_ == Direction::Right || direction_ == Direction::Bottom) {
            length_ = lengthOnPress_ + offset;
        }
        else {
            length_ = lengthOnPress_ - offset;
        }
        length_ = vgc::core::clamp(length_, minimumLength_, maximumLength_);
        parent_->updateGeometries_();
    }
}

void Splitter::mouseReleaseEvent(QMouseEvent* e) {
    if (e->button() == Qt::LeftButton) {
        isPressed_ = false;
        update();
    }
}

void Splitter::setCursor_() {
    if (isResizable()) {
        setCursor(orientation() == Qt::Horizontal ? Qt::SplitHCursor : Qt::SplitVCursor);
    }
    else {
        setCursor(Qt::ArrowCursor);
    }
}

CentralWidget::CentralWidget(
    QWidget* viewer,
    QWidget* toolbar,
    QWidget* console,
    QWidget* parent)

    : QWidget(parent)
    , viewer_(viewer)
    , toolbar_(toolbar)
    , console_(console)
    , panelArea_(new PanelArea(this))
    , margin_(0) {

    viewer_->setParent(this);
    toolbar->setParent(this);
    console->setParent(this);

    consoleToggleViewAction_ = new ToggleViewAction(
        tr("Python Console"),
        console_,
        this); // TODO: set "Python Console" text somewhere else
    connect(
        consoleToggleViewAction_, SIGNAL(toggled(bool)), this, SLOT(updateGeometries_()));

    connect(
        panelArea_,
        &PanelArea::visibleToParentChanged,
        this,
        &CentralWidget::updateGeometries_);

    // Create splitters, which handle resize mouse events.
    //
    // Note: We would prefer not having to create child widgets just for that,
    // but unfortunately this is the only reasonable way to capture mouse
    // events before they are captured by the other child widgets. Indeed,
    // child widgets decide whether to propagate events to parent widgets, not
    // the other way around.
    //
    // A possible workaround not involving the creation of splitter widgets
    // would be to enable mouse tracking and install an event filter on all the
    // descendant widgets of the CentralWidget. Unfortunately, this is
    // error-prone since new widgets may be created dynamically: how to ensure
    // that those widgets have the event filter installed? Possibly, we could
    // create a global event filter, checking if the event is a MouseMove, then
    // checking if the object is a descendant of CentralWidget, but this seems
    // costly, not elegant at all, and it does not solve the problem of setting
    // mouse tracking on all descendants.
    //
    // The current implementation is similar to how QSplitterHandle does it.
    //
    // Besides, even if we did manage to capture the mouse events without
    // creating an additional widget, another problem is that a parent widget
    // cannot draw over its child widgets (it can only draw below it). See:
    //
    // https://www.qtcentre.org/threads/232-Drawing-over-content-widgets-(overlay)
    // https://forum.qt.io/topic/67555/qwidget-paint-event-draw-over-children
    //
    // Therefore, we couldn't draw the splitter highlight color. This is quite
    // a serious limitation, which I believe is a design decision made by Qt
    // for performance reasons: no need to redraw the parent widget when a
    // child widget changes. However, in an app heavily GPU-accelerated anyway,
    // I think it makes more sense to just redraw the whole interface all the
    // time anyway (possibly rendering expected-not-to-change-often things in
    // framebuffers), which make it much more flexible to draw overlays.
    // Overlays could be especially useful for a "Tutorial" mode, or other
    // shadow/highlight effects.
    //
    splitters_.push_back(new Splitter(this, Splitter::Direction::Right, false, 150, 150));
    splitters_.push_back(new Splitter(this, Splitter::Direction::Left, true, 200, 200));
    splitters_.push_back(new Splitter(this, Splitter::Direction::Top, true, 200, 50));

    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    updateGeometries_();
}

CentralWidget::~CentralWidget() {
}

QSize CentralWidget::sizeHint() const {
    return QSize(1920, 1080);
}

QSize CentralWidget::minimumSizeHint() const {
    QSize res = QSize(2 * margin_, 2 * margin_);
    res += viewer_->minimumSizeHint();
    if (toolbar_->isVisibleTo(this)) {
        res += QSize(margin_ + splitters_[0]->minimumLength(), 0);
    }
    if (panelArea_->isVisibleTo(this)) {
        res += QSize(margin_ + splitters_[1]->minimumLength(), 0);
    }
    if (console_->isVisibleTo(this)) {
        res += QSize(0, margin_ + splitters_[2]->minimumLength());
    }
    return res;
}

Panel* CentralWidget::addPanel(const QString& title, QWidget* widget) {
    Panel* panel = panelArea_->addPanel(title, widget);
    return panel;
}

Panel* CentralWidget::panel(QWidget* widget) {
    return panelArea_->panel(widget);
}

void CentralWidget::resizeEvent(QResizeEvent*) {
    updateGeometries_();
}

void CentralWidget::updateGeometries_() {
    int M = margin_;
    int m1 = M / 2;
    int m2 = M - m1;

    int h = height();
    int w = width();

    int x1 = m1;
    int x4 = w - m2;
    int y1 = m1;
    int y3 = h - m2;

    // Splitter between toolbar and viewer/console
    int x2 = x1;
    Splitter* s0 = splitters_[0];
    if (toolbar_->isVisibleTo(this)) {
        x2 += M + s0->length();
        s0->setGeometryFromCenterline(x2, y1 + m2, y3 - y1 - M);
        s0->show();
    }
    else {
        s0->hide();
    }

    // Splitter between viewer/console and panels
    int x3 = x4;
    Splitter* s1 = splitters_[1];
    if (panelArea_->isVisibleTo(this)) {
        x3 -= M + s1->length();
        s1->setGeometryFromCenterline(x3, y1 + m2, y3 - y1 - M);
        s1->show();
    }
    else {
        s1->hide();
    }

    // Splitter between viewer and console
    int y2 = y3;
    Splitter* s2 = splitters_[2];
    if (console_->isVisibleTo(this)) {
        y2 -= M + s2->length();
        s2->setGeometryFromCenterline(x2 + m2, y2, x3 - x2 - M);
        s2->show();
    }
    else {
        s2->hide();
    }

    // Set maximum sizes. We need to run the setMaximumLength() functions two
    // times to converge to a solution. If we don't, we end up in an incorrect
    // state when making the right sidepanel visible while the window was at
    // its then-minimum size.
    QSize vMinSize = viewer_->minimumSizeHint();
    for (int i = 0; i < 2; ++i) {
        if (toolbar_->isVisibleTo(this)) {
            int max = x3 - x1 - 2 * M - vMinSize.width();
            max = core::clamp(max, s0->minimumLength(), core::tmax<int>);
            s0->setMaximumLength(max);
        }
        if (panelArea_->isVisibleTo(this)) {
            int max = x4 - x2 - 2 * M - vMinSize.width();
            max = core::clamp(max, s1->minimumLength(), core::tmax<int>);
            s1->setMaximumLength(max);
        }
        if (console_->isVisibleTo(this)) {
            int max = y3 - y1 - 2 * M - vMinSize.height();
            max = core::clamp(max, s2->minimumLength(), core::tmax<int>);
            s2->setMaximumLength(max);
        }
    }

    // Set geometry of actual useful widgets
    toolbar_->setGeometry(x1 + m2, y1 + m2, x2 - x1 - M, y3 - y1 - M);
    viewer_->setGeometry(x2 + m2, y1 + m2, x3 - x2 - M, y2 - y1 - M);
    console_->setGeometry(x2 + m2, y2 + m2, x3 - x2 - M, y3 - y2 - M);
    panelArea_->setGeometry(x3 + m2, y1 + m2, x4 - x3 - M, y3 - y1 - M);

    // Make sure that the window minimum size is increased
    // when making a new panel visible.
    updateGeometry();
}

} // namespace vgc::widgets
