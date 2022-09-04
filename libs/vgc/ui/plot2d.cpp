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
#include <vgc/graphics/strings.h>
#include <vgc/ui/logcategories.h>
#include <vgc/ui/strings.h>

#include <vgc/ui/detail/paintutil.h>

namespace vgc::ui {

namespace xyrgb {

inline void appendPoint(
    core::Array<float>& data,
    const geometry::Vec2f& p,
    const core::Colorf& color) {
    data.extend({p.x(), p.y(), color.r(), color.g(), color.b()});
}

void appendTriangle(
    core::Array<float>& data,
    const geometry::Vec2f& p0,
    const geometry::Vec2f& p1,
    const geometry::Vec2f& p2,
    const core::Colorf& color) {
    appendPoint(data, p0, color);
    appendPoint(data, p1, color);
    appendPoint(data, p2, color);
}

void appendLineOpaqueNoAA(
    core::Array<float>& data,
    const geometry::Vec2f& p0,
    const geometry::Vec2f& p1,
    const core::Colorf& color,
    const float width = 1.0f) {
    const float hw = width / 2;
    geometry::Vec2f v = (p1 - p0).normalized();
    geometry::Vec2f o = v.orthogonalized() * hw;
    v *= 0.5;
    //
    //     -v
    //  A---^---B
    //  |\  |   |
    //  | \P0-->|o
    //  |  \.   |
    //  |   \   |
    //  |   .\  |
    //  |  P1 \ |
    //  |   .  \|
    //  C-------D
    //
    geometry::Vec2f a = p0 - v - o;
    geometry::Vec2f b = p0 - v + o;
    geometry::Vec2f c = p1 + v - o;
    geometry::Vec2f d = p1 + v + o;
    appendTriangle(data, a, c, d, color);
    appendTriangle(data, d, b, a, color);
}

} // namespace xyrgb

Plot2d::Plot2d(Int numYs, Int maxXs)
    : Widget()
    , data_((1 + numYs) * maxXs)
    , yLabels_(numYs)
    , maxXs_(maxXs) {
    addStyleClass(strings::Plot2d);
    maxYText_ = graphics::RichText::create();
    minYText_ = graphics::RichText::create();
    maxYText_->setParentStylableObject(this);
    minYText_->setParentStylableObject(this);
    // temporary, we have to force update the cache atm
    maxYText_->addStyleClass(core::StringId("Plot2d-label-right-aligned"));
    minYText_->addStyleClass(core::StringId("Plot2d-label-right-aligned"));
}

Plot2dPtr Plot2d::create(Int numYs, Int maxXs) {
    return Plot2dPtr(new Plot2d(numYs, maxXs));
}

void Plot2d::onResize() {
    dirtyPlot_ = true;
    //dirtyHint_ = true;
}

void Plot2d::onPaintCreate(graphics::Engine* engine) {
    plotGeom_ =
        engine->createDynamicTriangleListView(graphics::BuiltinGeometryLayout::XYRGB);
    plotTextGeom_ =
        engine->createDynamicTriangleListView(graphics::BuiltinGeometryLayout::XYRGB);
    hintBgGeom_ =
        engine->createDynamicTriangleListView(graphics::BuiltinGeometryLayout::XYRGB);
    hintTextGeom_ =
        engine->createDynamicTriangleListView(graphics::BuiltinGeometryLayout::XYRGB);
}

void Plot2d::onPaintDraw(graphics::Engine* engine, PaintOptions /*options*/) {

    namespace gs = graphics::strings;

    if (dirtyPlot_ || dirtyHint_) {
        // XXX cache these
        core::Color backgroundColor = detail::getColor(this, gs::background_color);
        // TODO
        core::Color textColor = detail::getColor(this, gs::text_color);
        float paddingBottom = detail::getLength(this, gs::padding_bottom);
        float paddingLeft = detail::getLength(this, gs::padding_left);
        float paddingRight = detail::getLength(this, gs::padding_right);
        float paddingTop = detail::getLength(this, gs::padding_top);

        const Int numYs = this->numYs();
        //const Int numComponents = (1 + numYs);

        core::Array<core::Colorf> colors(numYs);
        const float hueDelta = (360.f / ((numYs + 2) / 3 * 3));
        for (Int i = 0; i < numYs; ++i) {
            int quo = 0;
            Int mod = std::remquo(i, 3, &quo);
            colors[i] = core::Colorf::hsl(60.f + quo * hueDelta + mod * 210.f, 1.f, 0.5f);
        }

        [[maybe_unused]] Int hoveredIdx = -1;

        constexpr float lblw = 80.f;
        constexpr float lblh = 20.f;

        if (dirtyPlot_) {
            dirtyPlot_ = false;

            core::FloatArray a = {};

            if (backgroundColor.a() > 0) {
                style::BorderRadiuses borderRadiuses = detail::getBorderRadiuses(this);
                detail::insertRect(a, backgroundColor, rect(), borderRadiuses);
            }

            // content only if we have data and the padded area is not empty
            if (numXs_ > 1 && (width() > paddingLeft + paddingRight)
                && (height() > paddingLeft + paddingRight)) {
                geometry::Rect2f contentRect(
                    paddingLeft,
                    paddingTop,
                    width() - paddingRight,
                    height() - paddingBottom);

                // get data hull
                // XXX impl an iterator ?
                double minX = *getXComponentPtr_(0);
                double maxX = *getXComponentPtr_(numXs_ - 1);
                double minY = 0;
                double maxY = 0;
                for (Int i = 0; i < numXs_; ++i) {
                    double* components = getXComponentPtr_(i);
                    if (isStacked_) {
                        double stackY = 0;
                        for (Int j = 1; j < 1 + numYs; ++j) {
                            stackY += components[j];
                        }
                        if (stackY < minY) {
                            minY = stackY;
                        }
                        else if (stackY > maxY) {
                            maxY = stackY;
                        }
                    }
                    else {
                        for (Int j = 1; j < 1 + numYs; ++j) {
                            double y = components[j];
                            if (y < minY) {
                                minY = y;
                            }
                            else if (y > maxY) {
                                maxY = y;
                            }
                        }
                    }
                }

                // align the hull in Y
                double deltaY = maxY - minY;
                double magY = std::pow(10., std::round(std::log10(deltaY))) / 10;
                double maxYAxis = maxY - std::remainder(maxY, magY) + magY;
                double minYAxis = minY - std::remainder(minY, magY);
                if (std::signbit(minYAxis) != std::signbit(minY)) {
                    minYAxis = 0;
                }

                double maxXAxis = maxX;
                double minXAxis = minX;

                // left labels
                if ((lblw * 2.f <= contentRect.width())
                    && (lblh * 2.f <= contentRect.width())) {
                    areLeftLabelsVisible_ = true;

                    core::FloatArray txt = {};

                    geometry::Rect2f maxYLabelRect = geometry::Rect2f::fromPositionSize(
                        contentRect.x(), contentRect.y(), lblw, lblh);
                    maxYText_->setRect(maxYLabelRect);
                    maxYText_->setText(core::format(yFormat, maxYAxis));
                    maxYText_->fill(txt);

                    geometry::Rect2f minYLabelRect = geometry::Rect2f::fromPositionSize(
                        contentRect.x(), contentRect.yMax() - lblh, lblw, lblh);
                    minYText_->setRect(minYLabelRect);
                    minYText_->setText(core::format(yFormat, minYAxis));
                    minYText_->fill(txt);

                    engine->updateVertexBufferData(plotTextGeom_, std::move(txt));

                    contentRect.setXMin(contentRect.xMin() + lblw);
                }
                else {
                    areLeftLabelsVisible_ = false;
                }

                // grid, simple background for now
                //internal::insertRect(a, hoveredBackgroundColor, contentRect.xMin(), contentRect.yMin(), contentRect.xMax(), contentRect.yMax(), 0.f);
                const float cy = contentRect.height() / (maxYAxis - minYAxis);
                const float cx = contentRect.width() / (maxXAxis - minXAxis);

                core::Colorf backgroundColorf(
                    static_cast<float>(backgroundColor.r()),
                    static_cast<float>(backgroundColor.g()),
                    static_cast<float>(backgroundColor.b()),
                    static_cast<float>(backgroundColor.a()));

                core::Colorf textColorf(
                    static_cast<float>(textColor.r()),
                    static_cast<float>(textColor.g()),
                    static_cast<float>(textColor.b()),
                    static_cast<float>(textColor.a()));

                // plots
                if (isStacked_) {
                    core::Array<float> ys(numYs * 2);
                    float* ys0 = ys.data();
                    float* ys1 = ys.data() + numYs;
                    float baseX = contentRect.xMin();
                    float baseY = contentRect.yMax();
                    float topY = contentRect.yMin();

                    auto initYs = [&](float* ysI, double* colData) {
                        ysI[0] = baseY - static_cast<float>(cy * (colData[1] - minYAxis));
                        for (Int i = 1; i < numYs; ++i) {
                            ysI[i] =
                                ysI[i - 1]
                                - static_cast<float>(cy * (colData[i + 1] - minYAxis));
                        }
                    };
                    double* colData = getXComponentPtr_(0);
                    initYs(ys0, colData);

                    float prevX =
                        baseX + static_cast<float>(cx * (colData[0] - minXAxis));

                    for (Int i = 1; i < numXs_; ++i) {
                        colData = getXComponentPtr_(i);
                        initYs(ys1, colData);
                        float x =
                            baseX + static_cast<float>(cx * (colData[0] - minXAxis));
                        //VGC_WARNING(LogVgcUi, "prevX:{} x:{}", prevX, x);
                        //
                        //  A-----B contentRect.yMax() - scaled Y
                        //  |     |
                        //  |     |
                        //  C-----D contentRect.yMax() - scaled Y+1
                        //  prevX x
                        //
                        // ACD
                        // DBA

                        for (Int j = numYs - 1; j >= 1; --j) {
                            core::Colorf color = colors[j];
                            a.extend({prevX, ys0[j], color.r(), color.g(), color.b()});
                            a.extend(
                                {prevX, ys0[j - 1], color.r(), color.g(), color.b()});
                            a.extend({x, ys1[j - 1], color.r(), color.g(), color.b()});
                            a.extend({x, ys1[j - 1], color.r(), color.g(), color.b()});
                            a.extend({x, ys1[j], color.r(), color.g(), color.b()});
                            a.extend({prevX, ys0[j], color.r(), color.g(), color.b()});
                            xyrgb::appendLineOpaqueNoAA(
                                a,
                                geometry::Vec2f(prevX, ys0[j] + 0.5f),
                                geometry::Vec2f(x, ys1[j] + 0.5f),
                                backgroundColorf,
                                1.3f);
                        }
                        core::Colorf color = colors[0];
                        a.extend({prevX, ys0[0], color.r(), color.g(), color.b()});
                        a.extend({prevX, baseY, color.r(), color.g(), color.b()});
                        a.extend({x, baseY, color.r(), color.g(), color.b()});
                        a.extend({x, baseY, color.r(), color.g(), color.b()});
                        a.extend({x, ys1[0], color.r(), color.g(), color.b()});
                        a.extend({prevX, ys0[0], color.r(), color.g(), color.b()});
                        xyrgb::appendLineOpaqueNoAA(
                            a,
                            geometry::Vec2f(prevX, ys0[0] + 0.5f),
                            geometry::Vec2f(x, ys1[0] + 0.5f),
                            backgroundColorf,
                            1.3f);

                        float midX = (prevX + x) * 0.5f;
                        if (isHovered_ && hoveredIdx == -1) {
                            if (mpos_.x() < midX) {
                                if (mpos_.x() >= prevX - 0.0001f) {
                                    if (mpos_.y() <= baseY
                                        && mpos_.y() >= topY /*ys0[numYs - 1]*/) {
                                        hoveredIdx = i - 1;
                                    }
                                }
                            }
                            else if (mpos_.x() < x) {
                                if (mpos_.y() <= baseY
                                    && mpos_.y() >= topY /*ys1[numYs - 1]*/) {
                                    hoveredIdx = i;
                                }
                            }
                        }

                        prevX = x;
                        std::swap(ys0, ys1);
                    }

                    if (hoveredIdx >= 0) {
                        colData = getXComponentPtr_(hoveredIdx);
                        float x =
                            baseX + static_cast<float>(cx * (colData[0] - minXAxis));
                        xyrgb::appendLineOpaqueNoAA(
                            a,
                            geometry::Vec2f(x, baseY),
                            geometry::Vec2f(x, topY),
                            textColorf,
                            2.4f);
                    }
                }
                else {
                    // todo
                    for (Int j = 1; j < 1 + numYs; ++j) {
                    }
                }

                engine->updateVertexBufferData(plotGeom_, std::move(a));
            }
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
    engine->draw(plotGeom_, -1, 0);
    if (areLeftLabelsVisible_) {
        engine->draw(plotTextGeom_, -1, 0);
    }
    engine->draw(hintBgGeom_, -1, 0);
    engine->draw(hintTextGeom_, -1, 0);
}

void Plot2d::onPaintDestroy(graphics::Engine*) {
    plotGeom_.reset();
    plotTextGeom_.reset();
    hintBgGeom_.reset();
    hintTextGeom_.reset();
}

bool Plot2d::onMouseMove(MouseEvent* event) {
    mpos_ = event->position();
    dirtyPlot_ = true;
    dirtyHint_ = true;
    requestRepaint();
    return true;
}

bool Plot2d::onMousePress(MouseEvent* /*event*/) {
    return true;
}

bool Plot2d::onMouseRelease(MouseEvent* /*event*/) {
    return true;
}

bool Plot2d::onMouseEnter() {
    isHovered_ = true;
    dirtyPlot_ = true;
    dirtyHint_ = true;
    requestRepaint();
    return true;
}

bool Plot2d::onMouseLeave() {
    isHovered_ = false;
    mpos_ = {0, 0};
    dirtyPlot_ = true;
    dirtyHint_ = true;
    requestRepaint();
    return true;
}

geometry::Vec2f Plot2d::computePreferredSize() const {
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

} // namespace vgc::ui
