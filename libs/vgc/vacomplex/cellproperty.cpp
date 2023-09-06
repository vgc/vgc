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

#include <vgc/vacomplex/cellproperty.h>

#include <vgc/vacomplex/complex.h>
#include <vgc/vacomplex/detail/operationsimpl.h>
#include <vgc/vacomplex/keyedgedata.h>
#include <vgc/vacomplex/keyfacedata.h>

namespace vgc::vacomplex {

CellProperty::OpResult
CellProperty::onTranslateGeometry_(const geometry::Vec2d& /*delta*/) {
    return OpResult::Unchanged;
}

CellProperty::OpResult
CellProperty::onTransformGeometry_(const geometry::Mat3d& /*transformation*/) {
    return OpResult::Unchanged;
}

CellProperty::OpResult
CellProperty::onUpdateGeometry_(const geometry::AbstractStroke2d* /*newStroke*/) {
    return OpResult::Unchanged;
}

std::unique_ptr<CellProperty> CellProperty::fromConcatStep_(
    const KeyHalfedgeData& /*khd1*/,
    const KeyHalfedgeData& /*khd2*/) const {

    return nullptr;
}

std::unique_ptr<CellProperty> CellProperty::fromConcatStep_(
    const KeyFaceData& /*kfd1*/,
    const KeyFaceData& /*kfd2*/) const {

    return nullptr;
}

CellProperty::OpResult CellProperty::finalizeConcat_() {
    return OpResult::Unchanged;
}

std::unique_ptr<CellProperty> CellProperty::fromGlue_(
    core::ConstSpan<KeyHalfedgeData> /*khds*/,
    const geometry::AbstractStroke2d* /*gluedStroke*/) const {

    return nullptr;
}

CellProperties::CellProperties(const CellProperties& other) {
    if (&other != this) {
        for (const auto& [name, prop] : other.map_) {
            map_[name] = prop->clone();
            // cell_ == nullptr, no need to call emitPropertyChanged_()
        }
    }
}

CellProperties::CellProperties(CellProperties&& other) noexcept
    : map_(std::move(other.map_)) {
    // cell_ == nullptr, no need to call emitPropertyChanged_()
}

CellProperties& CellProperties::operator=(const CellProperties& other) {
    if (&other != this) {
        clear();
        for (const auto& [name, prop] : other.map_) {
            map_[name] = prop->clone();
            emitPropertyChanged_(name);
        }
    }
    return *this;
}

CellProperties& CellProperties::operator=(CellProperties&& other) noexcept {
    if (&other != this) {
        clear();
        map_ = std::move(other.map_);
        for (auto& it : map_) {
            emitPropertyChanged_(it.first);
        }
    }
    return *this;
}

const CellProperty* CellProperties::find(core::StringId name) const {
    auto it = map_.find(name);
    return it != map_.end() ? it->second.get() : nullptr;
}

void CellProperties::insert(std::unique_ptr<CellProperty>&& value) {
    // XXX: skip if equal ?
    core::StringId name = value->name();
    map_[name] = std::move(value);
    emitPropertyChanged_(name);
}

void CellProperties::remove(core::StringId name) {
    if (map_.erase(name) > 0) {
        emitPropertyChanged_(name);
    }
}

void CellProperties::clear() {
    PropertyMap tmp = std::move(map_);
    for (const auto& it : tmp) {
        emitPropertyChanged_(it.first);
    }
}

void CellProperties::onTranslateGeometry(const geometry::Vec2d& delta) {
    doOperation_([&](CellProperty* p) { return p->onTranslateGeometry_(delta); });
}

void CellProperties::onTransformGeometry(const geometry::Mat3d& transformation) {
    doOperation_(
        [&](CellProperty* p) { return p->onTransformGeometry_(transformation); });
}

void CellProperties::onUpdateGeometry(const geometry::AbstractStroke2d* newStroke) {
    doOperation_([&](CellProperty* p) { return p->onUpdateGeometry_(newStroke); });
}

namespace {

struct PropertyTemplate {
    core::StringId id;
    const CellProperty* prop;
};

} // namespace

void CellProperties::assignFromConcatStep(
    const KeyHalfedgeData& khd1,
    const KeyHalfedgeData& khd2) {

    clear();

    KeyEdgeData* ked1 = khd1.edgeData();
    KeyEdgeData* ked2 = khd2.edgeData();
    VGC_ASSERT(ked1 != nullptr && ked2 != nullptr);

    core::Array<PropertyTemplate> templates;
    for (const auto& p : ked1->properties()) {
        core::StringId id = p.first;
        if (!templates.search([id](const PropertyTemplate& p) { return p.id == id; })) {
            templates.append(PropertyTemplate{id, p.second.get()});
        }
    }
    for (const auto& p : ked2->properties()) {
        core::StringId id = p.first;
        if (!templates.search([id](const PropertyTemplate& p) { return p.id == id; })) {
            templates.append(PropertyTemplate{id, p.second.get()});
        }
    }

    for (const PropertyTemplate& p : templates) {
        std::unique_ptr<CellProperty> newProp = p.prop->fromConcatStep_(khd1, khd2);
        if (newProp) {
            insert(std::move(newProp));
        }
    }
}

void CellProperties::assignFromConcatStep(
    const KeyFaceData& kfd1,
    const KeyFaceData& kfd2) {

    clear();

    core::Array<PropertyTemplate> templates;
    for (const auto& p : kfd2.properties()) {
        core::StringId id = p.first;
        if (!templates.search([id](const PropertyTemplate& p) { return p.id == id; })) {
            templates.append(PropertyTemplate{ id, p.second.get() });
        }
    }
    for (const auto& p : kfd2.properties()) {
        core::StringId id = p.first;
        if (!templates.search([id](const PropertyTemplate& p) { return p.id == id; })) {
            templates.append(PropertyTemplate{ id, p.second.get() });
        }
    }

    for (const PropertyTemplate& p : templates) {
        std::unique_ptr<CellProperty> newProp = p.prop->fromConcatStep_(kfd1, kfd2);
        if (newProp) {
            insert(std::move(newProp));
        }
    }
}

void CellProperties::finalizeConcat() {
    doOperation_([](CellProperty* p) { return p->finalizeConcat_(); });
}

void CellProperties::glue(
    core::ConstSpan<KeyHalfedgeData> khds,
    const geometry::AbstractStroke2d* gluedStroke) {

    clear();

    core::Array<PropertyTemplate> templates;
    for (const KeyHalfedgeData& khd : khds) {
        KeyEdgeData* ked = khd.edgeData();
        for (const auto& p : ked->properties()) {
            core::StringId id = p.first;
            if (!templates.search(
                    [id](const PropertyTemplate& p) { return p.id == id; })) {
                templates.append(PropertyTemplate{id, p.second.get()});
            }
        }
    }

    for (const PropertyTemplate& p : templates) {
        std::unique_ptr<CellProperty> newProp = p.prop->fromGlue_(khds, gluedStroke);
        if (newProp) {
            insert(std::move(newProp));
        }
    }
}

/* static */
void CellProperties::emitPropertyChanged_(core::StringId name) {
    if (cell_) {
        Complex* complex = cell_->complex();
        detail::Operations ops(complex);
        ops.onPropertyChanged_(cell_, name);
    }
}

} // namespace vgc::vacomplex
