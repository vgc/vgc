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

#ifndef VGC_UI_APPLICATION_H
#define VGC_UI_APPLICATION_H

#include <vgc/core/object.h>
#include <vgc/ui/api.h>
#include <vgc/ui/module.h>

namespace vgc::ui {

class Application;

VGC_DECLARE_OBJECT(Application);

/// Returns the global `Application` object.
///
Application* application();

/// \class vgc::ui::Application
/// \brief Represents the global application object.
///
/// Note that for now, you must create a `QGuiApplication` before
/// being able to call `exec()` on the `ui::Application` object or
/// create any `ui::Window`.
///
/// You can choose to either create a `QGuiApplication` or a `QApplication`,
/// depending on whether you need widgets or dialogs from QtWidgets.
///
/// Example:
///
/// ```
/// int main(int argc, char* argv[]) {
///     auto app = ui::Application::create(argc, argv);
///     QGuiApplication qapp;
///     return app.exec();
/// }
/// ```
///
class VGC_UI_API Application : public core::Object {
    VGC_OBJECT(Application, core::Object)

protected:
    Application(CreateKey, int argc, char* argv[]);
    void onDestroyed() override;

public:
    /// Creates the application. Note that you must never create more than one
    /// application in a given process.
    ///
    static ApplicationPtr create(int argc, char* argv[]);

    /// Starts execution of the application.
    ///
    int exec();

    /// Returns the application name.
    ///
    /// \sa `setApplicationName()`.
    ///
    const std::string& applicationName() const {
        return applicationName_;
    }

    /// Sets the application name, e.g. "VGC Illustration".
    ///
    /// Note that this is used by `settings()` in order to know where the
    /// settings are stored, so it must be set to a proper value before calling
    /// `settings()` for the first time.
    ///
    /// \sa `applicationName()`.
    ///
    void setApplicationName(std::string_view name);

    /// Returns the organization name.
    ///
    /// \sa `setOrganizationName()`.
    ///
    const std::string& organizationName() const {
        return organizationName_;
    }

    /// Sets the organization name, e.g., "VGC Software".
    ///
    /// Note that this is used by `settings()` in order to know where the
    /// settings are stored, so it must be set to a proper value before calling
    /// `settings()` for the first time.
    ///
    /// \sa `organizationName()`.
    ///
    void setOrganizationName(std::string_view name);

    /// Returns the organization domain.
    ///
    /// \sa `setOrganizationDomain()`.
    ///
    const std::string& organizationDomain() const {
        return organizationDomain_;
    }

    /// Sets the organization domain, e.g., "vgc.io".
    ///
    /// Note that this is used by `settings()` in order to know where the
    /// settings are stored, so it must be set to a proper value before calling
    /// `settings()` for the first time.
    ///
    /// \sa `organizationDomain()`.
    ///
    void setOrganizationDomain(std::string_view domain);

    /// Set the default window icons for all windows in this application.
    ///
    /// ```cpp
    /// setWindowIcon(vgc::core::resourcePath("apps/illustration/icons/512.png")
    /// ```
    ///
    void setWindowIcon(std::string_view iconPath);

    /// This is equivalent to:
    ///
    /// ```cpp
    /// setWindowIcon(vgc::core::resourcePath(rpath));
    /// ```
    ///
    /// Example:
    ///
    /// ```cpp
    /// setWindowIconFromResource("apps/illustration/icons/512.png");
    /// ```
    ///
    void setWindowIconFromResource(std::string_view rpath);

    /// Returns the module manager of the application.
    ///
    ModuleManager* moduleManager() const {
        return moduleManager_.get();
    }

    /// Retrieves the given `TModule` module, or creates it if there is no such
    /// module yet.
    ///
    template<typename TModule>
    core::ObjPtr<TModule> importModule() const {
        return moduleManager()->importModule<TModule>();
    }

    /// This signal is emitted when a module is created.
    ///
    VGC_SIGNAL(moduleCreated, (Module*, module))

private:
    std::string applicationName_;
    std::string organizationName_;
    std::string organizationDomain_;
    ModuleManagerPtr moduleManager_;
};

} // namespace vgc::ui

#endif // VGC_UI_APPLICATION_H
