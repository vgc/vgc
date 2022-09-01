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

#include <vgc/ui/grid.h>

#include <algorithm>
#include <list>
#include <utility>

#include <vgc/core/arithmetic.h>
#include <vgc/graphics/strings.h>
#include <vgc/ui/detail/layoututil.h>
#include <vgc/ui/strings.h>

#include <vgc/ui/detail/paintutil.h>

namespace vgc::ui {

Grid::Grid()
    : Widget() {

    addStyleClass(strings::Grid);
}

GridPtr Grid::create() {
    return GridPtr(new Grid());
}

void Grid::setWidgetAt(Widget* widget, Int i, Int j) {
    // clear from current position if already in a cell
    if (widget) {
        erase_(widget);
        resizeUpTo_(i, j);
        cellAt_(i, j).widget = widget;
        addChild(widget);
        requestGeometryUpdate();
    }
}

Widget* Grid::widgetAt(Int i, Int j) const {
    if (i >= numRows_() || j >= numCols_()) {
        return nullptr;
    }
    return cellAt_(i, j).widget;
}

WidgetPtr Grid::clearCell(Int i, Int j) {
    WidgetPtr w = widgetAt(i, j);
    if (w) {
        // this indirectly calls erase_(w) and requestGeometryUpdate()
        // see onWidgetRemoved()
        //
        w->reparent(nullptr);
    }
    return w;
}

void Grid::onWidgetAdded(Widget*) {
}

void Grid::onWidgetRemoved(Widget* widget) {
    erase_(widget);
    requestGeometryUpdate();
}

namespace {

using Length = double;
using Number = double;

float getNum(const Widget* w, core::StringId id) {
    return static_cast<float>(w->style(id).valueOrDefault<Number>());
}

PreferredSize getPreferredSize(const Widget* w, core::StringId id) {
    return w->style(id).valueOrDefault<PreferredSize>();
}

// UI coordinates are always in pixels
// dpi-awareness means more pixels
// we have to adjust the fixed values of the stylesheet to keep a similar aspect.
// XXX we could directly scale in the style Lengths getter.

float hintSpacing(float spacing) {
    if (spacing <= 0) {
        return 0;
    }
    return (std::max)(1.f, (std::round)(spacing));
}

float getSpacing(const Widget* w, core::StringId id, bool hint) {
    style::StyleValue spacing = w->style(id);
    if (spacing.has<Length>()) {
        float value = static_cast<float>(spacing.to<Length>());
        return hint ? hintSpacing(value) : value;
    }
    return 0;
}

} // namespace

void detail::GridCell::DirMetrics::init(const Widget* w, Int dirIndex, bool hint) {
    if (!w) {
        return;
    }

    namespace gs = graphics::strings;

    using namespace detail::DirIndex;

    // fixedMargins + stretch + shrink
    if (dirIndex == Horizontal) {
        const float marginL = getSpacing(w, gs::margin_left, hint);
        const float marginR = getSpacing(w, gs::margin_right, hint);
        fixedMarginsH = {marginL, marginR};
        widgetStretch = getNum(w, strings::horizontal_stretch);
        widgetShrink = getNum(w, strings::horizontal_shrink);
    }
    else {
        const float marginT = getSpacing(w, gs::margin_top, hint);
        const float marginB = getSpacing(w, gs::margin_bottom, hint);
        fixedMarginsH = {marginT, marginB};
        widgetStretch = getNum(w, strings::vertical_stretch);
        widgetShrink = getNum(w, strings::vertical_shrink);
    }

    // relativeMargins
    // XXX set values when margins support percentages
    //     and don't forget to clamp their sum by direction to 1!
    relativeMargins = {};

    float fixedMarginSumH(fixedMarginsH[0] + fixedMarginsH[1]);
    float relativeMarginSum(relativeMargins[0] + relativeMargins[1]);

    // invRelMarginFactor
    //
    // Margins in percentages are relative to the margin box.
    // In each direction:
    //     margin_box == (border_box + fixed_margins_sum) + (margin_box * rel_margins_sum)
    // <=> margin_box == (border_box + fixed_margins_sum) * (1 / (1 - rel_margins_sum))
    //
    invRelMarginFactor = (1 / (1 - relativeMarginSum));

    // widgetPreferredSize
    widgetPreferredSize = w->preferredSize()[dirIndex];
    // widgetMinSize
    widgetMinSize = 0;
    if (widgetShrink <= 0) {
        widgetMinSize = widgetPreferredSize;
    }

    // preferredSize
    preferredSizeH = (widgetPreferredSize + fixedMarginSumH);
    preferredSizeH *= invRelMarginFactor;
    // minSize
    minSizeH = fixedMarginSumH + widgetMinSize;
    minSizeH *= invRelMarginFactor;
    // fixedMarginSize
    fixedMarginSizeH = fixedMarginSumH;
    fixedMarginSizeH *= invRelMarginFactor;

    if (hint) {
        preferredSizeH = std::ceil(preferredSizeH);
        minSizeH = std::ceil(minSizeH);
        fixedMarginSizeH = std::ceil(fixedMarginSizeH);
    }
}

void detail::GridTrack::Metrics::stepUpdate(const GridCell::DirMetrics& cellMetrics) {

    widgetPreferredSizeRange.uniteWith(cellMetrics.widgetPreferredSize);
    widgetMinSizeRange.uniteWith(cellMetrics.widgetMinSize);
    cellPreferredSizeRangeH.uniteWith(cellMetrics.preferredSizeH);
    cellMinSizeRangeH.uniteWith(cellMetrics.minSizeH);
    cellFixedMarginSizeRangeH.uniteWith(cellMetrics.fixedMarginSizeH);

    if (cellMetrics.widgetStretch > 0) {
        totalWidgetStretch += cellMetrics.widgetStretch;
        ++numStretchableWidgets;
    }

    if (cellMetrics.widgetShrink > 0) {
        totalWidgetShrink += cellMetrics.widgetShrink;
        ++numShrinkableWidgets;
    }
}

void detail::GridTrack::Metrics::finalizeUpdate(bool hint) {
    if (widgetPreferredSizeRange.isEmpty()) {
        widgetPreferredSizeRange = {};
    }
    if (widgetMinSizeRange.isEmpty()) {
        widgetMinSizeRange = {};
    }
    if (cellPreferredSizeRangeH.isEmpty()) {
        cellPreferredSizeRangeH = {};
    }
    if (cellMinSizeRangeH.isEmpty()) {
        cellMinSizeRangeH = {};
    }
    if (cellFixedMarginSizeRangeH.isEmpty()) {
        cellFixedMarginSizeRangeH = {};
    }
    if (customSize.isAuto()) {
        preferredSizeH = cellPreferredSizeRangeH.pMax();
        minSizeH = cellMinSizeRangeH.pMax();
    }
    else {
        preferredSizeH = customSize.value();
        minSizeH = preferredSizeH;
        if (hint) {
            preferredSizeH = std::ceil(preferredSizeH);
        }
    }
    fixedMarginSizeH = cellFixedMarginSizeRangeH.pMax();
    stretchWeight = totalWidgetStretch;
    shrinkWeight = totalWidgetShrink * (preferredSizeH - minSizeH);
}

geometry::Vec2f Grid::computePreferredSize() const {

    namespace gs = graphics::strings;

    using namespace strings;
    using namespace detail::DirIndex;

    const PreferredSize hPrefSize = preferredWidth();
    const PreferredSize vPrefSize = preferredHeight();
    DirMetrics& hMetrics = metrics_[Horizontal];
    DirMetrics& vMetrics = metrics_[Vertical];

    const bool hint = (style(gs::pixel_hinting) == gs::normal);

    const Int numCols = numCols_();
    const Int numRows = numRows_();

    {
        const float paddingR = getSpacing(this, gs::padding_right, hint);
        const float paddingL = getSpacing(this, gs::padding_left, hint);
        hMetrics.fixedPaddingH = {paddingL, paddingR};
        hMetrics.gapSizeH = getSpacing(this, column_gap, hint);
        hMetrics.autoSize = getPreferredSize(this, grid_auto_columns);
    }

    {
        const float paddingT = getSpacing(this, gs::padding_top, hint);
        const float paddingB = getSpacing(this, gs::padding_bottom, hint);
        vMetrics.fixedPaddingH = {paddingT, paddingB};
        vMetrics.gapSizeH = getSpacing(this, row_gap, hint);
        vMetrics.autoSize = getPreferredSize(this, grid_auto_rows);
    }

    for (Int dir = 0; dir < 2; ++dir) {
        DirMetrics& metrics = metrics_[dir];
        metrics.autoPreferredSizeH = metrics.fixedPaddingH[0] + metrics.fixedPaddingH[1];
    }

    if (numCols > 0 && numRows > 0) {

        // compute and cache all metrics
        // -----

        // clear track metrics
        for (const Track& track : tracks_) {
            track.clearMetrics();
        }

        for (Int i = 0; i < numRows; ++i) {
            const Track& row = rowAt_(i);
            Track::Metrics& rowMetrics = row.metrics_;
            // XXX support "grid-template-rows" when lists are supported by style parser
            // fallback when undefined: grid-auto-rows
            rowMetrics.customSize = vMetrics.autoSize;
            bool rowNeedsCellMetrics = rowMetrics.customSize.isAuto();

            for (Int j = 0; j < numCols; ++j) {
                const Track& col = colAt_(j);
                Track::Metrics& colMetrics = col.metrics_;
                // XXX support "grid-template-columns" when lists are supported by style parser
                // fallback when undefined: grid-auto-columns
                colMetrics.customSize = hMetrics.autoSize;
                bool colNeedsCellMetrics = colMetrics.customSize.isAuto();

                const Cell& cell = cellAt_(i, j);
                Widget* w = cell.widget;
                if (w) {
                    if (colNeedsCellMetrics) {
                        Cell::DirMetrics& chMetrics = cell.metrics_[Horizontal];
                        chMetrics.init(w, Horizontal, hint);
                        colMetrics.stepUpdate(chMetrics);
                    }
                    if (rowNeedsCellMetrics) {
                        Cell::DirMetrics& cvMetrics = cell.metrics_[Vertical];
                        cvMetrics.init(w, Vertical, hint);
                        rowMetrics.stepUpdate(cvMetrics);
                    }
                }
            }
        }

        for (const Track& track : tracks_) {
            track.metrics_.finalizeUpdate(hint);
        }

        for (Int dir = 0; dir < 2; ++dir) {
            const Int nTracks = numTracks_[dir];
            DirMetrics& metrics = metrics_[dir];
            float innerSizeH = (nTracks - 1) * metrics.gapSizeH;
            for (Int i = 0; i < nTracks; ++i) {
                const Track& track = trackAt_(i, dir);
                innerSizeH += track.preferredSizeH();
            }
            metrics.autoPreferredSizeH += innerSizeH;
        }
    }

    geometry::Vec2f res(0, 0);
    if (hPrefSize.isAuto()) {
        res[0] = hMetrics.autoPreferredSizeH;
    }
    else {
        res[0] = hPrefSize.value();
    }
    if (vPrefSize.isAuto()) {
        res[1] = vMetrics.autoPreferredSizeH;
    }
    else {
        res[1] = vPrefSize.value();
    }

    if (hint) {
        res[0] = std::ceil(res[0]);
        res[1] = std::ceil(res[1]);
    }

    VGC_ASSERT(res[0] >= 0);
    VGC_ASSERT(res[1] >= 0);

    return res;
}

void Grid::updateChildrenGeometry() {

    namespace gs = graphics::strings;

    using namespace strings;
    using namespace detail::DirIndex;

    const bool hint = (style(gs::pixel_hinting) == gs::normal);

    bool isOutOfSpace = false;

    // XXX force recompute data if dirty..
    //preferredWidth();

    for (Int dir = 0; dir < 2; ++dir) {

        float givenSizeRaw = dir == Horizontal ? width() : height();

        // XXX bug with raw size in Widget, was nan during tests
        if (std::isnan(givenSizeRaw)) {
            givenSizeRaw = 0;
        }

        const float givenSizeH = hint ? std::floor(givenSizeRaw) : givenSizeRaw;
        const Int nTracks = numTracks_[dir];
        const float gapSizeH = metrics_[dir].gapSizeH;

        const geometry::Vec2f& fixedPaddingH = metrics_[dir].fixedPaddingH;
        const float sumFixedPaddingH = fixedPaddingH[0] + fixedPaddingH[1];
        const float sumGapsH = std::max<Int>(0, nTracks - 1) * gapSizeH;
        const float fixedSizeH = sumGapsH + sumFixedPaddingH;

        if (givenSizeH <= fixedSizeH) {
            isOutOfSpace = true;
            break;
        }

        const float givenContentSizeH = givenSizeH - fixedSizeH;
        VGC_ASSERT(givenContentSizeH > 0);

        float contentPreferredSizeH = 0;
        float contentMinSizeH = 0;
        double totalStretchD = 0;
        double totalShrinkD = 0;
        for (Int i = 0; i < nTracks; ++i) {
            const Track& track = trackAt_(i, dir);
            contentPreferredSizeH += track.preferredSizeH();
            contentMinSizeH += track.minSizeH();
            totalStretchD += track.stretchWeight();
            totalShrinkD += track.shrinkWeight();
        }
        VGC_ASSERT(contentMinSizeH <= contentPreferredSizeH);

        float bonusStretchWeight = 0;
        // XXX use epsilon ?
        if (totalStretchD == 0) {
            totalStretchD = static_cast<double>(nTracks);
            bonusStretchWeight = 1.0;
        }
        if (totalShrinkD == 0) {
            totalShrinkD = 1.0;
        }

        using HinterType = detail::StretchableLayoutElementsHinter<Track*>;
        HinterType hinter;

        if (givenContentSizeH >= contentPreferredSizeH) {
            // stretch
            //--------
            const double underflowH = givenContentSizeH - contentPreferredSizeH;
            for (Int i = 0; i < nTracks; ++i) {
                Track& track = trackAt_(i, dir);
                const float preferredSizeH = track.preferredSizeH();
                const float stretchWeight = track.stretchWeight() + bonusStretchWeight;
                float stretchedSize = preferredSizeH;
                if (stretchWeight > 0) {
                    double stretchFactor = stretchWeight / totalStretchD;
                    double delta = stretchFactor * underflowH;
                    double stretchedSizeD = preferredSizeH + delta;
                    if (hint) {
                        hinter.append(&track, i, stretchedSizeD, stretchFactor);
                    }
                    stretchedSize = static_cast<float>(stretchedSizeD);
                }
                track.size = stretchedSize;
                track.sizeH = stretchedSize;
            }
        }
        else if (givenContentSizeH >= contentMinSizeH) {
            // regular shrink, use shrink weights
            // ----------------------------------
            // there must be some shrinkables if contentMinSize < contentPreferredSize
            VGC_ASSERT(totalShrinkD > 0);

            // first we must sort tracks by shrink weight
            core::Array<Int> sortedTrackIndices(nTracks); // XXX use small array
            std::iota(sortedTrackIndices.begin(), sortedTrackIndices.end(), 0);
            std::sort(
                sortedTrackIndices.begin(), sortedTrackIndices.end(), [&](Int a, Int b) {
                    Track& trackA = trackAt_(a, dir);
                    Track& trackB = trackAt_(b, dir);
                    return trackA.totalWidgetShrink() > trackB.totalWidgetShrink();
                });
            // now that they are sorted we can give them their unhinted size iteratively
            const double overflowH = contentPreferredSizeH - givenContentSizeH;
            double remainingOverflowH = overflowH;
            double remainingTotalShrinkD = totalShrinkD;
            for (Int index : sortedTrackIndices) {
                Track& track = trackAt_(index, dir);
                const float preferredSizeH = track.preferredSizeH();
                const float minSizeH = track.minSizeH();
                const float shrinkWeight = track.shrinkWeight();
                float shrinkedSize = preferredSizeH;
                bool minSizeReached = false;
                if (shrinkWeight > 0) {
                    double shrinkFactorD = shrinkWeight / remainingTotalShrinkD;
                    double deltaD = shrinkFactorD * remainingOverflowH;
                    double shrinkedSizeD = preferredSizeH - deltaD;
                    if (shrinkedSize < minSizeH) {
                        shrinkedSize = minSizeH;
                        minSizeReached = true;
                    }
                    else {
                        shrinkedSize = static_cast<float>(shrinkedSizeD);
                        if (hint) {
                            hinter.append(&track, index, shrinkedSizeD, shrinkFactorD);
                        }
                    }
                }
                track.size = shrinkedSize;
                track.sizeH = shrinkedSize;
                if (minSizeReached) {
                    float shrinkableSizeH = preferredSizeH - minSizeH;
                    remainingTotalShrinkD -= shrinkWeight;
                    remainingOverflowH -= shrinkableSizeH;
                }
            }
        }
        else {
            // emergency shrink
            VGC_ASSERT(contentMinSizeH > 0);

            const double shrinkFactorD = givenContentSizeH / contentMinSizeH;
            for (Int i = 0; i < nTracks; ++i) {
                Track& track = trackAt_(i, dir);
                const float minSizeH = track.minSizeH();
                double shrinkedSizeD = shrinkFactorD * minSizeH;
                if (hint) {
                    hinter.append(&track, i, shrinkedSizeD, shrinkFactorD);
                }
                float shrinkedSize = static_cast<float>(shrinkedSizeD);
                track.size = shrinkedSize;
                track.sizeH = shrinkedSize;
            }
        }

        // distribution of the remaining underflow pixels
        if (hint) {
            hinter.doHint();
            for (const auto& e : hinter.entries()) {
                e.elementRef()->sizeH = e.hintedSize();
            }
        }

        // compute track offsets
        float offsetH = metrics_[dir].fixedPaddingH[0];
        for (Int i = 0; i < nTracks; ++i) {
            Track& track = trackAt_(i, dir);
            track.offsetH = offsetH;
            offsetH += track.sizeH + gapSizeH;
        }
    }

    // now setup cells
    const Int numCols = numCols_();
    const Int numRows = numRows_();
    std::array<const Track*, 2> dirTracks = {};
    for (Int i = 0; i < numRows; ++i) {
        dirTracks[Vertical] = std::addressof(rowAt_(i));
        for (Int j = 0; j < numCols; ++j) {
            dirTracks[Horizontal] = std::addressof(colAt_(j));
            Cell& cell = cellAt_(i, j);
            Widget* w = cell.widget;
            if (!w) {
                continue;
            }
            if (isOutOfSpace) {
                // XXX setVisible(false) ?
                w->updateGeometry(0, 0, 0, 0);
                continue;
            }

            std::array<std::array<float, 4>, 2> dirSizes = {};
            for (Int dir = 0; dir < 2; ++dir) {
                std::array<float, 4>& sizes = dirSizes[dir];
                const Track& track = *dirTracks[dir];
                Cell::DirMetrics cellMetrics = cell.metrics_[dir];
                if (track.sizeH <= 0) {
                    // leave margins to 0
                }
                else if (track.sizeH <= cellMetrics.minSizeH) {
                    // emergency shrink
                    // cellMetrics.minSizeH > 0
                    float scaleFactor = track.sizeH / cellMetrics.minSizeH;
                    geometry::Vec2f relMargins =
                        cellMetrics.relativeMargins * track.sizeH;
                    geometry::Vec2f margins = cellMetrics.fixedMarginsH + relMargins;
                    margins *= scaleFactor;
                    if (hint) {
                        // pixelFloor to prioritize widget
                        margins[0] = std::floor(margins[0]);
                        margins[1] = std::floor(margins[1]);
                    }
                    sizes[1] = margins[0];
                    sizes[2] = margins[1];
                }
                else if (
                    track.sizeH >= cellMetrics.preferredSizeH
                    && cellMetrics.widgetStretch <= 0) {
                    // justify, currently only centering..
                    geometry::Vec2f relMargins =
                        cellMetrics.relativeMargins * track.sizeH;
                    if (hint) {
                        // pixelFloor to prioritize widget
                        relMargins[0] = std::floor(relMargins[0]);
                        relMargins[1] = std::floor(relMargins[1]);
                    }
                    geometry::Vec2f margins = cellMetrics.fixedMarginsH + relMargins;
                    float extraMarginSize = (track.sizeH - cellMetrics.preferredSizeH);
                    float extraMargin0 = extraMarginSize * 0.5f;
                    if (hint) {
                        extraMargin0 = std::floor(extraMargin0);
                    }
                    float extraMargin1 = extraMarginSize - extraMargin0;
                    // extraMargin1 >= extraMargin0
                    constexpr bool centerL = true;
                    sizes[0] = centerL ? extraMargin0 : extraMargin1;
                    sizes[1] = margins[0];
                    sizes[2] = margins[1];
                    sizes[3] = centerL ? extraMargin1 : extraMargin0;
                }
                else {
                    // shrink/stretch
                    geometry::Vec2f relMargins =
                        cellMetrics.relativeMargins * track.sizeH;
                    if (hint) {
                        // pixelFloor to prioritize widget
                        relMargins[0] = std::floor(relMargins[0]);
                        relMargins[1] = std::floor(relMargins[1]);
                    }
                    geometry::Vec2f margins = cellMetrics.fixedMarginsH + relMargins;
                    sizes[1] = margins[0];
                    sizes[2] = margins[1];
                }
            }

            // values below are already hinted if hinting is enabled
            cell.extraMargins = Margins(
                dirSizes[Vertical][0],
                dirSizes[Horizontal][3],
                dirSizes[Vertical][3],
                dirSizes[Horizontal][0]);
            cell.margins = Margins(
                dirSizes[Vertical][1],
                dirSizes[Horizontal][2],
                dirSizes[Vertical][2],
                dirSizes[Horizontal][1]);
            geometry::Rect2f cellBox = geometry::Rect2f::fromPositionSize(
                dirTracks[Horizontal]->offsetH,
                dirTracks[Vertical]->offsetH,
                dirTracks[Horizontal]->sizeH,
                dirTracks[Vertical]->sizeH);
            cell.borderBox = cellBox - (cell.extraMargins + cell.margins);

            w->updateGeometry(
                cell.borderBox.xMin(),
                cell.borderBox.yMin(),
                cell.borderBox.width(),
                cell.borderBox.height());
        }
    }
}

void Grid::erase_(Widget* widget) {
    for (Cell& cell : cells_) {
        if (cell.widget == widget) {
            cell = Cell();
            // XXX shrink array ?
        }
    }
}

void Grid::resize_(Int numRows, Int numColumns) {

    // XXX check that there are no child widgets in the removed cells!!

    const Int newNumCols = numColumns;
    const Int newNumRows = numRows;
    const Int oldNumCols = numCols_();
    const Int oldNumRows = numRows_();
    const Int numCommonCols = (std::min)(oldNumCols, newNumCols);
    const Int numCommonRows = (std::min)(oldNumRows, newNumRows);

    // resize tracks_ (rows..., columns...)
    // ------------------------------------
    Int oldNumTracks = oldNumRows + oldNumCols;
    Int newNumTracks = newNumRows + newNumCols;
    // resize array now if bigger
    if (newNumTracks > oldNumTracks) {
        tracks_.resize(newNumTracks);
    }
    // prep iterators
    using TrackIt = decltype(tracks_)::iterator;
    TrackIt tracksBeg = tracks_.begin();
    TrackIt oldRowEndIt = tracksBeg + oldNumRows;
    TrackIt newRowEndIt = tracksBeg + newNumRows;
    if (newNumRows > oldNumRows) {
        // move common columns backward
        std::move_backward(
            oldRowEndIt, oldRowEndIt + numCommonCols, newRowEndIt + numCommonCols);
        // init new rows
        std::fill(oldRowEndIt, newRowEndIt, Track());
    }
    else if (newNumRows < oldNumRows) {
        // move common columns
        std::move(oldRowEndIt, oldRowEndIt + numCommonCols, newRowEndIt);
    }
    // init new columns
    if (newNumCols > oldNumCols) {
        std::fill(newRowEndIt + oldNumCols, newRowEndIt + newNumCols, Track());
    }
    // resize array now if smaller
    if (newNumTracks < oldNumTracks) {
        tracks_.resize(newNumTracks);
    }

    // resize cells_ (row-major)
    // -------------------------
    Int oldNumCells = oldNumRows * oldNumCols;
    Int newNumCells = newNumRows * newNumCols;
    // resize array now if bigger
    if (newNumCells > oldNumCells) {
        cells_.resize(newNumCells);
    }
    // prep iterators
    using CellIt = decltype(cells_)::iterator;
    CellIt cellsBeg = cells_.begin();
    if (newNumCols > oldNumCols) {
        // iterate backward
        for (Int i = numCommonRows - 1; i >= 0; --i) {
            // move cells of common columns backward
            CellIt oldRowStart = cellsBeg + i * oldNumCols;
            CellIt newRowStart = cellsBeg + i * newNumCols;
            std::move_backward(
                oldRowStart, oldRowStart + oldNumCols, newRowStart + oldNumCols);
            // init cells of new columns
            std::fill(newRowStart + oldNumCols, newRowStart + newNumCols, Cell());
        }
    }
    else if (newNumCols < oldNumCols) {
        for (Int i = 0; i < numCommonRows; ++i) {
            // move cells of common columns
            CellIt oldRowStart = cellsBeg + i * oldNumCols;
            CellIt newRowStart = cellsBeg + i * newNumCols;
            std::move(oldRowStart, oldRowStart + newNumCols, newRowStart);
        }
    }
    // init new rows
    if (newNumRows > oldNumRows) {
        std::fill(
            cellsBeg + oldNumRows * newNumCols,
            cellsBeg + newNumRows * newNumCols,
            Cell());
    }
    // resize array now if smaller
    if (newNumCells < oldNumCells) {
        cells_.resize(newNumCells);
    }

    // update numTracks_
    // -----------------
    numTracks_[detail::DirIndex::Horizontal] = newNumCols;
    numTracks_[detail::DirIndex::Vertical] = newNumRows;
}

} // namespace vgc::ui
