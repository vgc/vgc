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

#ifndef VGC_UI_DETAIL_LAYOUTUTIL_H
#define VGC_UI_DETAIL_LAYOUTUTIL_H

#include <algorithm>
#include <cmath>
#include <utility>

#include <vgc/core/arithmetic.h>
#include <vgc/core/array.h>
#include <vgc/core/assert.h>

namespace vgc::ui::detail {

template<typename TElementRef>
struct StretchableLayoutElementsHinter {
public:
    struct ElementEntry {
    public:
        friend StretchableLayoutElementsHinter;
        friend struct ElementEntryIncCostLess;

        ElementEntry(TElementRef&& ref, Int index, double inputSize, float pace)
            : elementRef_(std::move(ref))
            , elementIndex_(index)
            , pace_(pace)
            , inputSize_(inputSize) {
        }

        const TElementRef& elementRef() const {
            return elementRef_;
        }

        Int elementIndex() const {
            return elementIndex_;
        }

        double inputSize() const {
            return inputSize_;
        }

        float hintedSize() const {
            return static_cast<float>(size_);
        }

    protected:
        TElementRef elementRef_;
        Int elementIndex_ = 0;
        float pace_ = 0;
        Int32 size_ = 0;
        Int incCost_ = 0;
        Int incCostStep_ = 0;
        double inputSize_ = 0;
    };

    template<typename... Args>
    void append(
        TElementRef elementRef,
        Int elementIndex,
        double stretchedSize,
        float stretchFactor) {

        const float pace = stretchFactor > 0 ? 1.f / stretchFactor : core::tmax<float>;
        entries_.emplaceLast(std::move(elementRef), elementIndex, stretchedSize, pace);
    }

    void doHint(bool allowSizeWobbling = false) {

        if (entries_.empty()) {
            return;
        }

        // size-wobbling is when elements occasionally get smaller while increasing
        // the shared space. It is a better choice when there is a wide element
        // next to a lot of smaller ones.
        //
        if (allowSizeWobbling) {
            // this is a simple algorithm with a max size-wobbling of 1px per element.
            // (align from nonHinted to nearest pixel)
            double hintedSum = 0;
            double inputSum = 0;
            for (ElementEntry& e : entries_) {
                inputSum += e.inputSize_;
                double newHintedSum = std::round(inputSum);
                double hintedSize = newHintedSum - hintedSum;
                hintedSum = newHintedSum;
                e.size_ = static_cast<Int>(hintedSize);
            }
            return;
        }

        // compute the minimum pace to normalize them in the range [1, +inf)
        float minPace = core::tmax<float>;
        for (ElementEntry& e : entries_) {
            if (e.pace_ < minPace) {
                minPace = e.pace_;
            }
        }

        Int iPacePrecision = 32;

        Int floorSum = 0;
        double inputSizeSum = 0;
        for (ElementEntry& e : entries_) {

            double normalizedPace = e.pace_ / minPace;
            double sizeFloored = std::floor(e.inputSize_);
            Int iSizeFloored = static_cast<Int>(sizeFloored);
            double distToNextPix = 1.0 - (e.inputSize_ - sizeFloored);
            double costToNextPix = (distToNextPix * normalizedPace) * iPacePrecision;
            Int iCostToNextPix = static_cast<Int>(std::round(costToNextPix));
            double costForOnePix = normalizedPace * iPacePrecision;
            Int iCostForOnePix = static_cast<Int>(std::round(costForOnePix));

            e.incCost_ = iCostToNextPix;
            e.incCostStep_ = iCostForOnePix;
            e.size_ = iSizeFloored;

            floorSum += e.size_;
            inputSizeSum += e.inputSize_;
        }

        Int pixelUnderflow = static_cast<Int>(std::round(inputSizeSum)) - floorSum;
        VGC_ASSERT(pixelUnderflow <= static_cast<Int>(entries_.size()));
        if (pixelUnderflow <= 0) {
            return;
        }

        std::sort(entries_.begin(), entries_.end(), ElementEntryIncCostLess());

        while (pixelUnderflow > 0) {
            ElementEntry& e = entries_[0];
            e.size_++;
            e.incCost_ += e.incCostStep_;

            // already sorted except beg
            auto beg = entries_.begin();
            auto end = entries_.end();
            auto start = beg + 1;
            auto it = std::upper_bound(start, end, e, ElementEntryIncCostLess());
            if (it != start) {
                std::rotate(beg, start, it);
            }

            --pixelUnderflow;
        }
    }

    template<typename Compare>
    void sort(Compare compare) {
        std::sort(entries_.begin(), entries_.end(), std::forward<Compare>(compare));
    }

    const core::Array<ElementEntry>& entries() const {
        return entries_;
    }

protected:
    struct ElementEntryIncCostLess {
        bool operator()(const ElementEntry& a, const ElementEntry& b) {
            return (a.incCost_ < b.incCost_)
                   || (!(b.incCost_ < a.incCost_) && a.elementIndex_ < b.elementIndex_);
        };
    };

private:
    core::Array<ElementEntry> entries_;
};

} // namespace vgc::ui::detail

#endif // VGC_UI_DETAIL_LAYOUTUTIL_H
