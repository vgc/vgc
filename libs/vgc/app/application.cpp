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

#include <vgc/app/application.h>

#include <string_view>

#include <QDir>
#include <QIcon>
#include <QSettings>

#ifdef VGC_CORE_OS_WINDOWS
#    define WIN32_LEAN_AND_MEAN
#    include <Windows.h>
#endif

#include <vgc/core/paths.h>
#include <vgc/ui/qtutil.h>

namespace vgc::app {

namespace {

#ifdef VGC_QOPENGL_EXPERIMENT
// test fix for white artefacts during Windows window resizing.
// https://bugreports.qt.io/browse/QTBUG-89688
// indicated commit does not seem to be enough to fix the bug
void runtimePatchQt() {
    auto hMod = LoadLibraryA("platforms/qwindowsd.dll");
    if (hMod) {
        char* base = reinterpret_cast<char*>(hMod);
        char* target = base + 0x0001BA61;
        DWORD oldProt{};
        char patch[] = {'\x90', '\x90'};
        VirtualProtect(target, sizeof(patch), PAGE_EXECUTE_READWRITE, &oldProt);
        memcpy(target, patch, sizeof(patch));
        VirtualProtect(target, sizeof(patch), oldProt, &oldProt);
    }
}
constexpr bool qopenglExperiment = true;
#else
constexpr bool qopenglExperiment = false;
#endif

static void setAttribute(Qt::ApplicationAttribute attribute, bool on = true) {
    QGuiApplication::setAttribute(attribute, on);
}

// Set runtime paths from vgc.conf, an optional configuration file to be
// placed in the same folder as the executable.
//
// If vgc.conf exists, then the specified paths can be either absolute or
// relative to the directory where vgc.conf lives (that is, relative to the
// application dir path).
//
// If vgc.conf does not exist, or BasePath isn't specified, then BasePath
// is assumed to be ".." (that is, one directory above the application dir
// path).
//
// If vgc.conf does not exist, or PythonHome isn't specified, then
// PythonHome is assumed to be equal to BasePath.
//
// Note: in the future, we would probably want this to be handled directly
// by vgc::core, for example via a function core::init(argc, argv).
// For now, we keep it here for the convenience of being able to use Qt's
// applicationDirPath(), QDir, and QSettings. We don't want vgc::core to
// depend on Qt.
//
void setBasePath() {
    QString binPath = QCoreApplication::applicationDirPath();
    QDir binDir(binPath);
    binDir.makeAbsolute();
    binDir.setPath(binDir.canonicalPath()); // resolve symlinks
    QDir baseDir = binDir;
    baseDir.cdUp();
    std::string basePath = ui::fromQt(baseDir.path());
    if (binDir.exists("vgc.conf")) {
        QSettings conf(binDir.filePath("vgc.conf"), QSettings::IniFormat);
        if (conf.contains("BasePath")) {
            QString v = conf.value("BasePath").toString();
            if (!v.isEmpty()) {
                v = QDir::cleanPath(binDir.filePath(v));
                basePath = ui::fromQt(v);
            }
        }
    }
    core::setBasePath(basePath);
}

} // namespace

namespace detail {

// Initializations that must happen before creating the QGuiApplication.
//
PreInitializer::PreInitializer() {
    if (qopenglExperiment) {
        setAttribute(Qt::AA_ShareOpenGLContexts);
    }
    setAttribute(Qt::AA_SynthesizeMouseForUnhandledTabletEvents, false);
    setAttribute(Qt::AA_DisableHighDpiScaling, true);
}

QApplicationImpl::QApplicationImpl(int& argc, char** argv, Application* app)
    : QApplication(argc, argv)
    , app_(app) {
}

// Letting exceptions unhandled though QApplication::exec() causes the
// following Qt warning:
//
// « Qt has caught an exception thrown from an event handler. Throwing
// exceptions from an event handler is not supported in Qt.
// You must not let any exception whatsoever propagate through Qt code.
// If that is not possible, in Qt 5 you must at least reimplement
// QCoreApplication::notify() and catch all exceptions there. »
//
// Therefore, the try-catch below should be done here in notify(),
// than around `application_.exec()` in the implementation of
// `Application::exec()`
//
bool QApplicationImpl::notify(QObject* receiver, QEvent* event) {
#ifdef VGC_DEBUG_BUILD
    // Let exceptions go through up to the debugger to get
    // a more useful call stack.
    return QApplication::notify(receiver, event);
#else
    // Catch exceptions, let applications do last-minute save, and terminate.
    try {
        return QApplication::notify(receiver, event);
    }
    catch (...) {
        app_->onUnhandledException();
        std::terminate();
    }
#endif
};

}; // namespace detail

Application::Application(int argc, char* argv[])
    : preInitializer_()
    , argc_(argc)
    , application_(argc_, argv, this) {

    setBasePath();

#ifdef VGC_CORE_OS_WINDOWS
    for (int i = 0; i < argc_; ++i) {
        std::string_view arg = argv[i];
        if (arg == "--console") {
            AllocConsole();
            // disable ctrl+c shortcut
            SetConsoleCtrlHandler(nullptr, true);
            FILE* stream = {};
            freopen_s(&stream, "CONOUT$", "w", stdout);
            freopen_s(&stream, "CONOUT$", "w", stderr);
            break;
        }
    }
#endif
}

ApplicationPtr Application::create(int argc, char* argv[]) {
    return ApplicationPtr(new Application(argc, argv));
}

int Application::exec() {
    return application_.exec();
}

void Application::setWindowIcon(std::string_view iconPath) {
    application_.setWindowIcon(QIcon(ui::toQt(iconPath)));
}

void Application::setWindowIconFromResource(std::string_view rpath) {
    setWindowIcon(core::resourcePath(rpath));
}

void Application::onUnhandledException() {
}

} // namespace vgc::app
