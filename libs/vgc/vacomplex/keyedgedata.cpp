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

void KeyEdgeData::moveInit_(KeyEdgeData&& other) noexcept {
    CellData::moveInit_(std::move(other));
    moveInitDerivedOnly_(std::move(other));
}

void KeyEdgeData::moveInitDerivedOnly_(KeyEdgeData&& other) noexcept {
    stroke_ = std::move(other.stroke_);
    initSamplingQuality_(other);
}

KeyEdgeData::KeyEdgeData(detail::KeyEdgePrivateKey, KeyEdge* owner) noexcept
    : CellData(owner) {

    // Note: here, owner->complex() is typically still nullptr because the cell
    // has not yet been added to its parent group.
}

KeyEdgeData::KeyEdgeData(const KeyEdgeData& other)
    : CellData(other) {

    if (other.stroke_) {
        stroke_ = other.stroke_->clone();
    }
    copySamplingQuality_(other);
}

KeyEdgeData::KeyEdgeData(KeyEdgeData&& other) noexcept
    : CellData(std::move(other)) {

    moveInitDerivedOnly_(std::move(other));
}

KeyEdgeData& KeyEdgeData::operator=(const KeyEdgeData& other) {
    if (&other != this) {
        CellData::operator=(other);
        setStroke(other.stroke_.get());
        copySamplingQuality_(other);
    }
    return *this;
}

KeyEdgeData& KeyEdgeData::operator=(KeyEdgeData&& other) {
    if (&other != this) {
        CellData::operator=(std::move(other));
        setStroke(std::move(other.stroke_));
        copySamplingQuality_(other);
    }
    return *this;
}

KeyEdge* KeyEdgeData::keyEdge() const {
    Cell* cell = this->cell();
    return cell ? cell->toKeyEdge() : nullptr;
}

bool KeyEdgeData::isClosed() const {
    if (stroke_) {
        return stroke_->isClosed();
    }
    if (KeyEdge* ke = keyEdge()) {
        return ke->isClosed();
    }
    return false;
}

void KeyEdgeData::translate(const geometry::Vec2d& delta) {
    if (stroke_) {
        stroke_->translate(delta);
        dirtyStrokeSampling_();
        emitGeometryChanged();
    }
    properties_.onTranslateGeometry(delta);
}

void KeyEdgeData::transform(const geometry::Mat3d& transformation) {
    if (stroke_) {
        stroke_->transform(transformation);
        dirtyStrokeSampling_();
        emitGeometryChanged();
    }
    properties_.onTransformGeometry(transformation);
}

void KeyEdgeData::snapGeometry(
    const geometry::Vec2d& snapStartPosition,
    const geometry::Vec2d& snapEndPosition,
    geometry::CurveSnapTransformationMode mode) {

    if (stroke_ && stroke_->snap(snapStartPosition, snapEndPosition, mode)) {
        dirtyStrokeSampling_();
        emitGeometryChanged();
        properties_.onUpdateGeometry(stroke_.get());
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
            LogVgcVacomplex,
            "Assigning {} to {} caused implicit closedness conversion.",
            strokeType,
            edgeType);
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
    if (KeyEdge* ke = keyEdge()) {
        fixStrokeClosedness(stroke_.get(), ke->isClosed());
    }
    dirtyStrokeSampling_();
    emitGeometryChanged();
    properties_.onUpdateGeometry(stroke_.get());
}

void KeyEdgeData::setStroke(std::unique_ptr<geometry::AbstractStroke2d>&& newStroke) {
    stroke_ = std::move(newStroke);
    if (KeyEdge* ke = keyEdge()) {
        fixStrokeClosedness(stroke_.get(), ke->isClosed());
    }
    dirtyStrokeSampling_();
    emitGeometryChanged();
    properties_.onUpdateGeometry(stroke_.get());
}

void KeyEdgeData::setSamplingQualityOverride(geometry::CurveSamplingQuality quality) {
    hasSamplingQualityOverride_ = true;
    setSamplingQuality_(quality);
}

void KeyEdgeData::clearSamplingQualityOverride() {
    hasSamplingQualityOverride_ = false;
    if (KeyEdge* ke = keyEdge()) {
        if (Complex* complex = ke->complex()) {
            setSamplingQuality_(complex->samplingQuality());
        }
    }
}

