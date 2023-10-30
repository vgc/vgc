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

#include <vgc/ui/application.h>

#include <QGuiApplication>
#include <QIcon>

#include <vgc/core/boolguard.h>
#include <vgc/core/paths.h>
#include <vgc/ui/logcategories.h>
#include <vgc/ui/qtutil.h>

namespace vgc::ui {

namespace {

Application* globalApplication_ = nullptr;

void onQApplicationNameChanged() {
    static bool isRecursionDetected = false;
    Application* app = application();
    QCoreApplication* qapp = qCoreApplication();
    if (app && qapp && !isRecursionDetected) {
        core::BoolGuard guard(isRecursionDetected);
        std::string name = fromQt(qapp->applicationName());
        app->setApplicationName(name);
    }
}

void onQOrganizationNameChanged() {
    static bool isRecursionDetected = false;
    Application* app = application();
    QCoreApplication* qapp = qCoreApplication();
    if (app && qapp && !isRecursionDetected) {
        core::BoolGuard guard(isRecursionDetected);
        std::string name = fromQt(qapp->organizationName());
        app->setOrganizationName(name);
    }
}

void onQOrganizationDomainChanged() {
    static bool isRecursionDetected = false;
    Application* app = application();
    QCoreApplication* qapp = qCoreApplication();
    if (app && qapp && !isRecursionDetected) {
        core::BoolGuard guard(isRecursionDetected);
        std::string name = fromQt(qapp->organizationDomain());
        app->setOrganizationDomain(name);
    }
}

} // namespace

Application* application() {
    return globalApplication_;
}

Application::Application(CreateKey key, int /*argc*/, char* /*argv*/[])
    : Object(key) {

    moduleManager_ = ModuleManager::create();

    if (globalApplication_) {
        throw core::LogicError(
            "Cannot create vgc::ui::Application: one has already been created.");
    }
    globalApplication_ = this;
}

void Application::onDestroyed() {
    globalApplication_ = nullptr;
}

ApplicationPtr Application::create(int argc, char* argv[]) {
    return core::createObject<Application>(argc, argv);
}

int Application::exec() {

    // For now, we require a QGuiApplication to exist before calling exec().
    // In the long-term future, we may want to remove the Qt dependency and
    // implement our own event loop.
    //
    QGuiApplication* qapp = qGuiApplication();
    if (!qapp) {
        throw core::LogicError(
            "Cannot call vgc::ui::Application::exec(): no QGuiApplication created.");
    }
    using Qca = QCoreApplication;
    Qca::connect(qapp, &Qca::applicationNameChanged, onQApplicationNameChanged);
    Qca::connect(qapp, &Qca::organizationNameChanged, onQOrganizationNameChanged);
    Qca::connect(qapp, &Qca::organizationDomainChanged, onQOrganizationDomainChanged);
    return qapp->exec();
}

void Application::setApplicationName(std::string_view name) {
    if (name != applicationName()) {
        applicationName_ = name;
        QCoreApplication* qapp = qCoreApplication();
        if (qapp) {
            qapp->setApplicationName(toQt(name));
        }
    }
}

void Application::setOrganizationName(std::string_view name) {
    if (name != organizationName()) {
        organizationName_ = name;
        QCoreApplication* qapp = qCoreApplication();
        if (qapp) {
            qapp->setOrganizationName(toQt(name));
        }
    }
}

void Application::setOrganizationDomain(std::string_view domain) {
    if (domain != organizationDomain()) {
        organizationDomain_ = domain;
        QCoreApplication* qapp = qCoreApplication();
        if (qapp) {
            qapp->setOrganizationDomain(toQt(domain));
        }
    }
}

void Application::setWindowIcon(std::string_view iconPath) {
    QGuiApplication* qapp = qGuiApplication();
    if (qapp) {
        qapp->setWindowIcon(QIcon(ui::toQt(iconPath)));
    }
}

void Application::setWindowIconFromResource(std::string_view rpath) {
    setWindowIcon(core::resourcePath(rpath));
}

} // namespace vgc::ui
