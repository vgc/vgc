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

#ifndef VGC_GRAPHICS_BUFFER_H
#define VGC_GRAPHICS_BUFFER_H

#include <vgc/core/arithmetic.h>
#include <vgc/graphics/api.h>
#include <vgc/graphics/enums.h>
#include <vgc/graphics/resource.h>

namespace vgc::graphics {

class ImageView;

//class BufferDataSpan {
//    BufferDataSpan(const float* data, Int length)
//        : data_(data), length_(length) {}
//
//    const float* data() const {
//        return data_;
//    }
//
//    Int length() const {
//        return length_;
//    }
//
//private:
//    const float* data_;
//    Int length_;
//};

/// \class vgc::graphics::BufferCreateInfo
/// \brief Parameters for buffer creation.
///
class VGC_GRAPHICS_API BufferCreateInfo {
public:
    constexpr BufferCreateInfo() noexcept = default;

    Usage usage() const
    {
        return usage_;
    }

    void setUsage(Usage usage)
    {
        usage_ = usage;
    }

    BindFlags bindFlags() const
    {
        return bindFlags_;
    }

    void setBindFlags(BindFlags bindFlags)
    {
        bindFlags_ = bindFlags;
    }

    CpuAccessFlags cpuAccessFlags() const
    {
        return cpuAccessFlags_;
    }

    void setCpuAccessFlags(CpuAccessFlags cpuAccessFlags)
    {
        cpuAccessFlags_ = cpuAccessFlags;
    }

    ResourceMiscFlags resourceMiscFlags() const
    {
        return resourceMiscFlags_;
    }

    void setResourceMiscFlags(ResourceMiscFlags resourceMiscFlags)
    {
        resourceMiscFlags_ = resourceMiscFlags;
    }

private:
    Usage usage_ = Usage::Default;
    BindFlags bindFlags_ = BindFlag::None;
    CpuAccessFlags cpuAccessFlags_ = CpuAccessFlag::None;
    ResourceMiscFlags resourceMiscFlags_ = ResourceMiscFlag::None;
};

/// \class vgc::graphics::Buffer
/// \brief Abstract buffer resource.
///
/// It can be bound to different views attached to the graphics pipeline.
///
class VGC_GRAPHICS_API Buffer : public Resource {
protected:
    Buffer(ResourceRegistry* registry, const BufferCreateInfo& info)
        : Resource(registry) , lengthInBytes_(0) , info_(info)
    {
        // Limitation of D3D11 impl
        const BindFlags bindFlags = this->bindFlags();
        if (bindFlags == BindFlag::None) {
            throw core::LogicError("Bind flags cannot be None");
        }
        if (bindFlags & BindFlag::ConstantBuffer) {
            if (bindFlags != BindFlag::ConstantBuffer) {
                throw core::LogicError("BindFlags::UniformBuffer cannot be combined with any other bind flag");
            }
        }
    }

public:
    Int lengthInBytes() const
    {
        return lengthInBytes_;
    }

    Usage usage() const
    {
        return info_.usage();
    }

    BindFlags bindFlags() const
    {
        return info_.bindFlags();
    }

    CpuAccessFlags cpuAccessFlags() const
    {
        return info_.cpuAccessFlags();
    }

    ResourceMiscFlags resourceMiscFlags() const
    {
        return info_.resourceMiscFlags();
    }

protected:
    friend Engine;

    Int gpuLengthInBytes_ = 0;

private:
    Int lengthInBytes_ = 0;
    BufferCreateInfo info_;
};
using BufferPtr = ResourcePtr<Buffer>;

} // namespace vgc::graphics

#endif // VGC_GRAPHICS_BUFFER_H
