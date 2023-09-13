// Copyright 2023 The VGC Developers
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

#ifndef VGC_VACOMPLEX_CELLPROPERTY_H
#define VGC_VACOMPLEX_CELLPROPERTY_H

#include <memory> // std::unique_ptr, std::shared_ptr

#include <vgc/core/arithmetic.h>
#include <vgc/core/stringid.h>
#include <vgc/geometry/stroke.h> // geometry::AbstractStroke2d
#include <vgc/vacomplex/api.h>

namespace vgc::vacomplex {

class Cell;
class CellProperties;
class CellProperty;
class CellData;
class KeyEdgeData;
class KeyFaceData;

namespace detail {

class Operations;

} // namespace detail

class VGC_VACOMPLEX_API KeyHalfedgeData {
public:
    KeyHalfedgeData() noexcept = default;

    KeyHalfedgeData(KeyEdgeData* edgeData, bool direction) noexcept
        : edgeData_(edgeData)
        , direction_(direction) {
    }

    KeyEdgeData* edgeData() const {
        return edgeData_;
    }

    bool direction() const {
        return direction_;
    }

private:
    KeyEdgeData* edgeData_ = nullptr;
    bool direction_ = false;
};

enum class CellPropertyOpResult : UInt8 {
    Unsupported,
    Unchanged,
    Success
};

/// \class vgc::vacomplex::CellProperty
/// \brief Abstract authored property of a cell geometry.
///
class VGC_VACOMPLEX_API CellProperty {
protected:
    explicit CellProperty(core::StringId name) noexcept
        : name_(name) {
    }

public:
    virtual ~CellProperty() = default;

    core::StringId name() const {
        return name_;
    }

    std::unique_ptr<CellProperty> clone() const {
        return clone_();
    }

    using OpResult = CellPropertyOpResult;

protected:
    virtual std::unique_ptr<CellProperty> clone_() const = 0;

    // Returns OpResult::Unchanged by default.
    virtual OpResult onTranslateGeometry_(const geometry::Vec2d& delta);

    // Returns OpResult::Unchanged by default.
    virtual OpResult onTransformGeometry_(const geometry::Mat3d& transformation);

    // Returns OpResult::Unchanged by default.
    virtual OpResult onUpdateGeometry_(const geometry::AbstractStroke2d* newStroke);

    // Returns a null pointer by default.
    virtual std::unique_ptr<CellProperty>
    fromConcatStep_(const KeyHalfedgeData& khd1, const KeyHalfedgeData& khd2) const;

    // Returns a null pointer by default.
    virtual std::unique_ptr<CellProperty>
    fromConcatStep_(const KeyFaceData& kfd1, const KeyFaceData& kfd2) const;

    // Returns OpResult::Unchanged by default.
    virtual OpResult finalizeConcat_();

    // Returns a null pointer by default.
    virtual std::unique_ptr<CellProperty> fromGlue_(
        core::ConstSpan<KeyHalfedgeData> khds,
        const geometry::AbstractStroke2d* gluedStroke) const;

    // Returns a null pointer by default.
    virtual std::unique_ptr<CellProperty> fromSlice_(
        const KeyEdgeData* ked,
        const geometry::CurveParameter& start,
        const geometry::CurveParameter& end,
        Int numWraps,
        const geometry::AbstractStroke2d* subStroke) const;

private:
    core::StringId name_;

    friend CellProperties;
};

/// \class vgc::vacomplex::CellProperties
/// \brief Abstract authored properties of a cell (e.g.: style).
///
class VGC_VACOMPLEX_API CellProperties {
public:
    using PropertyMap = std::map<core::StringId, std::unique_ptr<CellProperty>>;

    CellProperties() noexcept = default;
    ~CellProperties() = default;

    CellProperties(const CellProperties& other);
    CellProperties(CellProperties&& other) noexcept;
    CellProperties& operator=(const CellProperties& rhs);
    CellProperties& operator=(CellProperties&& rhs) noexcept;

    const PropertyMap& map() const {
        return map_;
    }

    PropertyMap::const_iterator begin() const {
        return map_.cbegin();
    }

    PropertyMap::const_iterator end() const {
        return map_.cend();
    }

    Cell* cell() const {
        return cell_;
    }

    const CellProperty* find(core::StringId name) const;
    void insert(std::unique_ptr<CellProperty>&& value);
    void remove(core::StringId name);
    void clear();

    void onTranslateGeometry(const geometry::Vec2d& delta);
    void onTransformGeometry(const geometry::Mat3d& transformation);
    void onUpdateGeometry(const geometry::AbstractStroke2d* newStroke);

    void assignFromConcatStep(const KeyHalfedgeData& khd1, const KeyHalfedgeData& khd2);
    void assignFromConcatStep(const KeyFaceData& kfd1, const KeyFaceData& kfd2);

    void finalizeConcat();

    // Returns a null pointer by default.
    void glue(
        core::ConstSpan<KeyHalfedgeData> khds,
        const geometry::AbstractStroke2d* gluedStroke);

    void assignFromSlice(
        const KeyEdgeData* ked,
        const geometry::CurveParameter& start,
        const geometry::CurveParameter& end,
        Int numWraps,
        const geometry::AbstractStroke2d* subStroke);

private:
    std::map<core::StringId, std::unique_ptr<CellProperty>> map_;

    friend Cell;
    friend CellData;
    Cell* cell_ = nullptr;

    template<typename Op>
    bool doOperation_(const Op& op) {
        bool changed = false;
        core::Array<core::StringId> toRemove;
        for (const auto& p : map_) {
            switch (op(p.second.get())) {
            case CellProperty::OpResult::Success:
                emitPropertyChanged_(p.first);
                changed = true;
                break;
            case CellProperty::OpResult::Unchanged:
                break;
            case CellProperty::OpResult::Unsupported:
                toRemove.append(p.first);
                changed = true;
                break;
            }
        }
        for (const core::StringId& name : toRemove) {
            remove(name);
        }
        return changed;
    }

    void emitPropertyChanged_(core::StringId name);
};

} // namespace vgc::vacomplex

#endif // VGC_VACOMPLEX_CELLPROPERTY_H
