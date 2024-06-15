// Copyright 2024 The VGC Developers
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

#ifndef VGC_CANVAS_DEBUGDRAW_H
#define VGC_CANVAS_DEBUGDRAW_H

#include <mutex>

#include <vgc/canvas/api.h>

#include <vgc/core/array.h>
#include <vgc/graphics/engine.h>

namespace vgc::canvas {

using DebugDrawFunction = std::function<void(graphics::Engine* engine)>;

/// Adds the given `function` to a list of functions that are called whenever
/// the `Canvas` is drawn.
///
/// This is only meant for debugging purposes, to make it possible to visualize
/// intermediate steps of an algorithm that would otherwise not have access to
/// a `Canvas` or `Workspace` instance where to perform draw operations or add
/// items.
///
/// The `function` will be called at the end of the `Canvas::onPaintDraw()` call,
/// in scene coordinates.
///
/// The given `id` allows you to remove the `function` from the list where not
/// needed anymore, via `debugDrawClear(id)`.
///
/// \sa `debugDrawClear()`
///
VGC_CANVAS_API
void debugDraw(core::StringId id, DebugDrawFunction function);

/// Enables drawing things to the Canvas for debugging purposes.
///
VGC_CANVAS_API
void debugDrawClear(core::StringId id);

namespace detail {

struct DebugDraw {
    core::StringId id;
    DebugDrawFunction function;
};

VGC_CANVAS_API
core::Array<DebugDraw>& debugDraws();

VGC_CANVAS_API
std::lock_guard<std::mutex> lockDebugDraws();

} // namespace detail

} // namespace vgc::canvas

#endif // VGC_CANVAS_DEBUGDRAW_H
