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

#ifndef VGC_VACOMPLEX_KEYFACEDATA_H
#define VGC_VACOMPLEX_KEYFACEDATA_H

#include <memory>

#include <vgc/core/arithmetic.h>
#include <vgc/vacomplex/api.h>
#include <vgc/vacomplex/celldata.h>

namespace vgc::vacomplex {

class KeyFace;
class KeyFaceData;

namespace detail {

class Operations;

struct KeyFacePrivateKey {
private:
    friend KeyFace;
    constexpr KeyFacePrivateKey() noexcept = default;
};

} // namespace detail

/// \class vgc::vacomplex::KeyFaceData
/// \brief Authored model of the face geometry.
///
class VGC_VACOMPLEX_API KeyFaceData final : public CellData {
private:
    friend detail::Operations;

public:
    KeyFaceData() noexcept = default;
    ~KeyFaceData() = default;

    KeyFaceData(KeyFace* owner, detail::KeyFacePrivateKey) noexcept;

    KeyFaceData(const KeyFaceData& other);
    KeyFaceData(KeyFaceData&& other) noexcept;
    KeyFaceData& operator=(const KeyFaceData& other);
    KeyFaceData& operator=(KeyFaceData&& other) noexcept;

    KeyFace* keyFace() const;

    /// Expects delta in object space.
    ///
    void translate(const geometry::Vec2d& delta);

    /// Expects transformation in object space.
    ///
    void transform(const geometry::Mat3d& transformation);

    static void assignFromConcatStep(KeyFaceData& result, const KeyFaceData& kfd1, const KeyFaceData& kfd2);

    void finalizeConcat();

private:
    // no extra data, only properties atm
};

} // namespace vgc::vacomplex

#endif // VGC_VACOMPLEX_KEYFACEDATA_H
