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

#include <vgc/vacomplex/keyedgedata.h>

#include <vgc/vacomplex/detail/operationsimpl.h>
#include <vgc/vacomplex/keyedge.h>
#include <vgc/vacomplex/logcategories.h>

namespace vgc::vacomplex {

KeyEdgeData::KeyEdgeData(KeyEdge* owner, detail::KeyEdgePrivateKey) noexcept
    : CellData(owner) {
}

KeyEdgeData::KeyEdgeData(const KeyEdgeData& other)
    : CellData(other)
    , isClosed_(other.isClosed_) {

    if (other.stroke_) {
        stroke_ = other.stroke_->clone();
    }
}

KeyEdgeData::KeyEdgeData(KeyEdgeData&& other) noexcept
    : CellData(std::move(other))
    , isClosed_(other.isClosed_) {

    stroke_ = std::move(other.stroke_);
}

KeyEdgeData& KeyEdgeData::operator=(const KeyEdgeData& other) {
    if (&other != this) {
        CellData::operator=(other);
        isClosed_ = other.isClosed_;
        if (!other.stroke_) {
            stroke_.reset();
        }
        else if (!stroke_ || !stroke_->copyAssign(other.stroke_.get())) {
            stroke_ = other.stroke_->clone();
        }
    }
    return *this;
}

KeyEdgeData& KeyEdgeData::operator=(KeyEdgeData&& other) noexcept {
    if (&other != this) {
        CellData::operator=(std::move(other));
        isClosed_ = other.isClosed_;
        stroke_ = std::move(other.stroke_);
    }
    return *this;
}

std::unique_ptr<KeyEdgeData> KeyEdgeData::clone() const {
    return std::make_unique<KeyEdgeData>(*this);
}

KeyEdge* KeyEdgeData::keyEdge() const {
    Cell* cell = properties_.cell();
    return cell ? cell->toKeyEdge() : nullptr;
}

void KeyEdgeData::translate(const geometry::Vec2d& delta) {
    if (stroke_) {
        stroke_->translate(delta);
        emitGeometryChanged();
    }
    properties_.onTranslateGeometry(delta);
}

void KeyEdgeData::transform(const geometry::Mat3d& transformation) {
    if (stroke_) {
        stroke_->transform(transformation);
        emitGeometryChanged();
    }
    properties_.onTransformGeometry(transformation);
}

void KeyEdgeData::snap(
    const geometry::Vec2d& snapStartPosition,
    const geometry::Vec2d& snapEndPosition,
    geometry::CurveSnapTransformationMode mode) {

    if (stroke_) {
        if (stroke_->snap(snapStartPosition, snapEndPosition, mode)) {
            emitGeometryChanged();
            properties_.onUpdateGeometry(stroke_.get());
        }
    }
}

const geometry::AbstractStroke2d* KeyEdgeData::stroke() const {
    return stroke_.get();
}

namespace {

void fixStrokeClosedness(geometry::AbstractStroke2d* stroke, bool isEdgeClosed) {
    if (stroke && stroke->isClosed() != isEdgeClosed) {
        std::string_view strokeType;
        std::string_view edgeType;
        if (isEdgeClosed) {
            stroke->close(false);
            strokeType = "an open stroke";
            edgeType = "a closed edge";
        }
        else {
            stroke->open(true);
            strokeType = "a closed stroke";
            edgeType = "an open edge";
        }
        VGC_WARNING(
            LogVgcVacomplex, "Assigning {} to {} caused implicit closedness conversion.");
    }
}

} // namespace

void KeyEdgeData::setStroke(const geometry::AbstractStroke2d* newStroke) {
    if (!newStroke) {
        stroke_.reset();
    }
    else if (newStroke == stroke_.get()) {
        return;
    }
    else if (!stroke_ || !stroke_->copyAssign(newStroke)) {
        stroke_ = newStroke->clone();
    }
    fixStrokeClosedness(stroke_.get(), isClosed());
    emitGeometryChanged();
    properties_.onUpdateGeometry(stroke_.get());
}

void KeyEdgeData::setStroke(std::unique_ptr<geometry::AbstractStroke2d>&& newStroke) {
    stroke_ = std::move(newStroke);
    fixStrokeClosedness(stroke_.get(), isClosed());
    emitGeometryChanged();
    properties_.onUpdateGeometry(stroke_.get());
}

//
// IDEA: do conversion to common best stroke geometry to merge
//       then match cell properties by pairs (use null if not present)

/* static */
std::unique_ptr<KeyEdgeData> KeyEdgeData::fromConcatStep(
    const KeyHalfedgeData& khd1,
    const KeyHalfedgeData& khd2,
    bool smoothJoin) {

    KeyEdgeData* ked1 = khd1.edgeData();
    KeyEdgeData* ked2 = khd2.edgeData();
    VGC_ASSERT(ked1 != nullptr && ked2 != nullptr);

    const geometry::AbstractStroke2d* st1 = ked1->stroke();
    const geometry::AbstractStroke2d* st2 = ked2->stroke();
    const geometry::StrokeModelInfo& model1 = st1->modelInfo();
    const geometry::StrokeModelInfo& model2 = st2->modelInfo();

    std::unique_ptr<geometry::AbstractStroke2d> converted;
    if (model1.name() != model2.name()) {
        if (model1.defaultConversionRank() >= model2.defaultConversionRank()) {
            converted = st1->convert(st2);
            st2 = converted.get();
        }
        else {
            converted = st2->convert(st1);
            st1 = converted.get();
        }
    }
    std::unique_ptr<geometry::AbstractStroke2d> concatStroke = st1->cloneEmpty();
    concatStroke->assignFromConcat(
        st1, khd1.direction(), st2, khd2.direction(), smoothJoin);

    auto result = std::make_unique<KeyEdgeData>(ked1->isClosed());
    result->setStroke(std::move(concatStroke));
    result->properties_.assignFromConcatStep(khd1, khd2);
    return result;
}

void KeyEdgeData::finalizeConcat() {
    properties_.finalizeConcat();
}

/* static */
std::unique_ptr<KeyEdgeData>
KeyEdgeData::fromGlueOpen(core::ConstSpan<KeyHalfedgeData> khds) {

    struct ConvertedStroke {
        std::unique_ptr<geometry::AbstractStroke2d> converted;
        const geometry::AbstractStroke2d* st;
    };
    core::Array<ConvertedStroke> converteds;

    // Find best model first.
    const geometry::AbstractStroke2d* bestModelStroke = khds[0].edgeData()->stroke();
    core::Array<bool> directions;
    directions.reserve(khds.length());
    for (const KeyHalfedgeData& khd : khds) {
        const geometry::AbstractStroke2d* st = khd.edgeData()->stroke();
        const geometry::StrokeModelInfo& model2 = st->modelInfo();
        if (model2.name() != bestModelStroke->modelInfo().name()) {
            if (model2.defaultConversionRank()
                > bestModelStroke->modelInfo().defaultConversionRank()) {
                bestModelStroke = khd.edgeData()->stroke();
            }
        }
        converteds.emplaceLast().st = st;
        directions.append(khd.direction());
    }

    core::Array<const geometry::AbstractStroke2d*> strokes;
    for (ConvertedStroke& converted : converteds) {
        if (converted.st->modelInfo().name() != bestModelStroke->modelInfo().name()) {
            converted.converted = bestModelStroke->convert(converted.st);
            converted.st = converted.converted.get();
        }
        strokes.append(converted.st);
    }

    std::unique_ptr<geometry::AbstractStroke2d> gluedStroke =
        bestModelStroke->cloneEmpty();

    gluedStroke->assignFromAverageOpen(strokes, directions);

    return fromGlue(khds, std::move(gluedStroke));
}

/* static */
std::unique_ptr<KeyEdgeData> KeyEdgeData::fromGlueClosed(
    core::ConstSpan<KeyHalfedgeData> khds,
    core::ConstSpan<double> uOffsets) {

    struct ConvertedStroke {
        std::unique_ptr<geometry::AbstractStroke2d> converted;
        const geometry::AbstractStroke2d* st;
    };
    core::Array<ConvertedStroke> converteds;

    // Find best model first.
    const geometry::AbstractStroke2d* bestModelStroke = khds[0].edgeData()->stroke();
    core::Array<bool> directions;
    directions.reserve(khds.length());
    for (const KeyHalfedgeData& khd : khds) {
        const geometry::AbstractStroke2d* st = khd.edgeData()->stroke();
        const geometry::StrokeModelInfo& model2 = st->modelInfo();
        if (model2.name() != bestModelStroke->modelInfo().name()) {
            if (model2.defaultConversionRank()
                > bestModelStroke->modelInfo().defaultConversionRank()) {
                bestModelStroke = khd.edgeData()->stroke();
            }
        }
        converteds.emplaceLast().st = st;
        directions.append(khd.direction());
    }

    core::Array<const geometry::AbstractStroke2d*> strokes;
    for (ConvertedStroke& converted : converteds) {
        if (converted.st->modelInfo().name() != bestModelStroke->modelInfo().name()) {
            converted.converted = bestModelStroke->convert(converted.st);
            converted.st = converted.converted.get();
        }
        strokes.append(converted.st);
    }

    std::unique_ptr<geometry::AbstractStroke2d> gluedStroke =
        bestModelStroke->cloneEmpty();

    gluedStroke->assignFromAverageClosed(strokes, directions, uOffsets);

    return fromGlue(khds, std::move(gluedStroke));
}

/* static */
std::unique_ptr<KeyEdgeData> KeyEdgeData::fromGlue(
    core::ConstSpan<KeyHalfedgeData> khds,
    std::unique_ptr<geometry::AbstractStroke2d>&& gluedStroke) {

    auto result = std::make_unique<KeyEdgeData>(gluedStroke->isClosed());
    result->setStroke(std::move(gluedStroke));
    result->properties_.glue(khds, result->stroke());
    return result;
}

std::unique_ptr<KeyEdgeData> KeyEdgeData::fromSlice(
    const KeyEdgeData* ked,
    const geometry::CurveParameter& start,
    const geometry::CurveParameter& end,
    Int numWraps) {

    auto result = std::make_unique<KeyEdgeData>(false);
    std::unique_ptr<geometry::AbstractStroke2d> newStroke_ =
        ked->stroke()->subStroke(start, end, numWraps);
    geometry::AbstractStroke2d* newStroke = newStroke_.get();
    result->setStroke(std::move(newStroke_));
    result->properties_.assignFromSlice(ked, start, end, numWraps, newStroke);
    return result;
}

} // namespace vgc::vacomplex
