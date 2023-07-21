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

#ifndef VGC_GRAPHICS_ICON_H
#define VGC_GRAPHICS_ICON_H

#include <vgc/core/object.h>

#include <vgc/geometry/rect2d.h>
#include <vgc/geometry/vec2d.h>

#include <vgc/graphics/api.h>
#include <vgc/graphics/engine.h>

namespace vgc::graphics {

namespace detail {

class IconData;
struct IconDataDeleter {
    void operator()(IconData* p);
};
using IconDataPtr = std::unique_ptr<IconData, IconDataDeleter>;

} // namespace detail

VGC_DECLARE_OBJECT(Icon);

/// \class vgc::graphics::Icon
/// \brief Utility to be able to draw
///
// XXX Overkill to make it an object?
//     We use a slot for now, reason why it is an object.
//     In the future, we may want to allow using slot in
//     lightweight non-tree-based objects, e.g.:
//
//     SignalBase > Object > TreeObject
//
class VGC_GRAPHICS_API Icon : public core::Object {
private:
    VGC_OBJECT(Icon, core::Object)
    VGC_PRIVATIZE_OBJECT_TREE_MUTATORS

protected:
    Icon(float size, std::string_view svgPath);

public:
    /// Creates an icon of the given size with the given SVG path.
    ///
    static IconPtr create(float size, std::string_view svgPath);

    /// Draws this icon with the given engine.
    ///
    void draw(graphics::Engine* engine);

    /// Returns the size of the icon.
    ///
    /// Icon designers typically ensures that shapes in the icon are contained
    /// in the box defined by the two corners (0, 0) and (width, height), with
    /// possibly a small margin for aesthetic reasons.
    ///
    geometry::Vec2f size() const {
        return geometry::Vec2f(size_, size_);
    }

private:
    // Source data
    float size_;

    // Graphics resources
    detail::IconDataPtr data_;
    graphics::GeometryViewPtr geometryView_;

    // Engine management
    graphics::Engine* lastPaintEngine_ = nullptr;
    void updateEngine_(graphics::Engine* engine);
    void setEngine_(graphics::Engine* engine);
    void releaseEngine_();
    VGC_SLOT(releaseEngineSlot_, releaseEngine_)

    // Paint callbacks
    void onPaintCreate_(graphics::Engine* engine);
    void onPaintDraw_(graphics::Engine* engine);
    void onPaintDestroy_(graphics::Engine* engine);
};

} // namespace vgc::graphics

#endif // VGC_GRAPHICS_ICON_H
