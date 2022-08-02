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

#ifndef VGC_GRAPHICS_RASTERIZERSTATE_H
#define VGC_GRAPHICS_RASTERIZERSTATE_H

#include <array>

#include <vgc/core/arithmetic.h>
#include <vgc/graphics/api.h>
#include <vgc/graphics/enums.h>
#include <vgc/graphics/resource.h>

namespace vgc::graphics {

/// \class vgc::graphics::RasterizerStateCreateInfo
/// \brief Parameters for rasterizer state creation.
///
class VGC_GRAPHICS_API RasterizerStateCreateInfo {
public:
    constexpr RasterizerStateCreateInfo() noexcept = default;

    FillMode fillMode() const
    {
        return fillMode_;
    }

    void setFillMode(FillMode fillMode)
    {
        fillMode_ = fillMode;
    }

    CullMode cullMode() const
    {
        return cullMode_;
    }

    void setCullMode(CullMode cullMode)
    {
        cullMode_ = cullMode;
    }

    bool isFrontCounterClockwise() const
    {
        return isFrontCounterClockwise_;
    }

    void setFrontCounterClockwise_(bool isFrontCounterClockwise)
    {
        isFrontCounterClockwise_ = isFrontCounterClockwise;
    }

    bool isDepthClippingEnabled() const
    {
        return isDepthClippingEnabled_;
    }

    void setDepthClippingEnabled(bool enabled)
    {
        isDepthClippingEnabled_ = enabled;
    }

    bool isScissoringEnabled() const
    {
        return isScissoringEnabled_;
    }

    void setScissoringEnabled(bool enabled)
    {
        isScissoringEnabled_ = enabled;
    }

    bool isMultisamplingEnabled() const
    {
        return isMultisamplingEnabled_;
    }

    void setMultisamplingEnabled(bool enabled)
    {
        isMultisamplingEnabled_ = enabled;
    }

    bool isLineAntialiasingEnabled() const
    {
        return isLineAntialiasingEnabled_;
    }

    void setLineAntialiasingEnabled(bool enabled)
    {
        isLineAntialiasingEnabled_ = enabled;
    }

private:
    FillMode fillMode_ = FillMode::Solid;
    CullMode cullMode_ = CullMode::None;
    bool isFrontCounterClockwise_ = true;
    //Int32 depthBias_;
    //float depthBiasClamp_;
    //float slopeScaledDepthBias_;
    bool isDepthClippingEnabled_ = false;
    bool isScissoringEnabled_ = false;
    bool isMultisamplingEnabled_ = false;
    bool isLineAntialiasingEnabled_ = false;
};

/// \class vgc::graphics::RasterizerState
/// \brief Abstract pipeline rasterizer state.
///
class VGC_GRAPHICS_API RasterizerState : public Resource {
protected:
    RasterizerState(ResourceRegistry* registry, const RasterizerStateCreateInfo& info)
        : Resource(registry), info_(info)
    {
    }

public:
    FillMode fillMode() const
    {
        return info_.fillMode();
    }

    CullMode cullMode() const
    {
        return info_.cullMode();
    }

    bool isFrontCounterClockwise() const
    {
        return info_.isFrontCounterClockwise();
    }

    bool isDepthClippingEnabled() const
    {
        return info_.isDepthClippingEnabled();
    }

    bool isScissoringEnabled() const
    {
        return info_.isScissoringEnabled();
    }

    bool isMultisamplingEnabled() const
    {
        return info_.isMultisamplingEnabled();
    }

    bool isLineAntialiasingEnabled() const
    {
        return info_.isLineAntialiasingEnabled();
    }

private:
    RasterizerStateCreateInfo info_;
};
using RasterizerStatePtr = ResourcePtr<RasterizerState>;

} // namespace vgc::graphics

#endif // VGC_GRAPHICS_RASTERIZERSTATE_H
