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

#include <vgc/vacomplex/keyfacedata.h>

#include <vgc/vacomplex/detail/operationsimpl.h>
#include <vgc/vacomplex/keyface.h>

namespace vgc::vacomplex {

KeyFaceData::KeyFaceData(KeyFace* owner, detail::KeyFacePrivateKey) noexcept
    : CellData(owner) {
}

KeyFaceData::KeyFaceData(const KeyFaceData& other)
    : CellData(other) {
}

KeyFaceData::KeyFaceData(KeyFaceData&& other) noexcept
    : CellData(std::move(other)) {
}

KeyFaceData& KeyFaceData::operator=(const KeyFaceData& other) {
    if (&other != this) {
        CellData::operator=(other);
    }
    return *this;
}

KeyFaceData& KeyFaceData::operator=(KeyFaceData&& other) noexcept {
    if (&other != this) {
        CellData::operator=(std::move(other));
    }
    return *this;
}

KeyFace* KeyFaceData::keyFace() const {
    Cell* cell = properties_.cell();
    return cell ? cell->toKeyFace() : nullptr;
}

void KeyFaceData::translate(const geometry::Vec2d& delta) {
    properties_.onTranslateGeometry(delta);
}

void KeyFaceData::transform(const geometry::Mat3d& transformation) {
    properties_.onTransformGeometry(transformation);
}

/* static */
void
KeyFaceData::assignFromConcatStep(KeyFaceData& result, const KeyFaceData& kfd1, const KeyFaceData& kfd2) {
    result.properties_.assignFromConcatStep(kfd1, kfd2);
}

void KeyFaceData::finalizeConcat() {
    properties_.finalizeConcat();
}

} // namespace vgc::vacomplex
