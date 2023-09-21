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

#ifndef VGC_UI_PANELMANAGER_H
#define VGC_UI_PANELMANAGER_H

#include <vgc/core/id.h>
#include <vgc/core/object.h>
#include <vgc/ui/api.h>

namespace vgc::ui {

VGC_DECLARE_OBJECT(Panel);
VGC_DECLARE_OBJECT(PanelArea);

/// A `PanelFactory` implementation should create a new `Panel`
/// as a child of the given `PanelArea` and return it.
///
/// Example:
///
/// ```cpp
/// auto colorsPanelFactory = [app](PanelArea* parent) {
///     Panel* panel = createPanelWithPadding(parent, "Colors");
///     palette = parent->createChild<tools::ColorPalette>();
///     palette_->colorSelected().connect(app->onColorChangedSlot_());
/// };
/// ```
///
using PanelFactory = std::function<Panel*(PanelArea* /* parent */)>;

using PanelId = core::Id;

/// \class RegisteredPanel
/// \brief Stores information about a panel type registered in a `PanelManager`.
///
class RegisteredPanel {
public:
    /// Creates a `RegisteredPanel`.
    ///
    RegisteredPanel(std::string_view label, PanelFactory&& factory)
        : id_(core::genId())
        , label_(label)
        , factory_(std::move(factory)) {
    }

    /// Creates an instance of this registered panel by
    /// as a child of the given `parent` PanelArea by calling the `factory()` function.
    ///
    Panel* create(PanelArea* parent) {
        return factory_(parent);
    }

    /// Returns the ID of this registered panel.
    ///
    PanelId id() const {
        return id_;
    }

    /// Returns the label of this registered panel.
    ///
    std::string_view label() const {
        return label_;
    }

    /// Returns the factory function of this registered panel.
    ///
    const PanelFactory& factory() const {
        return factory_;
    }

private:
    PanelId id_;
    std::string label_;
    PanelFactory factory_;
};

/// \class vgc::ui::PanelManager
/// \brief Keeps track of information about existing of future panels
///
/// A `PanelManager` has the following responsibilities:
/// - Store a list of panel specifications for opening new panels.
/// - Keep track of which panels are already opened.
/// - Remember the last location of closed panels to re-open them in a similar
///   location.
///
class VGC_UI_API PanelManager {
public:
    PanelManager();

    /// Registers a given panel type, and returns a unique identifier for this
    /// panel type.
    ///
    PanelId registerPanel(std::string_view label, PanelFactory&& factory);

private:
    core::Array<RegisteredPanel> registeredPanels_;
};

} // namespace vgc::ui

#endif // VGC_UI_PANELMANAGER_H
