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
#include <vgc/ui/panelcontext.h>
#include <vgc/ui/paneldefaultarea.h>
#include <vgc/ui/widget.h>
#include <vgc/ui/module.h>

namespace vgc::ui {

VGC_DECLARE_OBJECT(Menu);
VGC_DECLARE_OBJECT(Panel);
VGC_DECLARE_OBJECT(PanelArea);
VGC_DECLARE_OBJECT(PanelManager);
VGC_DECLARE_OBJECT(Widget);

/// Uniquely identifies a panel type registered in a `PanelManager`.
///
/// This is a string that is provided by the developer of the panel, for
/// example `vgc.common.colorPalette`
///
using PanelTypeId = core::StringId;

namespace detail {

using PanelFactory = std::function<Panel*(PanelArea* /* parent */)>;

struct PanelTypeInfo {
    PanelTypeInfo(
        std::string_view label,
        PanelDefaultArea defaultArea,
        PanelFactory&& factory)

        : label_(label)
        , defaultArea_(defaultArea)
        , factory_(std::move(factory)) {
    }

    std::string label_;
    PanelDefaultArea defaultArea_;
    PanelFactory factory_;

    core::Array<Panel*> instances_;
};

using PanelTypeInfoMap = std::unordered_map<PanelTypeId, detail::PanelTypeInfo>;

} // namespace detail

/// \class vgc::ui::PanelManager
/// \brief Keeps track of information about existing or future panels
///
/// A `PanelManager` has the following responsibilities:
///
/// - Store a list of panel types that can be used for opening new panels.
///
/// - Keep track of which panels are already opened.
///
/// - Remember the last location of closed panels to re-open them in a similar
///   location.
///
class VGC_UI_API PanelManager : public ui::Module {
private:
    VGC_OBJECT(PanelManager, ui::Module)

    PanelManager(CreateKey, const ui::ModuleContext& context);

public:
    /// Creates a `PanelManager()`.
    ///
    /// The given `moduleManager` must be non-null and must outlive this `PanelManager`.
    ///
    static PanelManagerPtr create(const ui::ModuleContext& context);

    /// Registers a panel type.
    ///
    template<typename TPanel>
    void registerPanelType() {
        // XXX Use ObjectType instead of TPanel::id?
        infos_.try_emplace(
            PanelTypeId(TPanel::id),
            TPanel::label,
            TPanel::defaultArea,
            [this](ui::PanelArea* parent) {
                return this->createPanelInstance_<TPanel>(parent);
            });
        updatePanelsMenu_();
    }

    /// Returns the list of all registered panel type IDs.
    ///
    core::Array<PanelTypeId> registeredPanelTypeIds() const;

    /// Returns whether a panel type, given by its `id`, is registered in this
    /// manager.
    ///
    bool isRegistered(PanelTypeId id) const;

    /// Returns the label of a registered panel type.
    ///
    /// Throws `IndexError` if there is no registered panel type with the given
    /// `id`.
    ///
    std::string_view label(PanelTypeId id) const;

    /// Returns the `PanelDefaultArea` of a registered panel type.
    ///
    /// Throws `IndexError` if there is no registered panel type with the given
    /// `id`.
    ///
    PanelDefaultArea defaultArea(PanelTypeId id) const;

    /// Creates an instance of a registered panel type as a child of the given
    /// `parent` panel area.
    ///
    /// Throws `IndexError` if there is no registered panel type with the given
    /// `id`.
    ///
    Panel* createPanelInstance(PanelTypeId id, PanelArea* parent);

    /// \overload
    ///
    template<typename TPanel>
    Panel* createPanelInstance(PanelArea* parent) {
        return createPanelInstance(TPanel::id, parent);
    }

    /// Creates an instance of a panel.
    ///
    // XXX make this private, automatically called by the factory lambda
    // created by the manager?
    //
    // XXX do not accept extra arguments to enforce all panels can be created
    // from a menu?
    //
    template<typename TPanel, typename... Args>
    TPanel* createPanelInstance_(PanelArea* parentArea, Args&&... args) {
        ui::Widget* parentWidget = preCreatePanel_(parentArea);
        if (!parentWidget) {
            return nullptr;
        }
        PanelContext context(moduleManager());
        TPanel* panel =
            parentWidget->createChild<TPanel>(context, std::forward<Args>(args)...);
        postCreatePanel_(parentArea, panel);
        return panel;
    }

    /// Returns all existing instances of a registered panel type.
    ///
    /// Throws `IndexError` if there is no registered panel type with the given
    /// `id`.
    ///
    core::Array<Panel*> instances(PanelTypeId id) const;

    /// Returns whether a registered panel type has at least one existing
    /// instance.
    ///
    /// Throws `IndexError` if there is no registered panel type with the given
    /// `id`.
    ///
    bool hasInstance(PanelTypeId id) const {
        return !instances(id).isEmpty();
    }

    VGC_SIGNAL(createPanelInstanceRequested, (PanelTypeId, id))

private:
    detail::PanelTypeInfoMap infos_;

    void onPanelInstanceAboutToBeDestroyed_(Object* object);
    VGC_SLOT(onPanelInstanceAboutToBeDestroyed_)

    std::unordered_map<Object*, PanelTypeId> instanceToId_;

    Widget* preCreatePanel_(PanelArea* parentArea);
    void postCreatePanel_(PanelArea* parentArea, Panel* panel);

    ui::MenuWeakPtr panelsMenu_;
    void createPanelsMenu_();
    void updatePanelsMenu_();
};

} // namespace vgc::ui

#endif // VGC_UI_PANELMANAGER_H
