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

#ifndef VGC_GRAPHICS_SAMPLERSTATE_H
#define VGC_GRAPHICS_SAMPLERSTATE_H

#include <array>

#include <vgc/core/arithmetic.h>
#include <vgc/geometry/vec4f.h>
#include <vgc/graphics/api.h>
#include <vgc/graphics/enums.h>
#include <vgc/graphics/resource.h>


namespace vgc::graphics {

/// \class vgc::graphics::SamplerStateCreateInfo
/// \brief Parameters for sampler state creation.
///
class VGC_GRAPHICS_API SamplerStateCreateInfo {
public:
    constexpr SamplerStateCreateInfo() noexcept = default;

    FilterMode magFilter() const
    {
        return magFilter_;
    }

    void setMagFilter(FilterMode magFilter)
    {
        magFilter_ = magFilter;
    }

    FilterMode minFilter() const
    {
        return minFilter_;
    }

    void setMinFilter(FilterMode minFilter)
    {
        minFilter_ = minFilter;
    }

    FilterMode mipFilter() const
    {
        return mipFilter_;
    }

    void setMipFilter(FilterMode mipFilter)
    {
        mipFilter_ = mipFilter;
    }

    UInt8 maxAnisotropy() const
    {
        return maxAnisotropy_;
    }

    void setMaxAnisotropy(UInt8 maxAnisotropy)
    {
        maxAnisotropy_ = maxAnisotropy;
    }

    ImageWrapMode wrapModeU() const
    {
        return wrapModeU_;
    }

    void setWrapModeU(ImageWrapMode wrapModeU)
    {
        wrapModeU_ = wrapModeU;
    }

    ImageWrapMode wrapModeV() const
    {
        return wrapModeV_;
    }

    void setWrapModeV(ImageWrapMode wrapModeV)
    {
        wrapModeV_ = wrapModeV;
    }

    ImageWrapMode wrapModeW() const
    {
        return wrapModeW_;
    }

    void setWrapModeW(ImageWrapMode wrapModeW)
    {
        wrapModeW_ = wrapModeW;
    }

    ComparisonFunction comparisonFunction() const
    {
        return comparisonFunction_;
    }

    void setComparisonFunction(ComparisonFunction comparisonFunction)
    {
        comparisonFunction_ = comparisonFunction;
    }

    const geometry::Vec4f& wrapColor() const
    {
        return wrapColor_;
    }

    void setWrapColor(const geometry::Vec4f& wrapColor)
    {
        wrapColor_ = wrapColor;
    }

    float mipLODBias() const
    {
        return mipLODBias_;
    }

    void setMipLODBias(float mipLODBias)
    {
        mipLODBias_ = mipLODBias;
    }

    float minLOD() const
    {
        return minLOD_;
    }

    void setMinLOD(float minLOD)
    {
        minLOD_ = minLOD;
    }

    float maxLOD() const
    {
        return maxLOD_;
    }

    void setMaxLOD(float maxLOD)
    {
        maxLOD_ = maxLOD;
    }

private:
    FilterMode magFilter_ = FilterMode::Point;
    FilterMode minFilter_ = FilterMode::Point;
    FilterMode mipFilter_ = FilterMode::Point;
    // enables anisotropic filtering if >= 1, max is 16.
    // has precedence over user-defined filter modes.
    UInt8 maxAnisotropy_ = 0;
    ImageWrapMode wrapModeU_ = ImageWrapMode::ClampToConstantColor;
    ImageWrapMode wrapModeV_ = ImageWrapMode::ClampToConstantColor;
    ImageWrapMode wrapModeW_ = ImageWrapMode::ClampToConstantColor;
    ComparisonFunction comparisonFunction_ = ComparisonFunction::Disabled;
    // enables comparison filtering if != None
    geometry::Vec4f wrapColor_ = {0.f, 0.f, 0.f, 0.f};
    float mipLODBias_ = 0.f;
    float minLOD_ = 0.f;
    float maxLOD_ = 0.f;
};

/// \class vgc::graphics::SamplerState
/// \brief Abstract pipeline sampler state.
///
class VGC_GRAPHICS_API SamplerState : public Resource {
protected:
    SamplerState(ResourceRegistry* registry, const SamplerStateCreateInfo& info)
        : Resource(registry), info_(info)
    {
    }

public:
    FilterMode magFilter() const
    {
        return info_.magFilter();
    }

    FilterMode minFilter() const
    {
        return info_.minFilter();
    }

    FilterMode mipFilter() const
    {
        return info_.mipFilter();
    }

    UInt8 maxAnisotropy() const
    {
        return info_.maxAnisotropy();
    }

    ImageWrapMode wrapModeU() const
    {
        return info_.wrapModeU();
    }

    ImageWrapMode wrapModeV() const
    {
        return info_.wrapModeV();
    }

    ImageWrapMode wrapModeW() const
    {
        return info_.wrapModeW();
    }

    ComparisonFunction comparisonFunction() const
    {
        return info_.comparisonFunction();
    }

    const geometry::Vec4f& wrapColor() const
    {
        return info_.wrapColor();
    }

    float mipLODBias() const
    {
        return info_.mipLODBias();
    }

    float minLOD() const
    {
        return info_.minLOD();
    }

    float maxLOD() const
    {
        return info_.maxLOD();
    }

private:
    SamplerStateCreateInfo info_;
};
using SamplerStatePtr = ResourcePtr<SamplerState>;

} // namespace vgc::graphics

#endif // VGC_GRAPHICS_SAMPLERSTATE_H
