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

#include <vgc/ui/plot2d.h>

#include <vgc/core/array.h>
#include <vgc/geometry/rect2f.h>
#include <vgc/ui/strings.h>

#include <vgc/ui/internal/paintutil.h>

namespace vgc {
namespace ui {

Plot2d::Plot2d()
    : Widget()
{
    addStyleClass(strings::Plot2d);
    maxYText_ = graphics::RichText::create();
    minYText_ = graphics::RichText::create();
    minXText_ = graphics::RichText::create();
    maxXText_ = graphics::RichText::create();
    hintText_ = graphics::RichText::create();
    maxYText_->setParentStylableObject(this);
    minYText_->setParentStylableObject(this);
    minXText_->setParentStylableObject(this);
    maxXText_->setParentStylableObject(this);
    hintText_->setParentStylableObject(this);
    maxYText_->addStyleClass(core::StringId("foo"));
    minYText_->addStyleClass(core::StringId("foo"));
    minXText_->addStyleClass(core::StringId("foo"));
    maxXText_->addStyleClass(core::StringId("foo"));
    hintText_->addStyleClass(core::StringId("foo"));
}

Plot2dPtr Plot2d::create()
{
    return Plot2dPtr(new Plot2d());
}

void Plot2d::setData(core::Array<geometry::Vec2f> points)
{
    points_ = std::move(points);
    dirtyPlot_ = true;
}

void Plot2d::appendDataPoint(geometry::Vec2f point)
{
    points_.emplaceLast(point);
    dirtyPlot_ = true;
}

void Plot2d::onResize()
{
    dirtyPlot_ = true;
    dirtyBg_ = true;
    //dirtyHint_ = true;
}

void Plot2d::onPaintCreate(graphics::Engine* engine)
{
    bgGeom_ = engine->createDynamicTriangleListView(graphics::BuiltinGeometryLayout::XYRGB);
    {
        graphics::BufferPtr vertexBuffer = engine->createVertexBuffer(0);
        graphics::GeometryViewCreateInfo createInfo = {};
        createInfo.setBuiltinGeometryLayout(graphics::BuiltinGeometryLayout::XYRGB);
        createInfo.setPrimitiveType(graphics::PrimitiveType::LineList);
        createInfo.setVertexBuffer(0, vertexBuffer);
        plotGridGeom_ = engine->createGeometryView(createInfo);
    }
    {
        graphics::BufferPtr vertexBuffer = engine->createVertexBuffer(0);
        graphics::GeometryViewCreateInfo createInfo = {};
        createInfo.setBuiltinGeometryLayout(graphics::BuiltinGeometryLayout::XYRGB);
        createInfo.setPrimitiveType(graphics::PrimitiveType::LineStrip);
        createInfo.setVertexBuffer(0, vertexBuffer);
        plotGeom_ = engine->createGeometryView(createInfo);
    }
    plotTextGeom_ = engine->createDynamicTriangleListView(graphics::BuiltinGeometryLayout::XYRGB);
    hintBgGeom_ = engine->createDynamicTriangleListView(graphics::BuiltinGeometryLayout::XYRGB);
    hintTextGeom_ = engine->createDynamicTriangleListView(graphics::BuiltinGeometryLayout::XYRGB);
}

void Plot2d::onPaintDraw(graphics::Engine* engine, PaintOptions /*options*/)
{
    if (dirtyBg_ || dirtyPlot_ || dirtyHint_) {
        core::Color backgroundColor = internal::getColor(this, strings::background_color);
        core::Color hoveredBackgroundColor = internal::getColor(this, strings::background_color_on_hover);
        core::Color textColor = internal::getColor(this, strings::text_color);
        float borderRadius = internal::getLength(this, strings::border_radius);
        float paddingBottom = internal::getLength(this, strings::padding_bottom);
        float paddingLeft = internal::getLength(this, strings::padding_left);
        float paddingRight = internal::getLength(this, strings::padding_right);
        float paddingTop = internal::getLength(this, strings::padding_top);

        float yz[5] = {0, 0, 0, 0};
        float xz[4] = {0, 0, 0, 0};
        //Int hoveredIdx_ = -1;

        if (dirtyPlot_) {
            dirtyPlot_ = false;

            const float textw = 200.f;
            const float texth = 20.f;

            yz[4] = std::max(0.f, height() - paddingBottom);
            yz[0] = core::clamp(paddingTop, 0, yz[4]);
            const float yd3 = (yz[4] - yz[0]) * 0.333f;
            yz[1] = core::clamp(yz[0] + texth, yz[0], yz[0] + yd3);
            yz[3] = core::clamp(yz[4] - texth, yz[4] - yd3, yz[4]);
            yz[2] = core::clamp(yz[3] - texth, yz[0] + yd3, yz[4] - yd3);

            xz[3] = std::max(0.f, width() - paddingRight);
            xz[0] = core::clamp(paddingLeft, 0, xz[3]);
            float xd2 = (xz[3] - xz[0]) * 0.5f;
            xz[1] = core::clamp(xz[0] + textw, xz[0], xz[0] + xd2);
            xz[2] = core::clamp(xz[3] - textw, xz[3] - xd2, xz[3]);

            geometry::Rect2f hull = geometry::Rect2f::empty;
            for (geometry::Vec2f point : points_) {
                hull.uniteWith(point);
            }
            float padY = hull.height() * 0.1;
            float bX = hull.xMin();
            float bY = hull.yMin() - padY;
            float dX = hull.width();
            float dY = hull.height() + padY * 2;

            // grid
            {
                core::FloatArray a = {};
                a.extend({ std::round(xz[0]) + 0.5f, std::round(yz[0]) + 0.5f, static_cast<float>(textColor.r()), static_cast<float>(textColor.r()), static_cast<float>(textColor.r())});
                a.extend({ std::round(xz[0]) + 0.5f, std::round(yz[3]) + 0.5f, static_cast<float>(textColor.r()), static_cast<float>(textColor.r()), static_cast<float>(textColor.r())});
                a.extend({ std::round(xz[0]) + 0.5f, std::round(yz[3]) + 0.5f, static_cast<float>(textColor.r()), static_cast<float>(textColor.r()), static_cast<float>(textColor.r())});
                a.extend({ std::round(xz[3]) + 0.5f, std::round(yz[3]) + 0.5f, static_cast<float>(textColor.r()), static_cast<float>(textColor.r()), static_cast<float>(textColor.r())});
                engine->updateVertexBufferData(plotGridGeom_, std::move(a));
            }

            // text
            {
                core::FloatArray a = {};
                // maxY text
                {
                    minYText_->setRect(geometry::Rect2f(xz[0], yz[0], xz[1], yz[1]));
                    minYText_->setText(core::format("{}", bY + dY));
                    minYText_->fill(a);
                }
                // minY text
                {
                    minYText_->setRect(geometry::Rect2f(xz[0], yz[2], xz[1], yz[3]));
                    minYText_->setText(core::format("{}", bY));
                    minYText_->fill(a);
                }
                // minX text
                {
                    minYText_->setRect(geometry::Rect2f(xz[0], yz[3], xz[1], yz[4]));
                    minYText_->setText(core::format("{}", bX));
                    minYText_->fill(a);
                }
                // maxX text
                {
                    minYText_->setRect(geometry::Rect2f(xz[2], yz[3], xz[3], yz[4]));
                    minYText_->setText(core::format("{}", bX + dX));
                    minYText_->fill(a);
                }
                engine->updateVertexBufferData(plotTextGeom_, std::move(a));
            }

            core::FloatArray a(points_.size() * 5, core::NoInit{});
            Int i = 0;
            for (geometry::Vec2f point : points_) {
                a[i++] = xz[0] + (point.x() - bX) / dX * (xz[3] - xz[0]);
                a[i++] = yz[3] - (point.y() - bY) / dY * (yz[3] - yz[0]);
                a[i++] = textColor.r();
                a[i++] = textColor.g();
                a[i++] = textColor.b();
            }

            engine->updateVertexBufferData(plotGeom_, std::move(a));
        }

        if (dirtyBg_) {
            dirtyBg_ = false;
            core::FloatArray a = {};
            internal::insertRect(a, backgroundColor, 0, 0, width(), height(), borderRadius);
            internal::insertRect(a, hoveredBackgroundColor, xz[0], yz[0], xz[3], yz[3], 0.f);
            engine->updateVertexBufferData(bgGeom_, std::move(a));
        }

        if (dirtyHint_) {
            dirtyHint_ = false;
            //core::FloatArray a = {};

            //graphics::TextProperties textProperties(
            //    graphics::TextHorizontalAlign::Center,
            //    graphics::TextVerticalAlign::Middle);
            //graphics::TextCursor textCursor;
            //bool hinting = style(strings::pixel_hinting) == strings::normal;

            // todo

            //internal::insertRect(a, backgroundColor, 0, 0, width(), height(), borderRadius);
            //internal::insertText(a, textColor, 0, 0, width(), height(), 0, 0, 0, 0, text_, textProperties, textCursor, hinting);
            //engine->updateVertexBufferData(triangles_, std::move(a));
        }
    }

    engine->setProgram(graphics::BuiltinProgram::Simple);
    engine->draw(bgGeom_, -1, 0);
    engine->draw(plotGridGeom_, -1, 0);
    engine->draw(plotGeom_, -1, 0);
    engine->draw(plotTextGeom_, -1, 0);
    engine->draw(hintBgGeom_, -1, 0);
    engine->draw(hintTextGeom_, -1, 0);
}

void Plot2d::onPaintDestroy(graphics::Engine*)
{
    bgGeom_.reset();
    plotGridGeom_.reset();
    plotGeom_.reset();
    plotTextGeom_.reset();
    hintBgGeom_.reset();
    hintTextGeom_.reset();
}

bool Plot2d::onMouseMove(MouseEvent* /*event*/)
{
    return true;
}

bool Plot2d::onMousePress(MouseEvent* /*event*/)
{
    return true;
}

bool Plot2d::onMouseRelease(MouseEvent* /*event*/)
{
    return true;
}

bool Plot2d::onMouseEnter()
{
    isHovered_ = true;
    dirtyHint_ = true;
    repaint();
    return true;
}

bool Plot2d::onMouseLeave()
{
    isHovered_ = false;
    dirtyHint_ = true;
    repaint();
    return true;
}

geometry::Vec2f Plot2d::computePreferredSize() const
{
    PreferredSizeType auto_ = PreferredSizeType::Auto;
    PreferredSize w = preferredWidth();
    PreferredSize h = preferredHeight();
    geometry::Vec2f res(0, 0);
    if (w.type() == auto_) {
        res[0] = 100;
        // TODO: compute appropriate width based on data
    }
    else {
        res[0] = w.value();
    }
    if (h.type() == auto_) {
        res[1] = 40;
        // TODO: compute appropriate height based on data
    }
    else {
        res[1] = h.value();
    }
    return res;
}

} // namespace ui
} // namespace vgc