void KeyEdgeData::closeStroke(bool smoothJoin) {
    if (KeyEdge* ke = keyEdge()) {
        if (!ke->isClosed()) {
            VGC_ERROR(
                LogVgcVacomplex,
                "closeStroke: KeyEdgeData is bound to an open KeyEdge, "
                "cannot close its stroke.");
        }
        return;
    }
    stroke_->close(smoothJoin);
    dirtyStrokeSampling_();
    emitGeometryChanged();
    properties_.onUpdateGeometry(stroke_.get());
}

//
// IDEA: do conversion to common best stroke geometry to merge
//       then match cell properties by pairs (use null if not present)

/* static */
KeyEdgeData KeyEdgeData::fromConcatStep(
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

    KeyEdgeData result;
    result.setStroke(std::move(concatStroke));
    result.properties_.assignFromConcatStep(khd1, khd2);
    return result;
}

void KeyEdgeData::finalizeConcat() {
    properties_.finalizeConcat();
}

/* static */
KeyEdgeData KeyEdgeData::fromGlueOpen(core::ConstSpan<KeyHalfedgeData> khds) {

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
KeyEdgeData KeyEdgeData::fromGlueClosed(
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
KeyEdgeData KeyEdgeData::fromGlue(
    core::ConstSpan<KeyHalfedgeData> khds,
    std::unique_ptr<geometry::AbstractStroke2d>&& gluedStroke) {

    KeyEdgeData result;
    result.setStroke(std::move(gluedStroke));
    result.properties_.glue(khds, result.stroke());
    return result;
}

KeyEdgeData KeyEdgeData::fromSlice(
    const KeyEdgeData& ked,
    const geometry::CurveParameter& start,
    const geometry::CurveParameter& end,
    Int numWraps) {

    KeyEdgeData result;
    // subStroke returns an open stroke.
    std::unique_ptr<geometry::AbstractStroke2d> newStroke_ =
        ked.stroke()->subStroke(start, end, numWraps);
    geometry::AbstractStroke2d* newStroke = newStroke_.get();
    result.setStroke(std::move(newStroke_));
    result.properties_.assignFromSlice(ked, start, end, numWraps, newStroke);
    return result;
}

void KeyEdgeData::setSamplingQuality_(geometry::CurveSamplingQuality quality) {
    if (samplingQuality_ != quality) {
        samplingQuality_ = quality;
        dirtyStrokeSampling_();
        if (Cell* cell = this->cell()) {
            if (Complex* complex = cell->complex()) {
                complex->cellSamplingQualityChanged().emit(cell);
            }
        }
    }
}

// Note 1: there is no point calling this in the following constructors:
// - KeyEdgeData()
// - KeyEdgeData(detail::KeyEdgePrivateKey, KeyEdge* owner)
// because complex() is nullptr there.
//
// Note 2: we do not want to emit cellSamplingQualityChanged() here,
// since it's an initialization, hence nothing has "changed".
//
void KeyEdgeData::initSamplingQuality_(const KeyEdgeData& other) {
    hasSamplingQualityOverride_ = other.hasSamplingQualityOverride_;
    if (hasSamplingQualityOverride_) {
        samplingQuality_ = other.samplingQuality_;
    }
    else {
        if (Cell* cell = this->cell()) {
            if (Complex* complex = cell->complex()) {
                samplingQuality_ = complex->samplingQuality();
            }
        }
    }
}

void KeyEdgeData::copySamplingQuality_(const KeyEdgeData& other) {
    hasSamplingQualityOverride_ = other.hasSamplingQualityOverride_;
    if (hasSamplingQualityOverride_) {
        setSamplingQuality_(other.samplingQuality_);
    }
}

void KeyEdgeData::updateStrokeSampling_() const {
    if (!strokeSampling_) {
        geometry::StrokeSampling2d sampling = computeStrokeSampling_(samplingQuality_);
        strokeSampling_ =
            std::make_shared<const geometry::StrokeSampling2d>(std::move(sampling));
    }
}

geometry::StrokeSampling2d
KeyEdgeData::computeStrokeSampling_(geometry::CurveSamplingQuality quality) const {
    // TODO: define guarantees.
    // - what about a closed edge without points data ?
    // - what about an open edge without points data and same end points ?
    // - what about an open edge without points data but different end points ?
    if (!stroke_) {
        geometry::StrokeSample2dArray fakeSamples = {geometry::StrokeSample2d()};
        return geometry::StrokeSampling2d(fakeSamples);
    }
    geometry::StrokeSampling2d sampling = stroke_->computeSampling(quality);
    return sampling;
}

} // namespace vgc::vacomplex
