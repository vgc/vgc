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

#ifndef VGC_VACOMPLEX_CELLDATA_H
#define VGC_VACOMPLEX_CELLDATA_H

#include <map>
#include <memory> // std::unique_ptr

#include <vgc/core/arithmetic.h>
#include <vgc/core/stringid.h>
#include <vgc/vacomplex/api.h>
#include <vgc/vacomplex/cellproperty.h>

namespace vgc::vacomplex {

class Cell;
class CellData;

/// \class vgc::vacomplex::CellData
/// \brief Base authored data of a cell (geometry and properties).
///
class VGC_VACOMPLEX_API CellData {
protected:
    CellData() noexcept = default;
    ~CellData() = default; // not virtual

    CellData(Cell* owner) noexcept {
        properties_.cell_ = owner;
    }

    // protected to prevent partial copy/move
    CellData(const CellData& other) = default;
    CellData(CellData&& other) noexcept = default;
    CellData& operator=(const CellData& other) = default;
    CellData& operator=(CellData&& other) noexcept = default;

public:
    const CellProperties& properties() const {
        return properties_;
    }

    const CellProperty* findProperty(core::StringId name) const {
        return properties_.find(name);
    }

    void insertProperty(std::unique_ptr<CellProperty>&& value) {
        properties_.insert(std::move(value));
    }

    void removeProperty(core::StringId name) {
        properties_.remove(name);
    }

    void clearProperties() {
        properties_.clear();
    }

    void setProperties(const CellProperties& properties) {
        // emitPropertyChanged_() calls are handled by the copy-assign operator.
        properties_ = properties;
    }

    void setProperties(CellProperties&& properties) {
        // emitPropertyChanged_() calls are handled by the move-assign operator.
        properties_ = std::move(properties);
    }

protected:
    CellProperties properties_;

    // XXX: additional argument when it is only an affine transformation ?
    void emitGeometryChanged() const;
};

} // namespace vgc::vacomplex

#endif // VGC_VACOMPLEX_CELLDATA_H
