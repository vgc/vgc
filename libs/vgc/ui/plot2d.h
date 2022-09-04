// Copyright 2022 The VGC Developers
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

#ifndef VGC_UI_PLOT2D_H
#define VGC_UI_PLOT2D_H

#include <vgc/core/array.h>
#include <vgc/core/color.h>
#include <vgc/geometry/rect2f.h>
#include <vgc/geometry/vec2d.h>
#include <vgc/graphics/richtext.h>
#include <vgc/ui/widget.h>

namespace vgc::ui {

VGC_DECLARE_OBJECT(Plot2d);

enum class Plot2dValueType {
    FloatingPoint,
    Integer
};

/// \class vgc::ui::Plot2d
/// \brief A Plot2d widget.
///
class VGC_UI_API Plot2d : public Widget {
private:
    VGC_OBJECT(Plot2d, Widget)

protected:
    /// This is an implementation details.
    /// Please use Plot2d::create(text) instead.
    ///
    Plot2d(Int numYs, Int maxXs);

public:
    /// Creates a Plot2d.
    ///
    static Plot2dPtr create(Int numYs = 0, Int maxXs = 100);

    bool isStacked() const {
        return isStacked_;
    }

    void setStacked(bool isStacked) {
        if (isStacked_ != isStacked) {
            isStacked_ = isStacked;
            dirtyPlot_ = true;
            dirtyHint_ = true;
        }
    }

    Int numYs() const {
        return yLabels_.size();
    }

    //void setYs(std::initializer_list<std::string>);

    void setNumYs(Int numYs) {
        const Int numComponents = (1 + numYs);
        data_.resize(maxXs_ * numComponents);
        yLabels_.resize(numYs);
        numXs_ = 0;
        firstX_ = 0;
        dirtyPlot_ = true;
        dirtyHint_ = true;
    }

    void setYLabel(Int index, std::string_view label) {
        if (index >= numYs()) {
            // XXX warning ?
            setNumYs(index + 1);
        }
        yLabels_[index] = label;
        dirtyHint_ = true;
    }

    // precision -1 means "auto"
    void setYUnit(std::string_view unit, Int precision = -1) {
        yUnit = unit;
        yFormat = precision >= 0 ? core::format("{{:.{}f}}", precision) : "{}";
        dirtyHint_ = true;
    }

    Int numXs() const {
        return numXs_;
    }

    Int maxXs() const {
        return maxXs_;
    }

    void setMaxXs(Int maxXs) {
        if (maxXs == maxXs_) {
            return;
        }
        const Int numYs = this->numYs();
        const Int numComponents = (1 + numYs);
        if (firstX_ != 0) {
            std::rotate(
                data_.begin(), data_.begin() + firstX_ * numComponents, data_.end());
        }
        data_.resize(maxXs * numComponents);
        maxXs_ = maxXs;
        if (maxXs < numXs_) {
            numXs_ = maxXs;
        }
    }

    template<typename... Ys>
    void appendDataPoint(double x, Ys&&... ys) {
        // XXX should check that every y fits in double precision
        appendDataPoint_(x, {static_cast<double>(ys)...});
        dirtyPlot_ = true;
        dirtyHint_ = true;
        requestRepaint();
    }

    // reimpl
    void onResize() override;
    void onPaintCreate(graphics::Engine* engine) override;
    void onPaintDraw(graphics::Engine* engine, PaintOptions options) override;
    void onPaintDestroy(graphics::Engine* engine) override;
    bool onMouseMove(MouseEvent* event) override;
    bool onMousePress(MouseEvent* event) override;
    bool onMouseRelease(MouseEvent* event) override;
    bool onMouseEnter() override;
    bool onMouseLeave() override;

protected:
    geometry::Vec2f computePreferredSize() const override;

private:
    core::Array<double> data_;
    core::Array<std::string> yLabels_;
    std::string yUnit = "";
    std::string yFormat = "{}";
    Int firstX_ = 0;
    Int numXs_ = 0;
    Int maxXs_ = 0;
    bool isStacked_ = true;

    graphics::GeometryViewPtr plotGeom_;
    graphics::GeometryViewPtr plotTextGeom_;
    graphics::GeometryViewPtr hintBgGeom_;
    graphics::GeometryViewPtr hintTextGeom_;
    bool areLeftLabelsVisible_ = false;
    bool dirtyPlot_ = false;
    bool dirtyHint_ = false;
    bool isHovered_ = false;
    geometry::Vec2f mpos_;
    graphics::RichTextPtr maxYText_;
    graphics::RichTextPtr minYText_;
    core::Array<graphics::RichTextPtr> hintTexts_;

    constexpr Int toInternalIndex_(Int i) const {
        return (firstX_ + i) % maxXs_;
    }

    double* getXComponentPtr_(Int index) {
        return data_.data() + (toInternalIndex_(index) * (1 + numYs()));
    }

    double* getXComponentPtrInternal_(Int internalIndex) {
        return data_.data() + (internalIndex * (1 + numYs()));
    }

    void appendDataPoint_(double x, std::initializer_list<double> ys) {
        if (maxXs_ == 0) {
            return;
        }
        const Int appendX = toInternalIndex_(numXs_);
        const Int numYs = this->numYs();
        double* pc = getXComponentPtrInternal_(appendX);
        *pc++ = x;
        Int yCount = static_cast<Int>(ys.size());
        // XXX check ys.size() <= numYs
        yCount = (std::min)(numYs, yCount);
        const double* py = ys.begin();
        for (Int i = 0; i < yCount; ++i) {
            *pc++ = *py++;
        }
        for (Int i = yCount; i < numYs; ++i) {
            *pc++ = 0;
        }
        if (numXs_ == maxXs_) {
            // overwrote oldest point
            VGC_ASSERT(appendX == firstX_);
            ++firstX_;
        }
        else {
            ++numXs_;
        }
    }
};

} // namespace vgc::ui

#endif // VGC_UI_BUTTON_H
