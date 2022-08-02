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

#ifndef VGC_GRAPHICS_BLENDSTATE_H
#define VGC_GRAPHICS_BLENDSTATE_H

#include <array>

#include <vgc/core/arithmetic.h>
#include <vgc/graphics/api.h>
#include <vgc/graphics/enums.h>
#include <vgc/graphics/resource.h>

namespace vgc::graphics {

/// \class vgc::graphics::BlendEquation
/// \brief Describes a blend equation.
///
class VGC_GRAPHICS_API BlendEquation {
public:
    constexpr BlendEquation() noexcept = default;

    BlendEquation(BlendOp operation, BlendFactor sourceFactor, BlendFactor targetFactor)
        : operation_(operation), sourceFactor_(sourceFactor), targetFactor_(targetFactor)
    {
    }

    BlendOp operation() const
    {
        return operation_;
    }

    BlendFactor sourceFactor() const
    {
        return sourceFactor_;
    }

    BlendFactor targetFactor() const
    {
        return targetFactor_;
    }

private:
    BlendOp operation_ = BlendOp::Add;
    BlendFactor sourceFactor_ = BlendFactor::One;
    BlendFactor targetFactor_ = BlendFactor::One;
};

/// \class vgc::graphics::TargetBlendState
/// \brief Describes the blend state for some target.
///
class VGC_GRAPHICS_API TargetBlendState {
public:
    constexpr TargetBlendState() noexcept = default;

    bool isEnabled() const
    {
        return isEnabled_;
    }

    void setEnabled(bool enabled)
    {
        isEnabled_ = enabled;
    }

    BlendEquation equationRGB() const
    {
        return equationRGB_;
    }

    void setEquationRGB(const BlendEquation& equation)
    {
        equationRGB_ = equation;
    }

    void setEquationRGB(BlendOp operation, BlendFactor sourceFactor, BlendFactor targetFactor)
    {
        equationRGB_ = BlendEquation(operation, sourceFactor, targetFactor);
    }

    BlendEquation equationAlpha() const
    {
        return equationAlpha_;
    }

    void setEquationAlpha(const BlendEquation& equation)
    {
        equationAlpha_ = equation;
    }

    void setEquationAlpha(BlendOp operation, BlendFactor sourceFactor, BlendFactor targetFactor)
    {
        equationAlpha_ = BlendEquation(operation, sourceFactor, targetFactor);
    }

    BlendWriteMask writeMask() const
    {
        return writeMask_;
    }

    void setWriteMask(BlendWriteMask writeMask)
    {
        writeMask_ = writeMask;
    }

private:
    bool isEnabled_ = true;
    BlendEquation equationRGB_ = {};
    BlendEquation equationAlpha_ = {};
    BlendWriteMask writeMask_ = BlendWriteMask::All;
};

/// \class vgc::graphics::BlendStateCreateInfo
/// \brief Parameters for blend state creation.
///
class VGC_GRAPHICS_API BlendStateCreateInfo {
public:
    constexpr BlendStateCreateInfo() noexcept = default;

    bool isAlphaToCoverageEnabled() const
    {
        return isAlphaToCoverageEnabled_;
    }

    void setAlphaToCoverageEnabled(bool enabled)
    {
        isAlphaToCoverageEnabled_ = enabled;
    }

    bool isIndependentBlendEnabled() const
    {
        return isIndependentBlendEnabled_;
    }

    void setIndependentBlendEnabled(bool enabled)
    {
        isIndependentBlendEnabled_ = enabled;
    }

    const TargetBlendState& targetBlendState(Int i) const
    {
        size_t idx = castStateIndex_(i);
        return targetBlendStates_[idx];
    }

    void setTargetBlendEquationRGB(Int i, BlendOp operation, BlendFactor sourceFactor, BlendFactor targetFactor)
    {
        size_t idx = castStateIndex_(i);
        targetBlendStates_[idx].setEquationRGB(operation, sourceFactor, targetFactor);
    }

    void setTargetBlendEquationAlpha(Int i, BlendOp operation, BlendFactor sourceFactor, BlendFactor targetFactor)
    {
        size_t idx = castStateIndex_(i);
        targetBlendStates_[idx].setEquationAlpha(operation, sourceFactor, targetFactor);
    }

    void setTargetBlendEnabled(Int i, bool enabled)
    {
        size_t idx = castStateIndex_(i);
        targetBlendStates_[idx].setEnabled(enabled);
    }

    void setTargetBlendWriteMask(Int i, BlendWriteMask writeMask)
    {
        size_t idx = castStateIndex_(i);
        targetBlendStates_[idx].setWriteMask(writeMask);
    }

private:
    bool isAlphaToCoverageEnabled_ = false;
    bool isIndependentBlendEnabled_ = false;
    std::array<TargetBlendState, 8> targetBlendStates_ = {};

    size_t castStateIndex_(Int i) const
    {
        size_t idx = core::int_cast<size_t>(i);
        if (idx >= targetBlendStates_.size()) {
            throw core::IndexError(core::format(
                "Target blend state index {} is out of range [0, {}]", i, targetBlendStates_.size() - 1));
        }
        return idx;
    }
};

/// \class vgc::graphics::BlendState
/// \brief Abstract pipeline blend state.
///
class VGC_GRAPHICS_API BlendState : public Resource {
protected:
    BlendState(ResourceRegistry* registry, const BlendStateCreateInfo& info)
        : Resource(registry), info_(info)
    {
    }

public:
    bool isAlphaToCoverageEnabled() const
    {
        return info_.isAlphaToCoverageEnabled();
    }

    bool isIndependentBlendEnabled() const
    {
        return info_.isIndependentBlendEnabled();
    }

    const TargetBlendState& targetBlendState(Int i) const
    {
        return info_.targetBlendState(i);
    }

private:
    BlendStateCreateInfo info_;
};
using BlendStatePtr = ResourcePtr<BlendState>;

} // namespace vgc::graphics

#endif // VGC_GRAPHICS_BLENDSTATE_H
