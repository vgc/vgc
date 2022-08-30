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

#ifndef VGC_UI_GRID_H
#define VGC_UI_GRID_H

#include <vgc/core/arithmetic.h>
#include <vgc/core/array.h>
#include <vgc/geometry/range1f.h>
#include <vgc/geometry/vec2f.h>
#include <vgc/style/style.h>
#include <vgc/ui/margins.h>
#include <vgc/ui/widget.h>

namespace vgc::ui {

namespace detail {

// XXX simplest version with unexpected behavior in most non-simple cases.

namespace DirIndex {
enum Value {
    Horizontal = 0, // in x
    Vertical = 1,   // in y
};
} // namespace DirIndex

struct GridCell {
    Widget* widget = nullptr;
    Margins extraMargins;
    Margins margins;
    geometry::Rect2f borderBox;

    struct DirMetrics {
        // value is fixed
        geometry::Vec2f fixedMarginsH;
        // value is relative to border box
        geometry::Vec2f relativeMargins;
        // factor per direction: 1 / (1 - sum_of_relative_margins_in_direction)
        // multiply preferred fixed size by this to get preferred margin box
        float invRelMarginFactor = 0;

        // coefficient
        float widgetStretch = 0;
        // coefficient
        float widgetShrink = 0;
        // border box size
        float widgetPreferredSize = 0;
        float widgetMinSize = 0;

        float preferredSizeH = 0;
        float minSizeH = 0;
        float fixedMarginSizeH = 0;

        void init(const Widget* w, Int dirIndex, bool hint);
    };
    mutable std::array<DirMetrics, 2> metrics_;

    constexpr bool isStretchable(Int dirIndex) const {
        return metrics_[dirIndex].widgetStretch > 0;
    }

    constexpr bool isShrinkable(Int dirIndex) const {
        return metrics_[dirIndex].widgetShrink > 0;
    }

    constexpr void clearMetrics() const {
        metrics_[DirIndex::Horizontal] = {};
        metrics_[DirIndex::Vertical] = {};
    }

    constexpr void clear() {
        widget = nullptr;
        margins = {};
        borderBox = {};
        clearMetrics();
    }
};

struct GridTrack {
    float offsetH = 0;
    float sizeH = 0;
    float size = 0;

    struct Metrics {
        // size defined by grid-template-rows, grid-template-columns
        //              or grid-auto-rows, grid-auto-columns.
        PreferredSize customSize;

        // sizes below are cross track
        //----------------------------
        geometry::Range1f widgetPreferredSizeRange =
            geometry::Range1f(core::FloatInfinity, 0);
        geometry::Range1f widgetMinSizeRange = geometry::Range1f(core::FloatInfinity, 0);
        geometry::Range1f cellPreferredSizeRangeH =
            geometry::Range1f(core::FloatInfinity, 0);
        geometry::Range1f cellMinSizeRangeH = geometry::Range1f(core::FloatInfinity, 0);
        geometry::Range1f cellFixedMarginSizeRangeH =
            geometry::Range1f(core::FloatInfinity, 0);

        float totalWidgetStretch = 0;
        float totalWidgetShrink = 0;
        // empty cells do not count
        Int numStretchableWidgets = 0;
        // empty cells do not count
        Int numShrinkableWidgets = 0;

        // hinted if hinting is on
        float preferredSizeH = 0;
        // hinted if hinting is on
        float minSizeH = 0;
        // hinted if hinting is on
        float fixedMarginSizeH = 0;

        float stretchWeight = 0;
        float shrinkWeight = 0;

        void stepUpdate(const GridCell::DirMetrics& cellMetrics);
        void finalizeUpdate(bool hint);
    };
    mutable Metrics metrics_;

    constexpr float endOffset() const {
        return offsetH + sizeH;
    }

    constexpr bool hasStretchables() const {
        return metrics_.numStretchableWidgets > 0;
    }

    constexpr bool hasShrinkables() const {
        return metrics_.numShrinkableWidgets > 0;
    }

    float maxWidgetPreferredSize() const {
        return metrics_.widgetPreferredSizeRange.isEmpty()
                   ? 0
                   : metrics_.widgetPreferredSizeRange.pMax();
    }

    float maxWidgetMinSize() const {
        return metrics_.widgetMinSizeRange.isEmpty() ? 0
                                                     : metrics_.widgetMinSizeRange.pMax();
    }

    float maxCellPreferredSize() const {
        return metrics_.cellPreferredSizeRangeH.isEmpty()
                   ? 0
                   : metrics_.cellPreferredSizeRangeH.pMax();
    }

    float maxCellMinSize() const {
        return metrics_.cellMinSizeRangeH.isEmpty() ? 0
                                                    : metrics_.cellMinSizeRangeH.pMax();
    }

    float maxCellFixedMarginSize() const {
        return metrics_.cellFixedMarginSizeRangeH.isEmpty()
                   ? 0
                   : metrics_.cellFixedMarginSizeRangeH.pMax();
    }

    float totalWidgetStretch() const {
        return metrics_.totalWidgetStretch;
    }

