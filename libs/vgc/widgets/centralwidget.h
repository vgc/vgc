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

#ifndef VGC_WIDGETS_CENTRALWIDGET_H
#define VGC_WIDGETS_CENTRALWIDGET_H

#include <vector>

#include <QWidget>

#include <vgc/widgets/api.h>
#include <vgc/widgets/panel.h>
#include <vgc/widgets/panelarea.h>

namespace vgc::widgets {

class CentralWidget;

/// \class vgc::widgets::Splitter
/// \brief The resize handles between the child widgets of a CentralWidget.
///
class VGC_WIDGETS_API Splitter : public QWidget {
private:
    Q_OBJECT

    Q_PROPERTY(int grabWidth READ grabWidth WRITE setGrabWidth DESIGNABLE true)
    Q_PROPERTY(
        int highlightWidth READ highlightWidth WRITE setHighlightWidth DESIGNABLE true)
    Q_PROPERTY(
        QColor highlightColor READ highlightColor WRITE setHighlightColor DESIGNABLE true)

public:
    /// \enum vgc::widgets::Splitter::Direction
    /// \brief The four possible directions of a Splitter.
    ///
    enum class Direction {
        Right,
        Left,
        Bottom,
        Top
    };

    /// Constructs a Splitter.
    ///
    Splitter(
        CentralWidget* parent,
        Direction direction,
        bool isResizable,
        int length,
        int minimumLength = 50,
        int maximumLength = 400);

    /// Returns the direction of this splitter, that is, the direction in which
    /// the splitter goes when length() increases.
    ///
    Direction direction() const {
        return direction_;
    }

    /// Returns whether this splitter is resizable.
    ///
    bool isResizable() const {
        return isResizable_;
    }

    /// Sets whether this splitter is resizable.
    ///
    void setResizable(bool isResizable);

    /// Returns the length of the splitter, that is:
    ///
    /// - if direction() = Right: the desired width of the widget on the left of this splitter
    /// - if direction() = Left: the desired width of the widget on the right of this splitter
    /// - if direction() = Bottom: the desired height of the widget above this splitter
    /// - if direction() = Top: the desired height of the widget below this splitter
    ///
    int length() const {
        return length_;
    }

    /// Sets the length of the splitter.
    ///
    void setLength(int length);

    /// Returns the minimum allowed length.
    ///
    int minimumLength() const {
        return minimumLength_;
    }

    /// Sets the minimum allowed length.
    ///
    void setMinimumLength(int min);

    /// Returns the maximum allowed length.
    ///
    int maximumLength() const {
        return maximumLength_;
    }

    /// Sets the maximum allowed length.
    ///
    void setMaximumLength(int max);

    /// Sets the minimum and maximum allowed length.
    ///
    void setLengthRange(int min, int max);

    /// Returns the width within which the handle can be grabbed.
    ///
    int grabWidth() const {
        return grabWidth_;
    }

    /// Sets the width within which the handle can be grabbed. This may be
    /// larger than the actual width between the panels. The default is 10, to
    /// ensure that the handle can be easily grabbed even using a pen tablet.
    ///
    void setGrabWidth(int width);

    /// Returns the width used for highlighting the handle.
    ///
    int highlightWidth() const {
        return highlightWidth_;
    }

    /// Sets the width used for highlighting the handle. The default is 4.
    /// Currently, we only support highlightWidth <= grabWidth.
    ///
    void setHighlightWidth(int width);

    /// Returns the color used for highlighting the handle.
    ///
    QColor highlightColor() const {
        return highlightColor_;
    }

    /// Sets the color used for highlighting the handle. The default is
    /// transparent (which means that the handle is never actually visible, but
    /// users can still see when it is hovered due to the change of cursor).
    ///
    void setHighlightColor(QColor color);

    /// Sets the geometry of the splitter from its centerline, given by its
    /// starting point (x, y) and length l.
    ///
    void setGeometryFromCenterline(int x, int y, int l);

protected:
    /// Reimplements QWidget::event().
    ///
    bool event(QEvent* event) override;

    /// Reimplements QWidget::paintEvent().
    ///
    void paintEvent(QPaintEvent* event) override;

    /// Reimplements QWidget::mousePressEvent().
    ///
    void mousePressEvent(QMouseEvent* event) override;

    /// Reimplements QWidget::mouseMoveEvent().
    ///
    void mouseMoveEvent(QMouseEvent* event) override;

    /// Reimplements QWidget::mouseReleaseEvent().
    ///
    void mouseReleaseEvent(QMouseEvent* event) override;

private:
    CentralWidget* parent_;
    Direction direction_;
    Qt::Orientation orientation() const {
        return (direction_ == Direction::Left || direction_ == Direction::Right)
                   ? Qt::Horizontal
                   : Qt::Vertical;
    }
    bool isResizable_;
    void setCursor_();
    int length_;
    int minimumLength_;
    int maximumLength_;
    QPoint centerlineStartPos_;
    int centerlineLength_;
    void updateGeometry_();
    int grabWidth_;
    int highlightWidth_;
    QColor highlightColor_;
    bool isHovered_;
    bool isPressed_;
    int lengthOnPress_;
    int zOnPress_;
    int z_(const QPoint& pos) const {
        QPoint q = mapToParent(pos);
        return orientation() == Qt::Horizontal ? q.x() : q.y();
    }
};

/// \class vgc::widgets::CentralWidget
/// \brief The central widget of the MainWindow, providing toolbars and docks.
///
class VGC_WIDGETS_API CentralWidget : public QWidget {
private:
    Q_OBJECT

public:
    /// Constructs a CentralWidget.
    ///
    explicit CentralWidget(
        QWidget* viewer,
        QWidget* toolbar,
        QWidget* console,
        QWidget* parent = nullptr);

    /// Destructs the CentralWidget.
    ///
    ~CentralWidget() override;

    /// Reimplements QWidget::sizeHint().
    ///
    QSize sizeHint() const override;

    /// Reimplements QWidget::minimumSizeHint().
    ///
    QSize minimumSizeHint() const override;

    /// Returns the toggle view action for the console.
    ///
    QAction* consoleToggleViewAction() const {
        return consoleToggleViewAction_;
    }

    /// Adds a Panel wrapping the given widget.
    ///
    Panel* addPanel(const QString& title, QWidget* widget);

    /// Returns the panel wrapping the given widget, or nullptr if not found.
    ///
    Panel* panel(QWidget* widget);

protected:
    /// Reimplements QWidget::resizeEvent().
    ///
    void resizeEvent(QResizeEvent* event) override;

private Q_SLOTS:
    void updateGeometries_();

private:
    // Ad-hoc widgets and sizes for now. Will change to more generic
    // splitting mechanism later.
    QWidget* viewer_;
    QWidget* toolbar_;
    QWidget* console_;
    PanelArea* panelArea_;
    QAction* consoleToggleViewAction_;

    // Splitters
    int margin_;
    friend class Splitter;
    std::vector<Splitter*> splitters_;
};

} // namespace vgc::widgets

#endif // VGC_WIDGETS_CENTRALWIDGET_H