    float totalWidgetShrink() const {
        return metrics_.totalWidgetShrink;
    }

    float avgWidgetStretch() const {
        return metrics_.totalWidgetStretch / metrics_.numStretchableWidgets;
    }

    float avgWidgetShrink() const {
        return metrics_.totalWidgetShrink / metrics_.numShrinkableWidgets;
    }

    float stretchWeight() const {
        return metrics_.stretchWeight;
    }

    float shrinkWeight() const {
        return metrics_.shrinkWeight;
    }

    // hinted if hinting is on
    float preferredSizeH() const {
        return metrics_.preferredSizeH;
    }

    // hinted if hinting is on
    float minSizeH() const {
        return metrics_.minSizeH;
    }

    // hinted if hinting is on
    float fixedMarginSizeH() const {
        return metrics_.fixedMarginSizeH;
    }

    constexpr void clearMetrics() const {
        metrics_.widgetPreferredSizeRange = geometry::Range1f(core::FloatInfinity, 0);
        metrics_.widgetMinSizeRange = geometry::Range1f(core::FloatInfinity, 0);
        metrics_.cellPreferredSizeRangeH = geometry::Range1f(core::FloatInfinity, 0);
        metrics_.cellMinSizeRangeH = geometry::Range1f(core::FloatInfinity, 0);
        metrics_.cellFixedMarginSizeRangeH = geometry::Range1f(core::FloatInfinity, 0);
        metrics_.totalWidgetStretch = 0;
        metrics_.totalWidgetShrink = 0;
        metrics_.numStretchableWidgets = 0;
        metrics_.numShrinkableWidgets = 0;
        metrics_.preferredSizeH = 0;
        metrics_.minSizeH = 0;
        metrics_.fixedMarginSizeH = 0;
        metrics_.stretchWeight = 0;
        metrics_.shrinkWeight = 0;
    }
};

} // namespace detail

VGC_DECLARE_OBJECT(Grid);

/// \class vgc::ui::Grid
/// \brief Arrange a sequence of widgets in rows and/or columns.
///
class VGC_UI_API Grid : public Widget {
private:
    VGC_OBJECT(Grid, Widget)

    using Cell = detail::GridCell;
    using Track = detail::GridTrack;

protected:
    /// This is an implementation details. Please use
    /// Grid::create() instead.
    ///
    Grid();

public:
    /// Creates a Row.
    ///
    static GridPtr create();

    /// Returns the current number of columns.
    ///
    Int numRows() const {
        return numRows_();
    }

    /// Returns the current number of rows.
    ///
    Int numColumns() const {
        return numCols_();
    }

    /// Adds a widget to this grid and in the cell at the `i`-th row and `j`-th column.
    ///
    void setWidgetAt(Widget* widget, Int i, Int j);

    /// Returns the widget in the cell at the `i`-th row and `j`-th column.
    ///
    Widget* widgetAt(Int i, Int j) const;

    /// Clears the cell at the `i`-th row and `j`-th column from any widget.
    ///
    WidgetPtr clearCell(Int i, Int j);

protected:
    void onWidgetAdded(Widget* child) override;
    void onWidgetRemoved(Widget* child) override;
    geometry::Vec2f computePreferredSize() const override;
    void updateChildrenGeometry() override;

private:
    // row-major storage
    core::Array<Cell> cells_;
    // [rows..., columns...]
    core::Array<Track> tracks_;
    std::array<Int, 2> numTracks_ = {};

    struct DirMetrics {
        // hinted if hinting is on
        geometry::Vec2f fixedPaddingH = {};
        PreferredSize autoSize = {};
        float gapSizeH = 0;
        float autoPreferredSizeH = 0;
    };
    mutable std::array<DirMetrics, 2> metrics_;

    Int numCols_() const {
        return numTracks_[detail::DirIndex::Horizontal];
    }

    Int numRows_() const {
        return numTracks_[detail::DirIndex::Vertical];
    }

    Cell& cellAt_(Int i, Int j) {
        return cells_[i * numCols_() + j];
    }

    const Cell& cellAt_(Int i, Int j) const {
        return cells_[i * numCols_() + j];
    }

    Track& trackAt_(Int i, Int dirIndex) {
        return tracks_[dirIndex == detail::DirIndex::Horizontal ? numRows_() + i : i];
    }

    const Track& trackAt_(Int i, Int dirIndex) const {
        return tracks_[dirIndex == detail::DirIndex::Horizontal ? numRows_() + i : i];
    }

    const Track& colAt_(Int j) const {
        return trackAt_(j, detail::DirIndex::Horizontal);
    }

    const Track& rowAt_(Int i) const {
        return trackAt_(i, detail::DirIndex::Vertical);
    }

    void erase_(Widget* widget);
    void resize_(Int numRows, Int numColumns);

    void resizeUpTo_(Int i, Int j) {
        resize_((std::max)(numRows_(), i + 1), (std::max)(numCols_(), j + 1));
    }
};

} // namespace vgc::ui

#endif // VGC_UI_GRID_H
