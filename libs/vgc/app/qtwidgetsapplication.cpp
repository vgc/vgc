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

#include <vgc/app/qtwidgetsapplication.h>

#include <csignal> // signal, SIGABRT, SIGFPE, SIGILL, SIGINT, SIGSEGV, SIGTERM

#include <string_view>

#include <QDir>
#include <QIcon>
#include <QSettings>

#ifdef VGC_OS_WINDOWS
#    define WIN32_LEAN_AND_MEAN
#    include <Windows.h>
#endif

#include <vgc/app/logcategories.h>
#include <vgc/core/paths.h>
#include <vgc/ui/qtutil.h>

namespace vgc::app {

namespace {

const char* signalName(int sig) {
    switch (sig) {
    case SIGTERM:
        return "SIGTERM";
    case SIGSEGV:
        return "SIGSEGV";
    case SIGINT:
        return "SIGINT";
    case SIGILL:
        return "SIGILL";
    case SIGABRT:
        return "SIGABRT";
    case SIGFPE:
        return "SIGFPE";
    }
    return "Unknown signal";
}

const char* signalDescription(int sig) {
    switch (sig) {
    case SIGTERM:
        return "Termination request sent to the program.";
    case SIGSEGV:
        return "Invalid memory access (segmentation fault).";
    case SIGINT:
        return "External interrupt."; // usually initiated by the user
    case SIGILL:
        return "Invalid program image."; // such as invalid instruction
    case SIGABRT:
        return "Abnormal termination condition."; // e.g. initiated by std::abort()";
    case SIGFPE:
        return "Erroneous arithmetic operation (e.g., divide by zero).";
    }
    return "An error happened.";
}

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

// Prevent showing the error message twice to the user:
// - Once in onUnhandledException()
// - Once in systemSignalHandler(SIGABRT), caused by
//   std::terminate() called after onUnhandledException().
//
// See QApplicationImpl::notify() and systemSignalHandler().
//
static bool isUnhandledException_ = false;

} // namespace

namespace detail {

// Initializations that must happen before creating the QGuiApplication.
//
PreInitializer::PreInitializer() {

    // Setup a signal handler to do something meaningful on segfault, etc.
    for (int sig : {SIGABRT, SIGFPE, SIGILL, SIGINT, SIGSEGV, SIGTERM}) {
        std::signal(sig, detail::systemSignalHandler);
    }

    // Set various application attributes
    if (qopenglExperiment) {
        setAttribute(Qt::AA_ShareOpenGLContexts);
    }
    setAttribute(Qt::AA_SynthesizeMouseForUnhandledTabletEvents, false);

    // High-DPI scaling.
    //
    // Our initial choice was to explicitly disable it so that we can do it
    // manually ourself based on raw pixel values given by Qt. Unfortunately,
    // the `AA_DisableHighDpiScaling` attribute is now deprecated in Qt6 and
    // has no longer any effect: High_DPI scaling is always enabled. So for now
    // we just disable the warning by not setting the attribute, but we
    // probably need to do more testing to ensure that High-DPI scaling works
    // as we expect: perhaps we need to add some logical px to physical pixel
    // conversions in the mouse events, resize events and paint events.
    //
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    setAttribute(Qt::AA_DisableHighDpiScaling, true);
#endif
}

QApplicationImpl::QApplicationImpl(int& argc, char** argv, QtWidgetsApplication* app)
    : QApplication(argc, argv)
    , app_(app) {

#ifdef VGC_OS_MACOS
    // Fix all text in message boxes being bold in macOS.
    //
    // Also note that in Qt 5.15.2 (fixed in 5.15.3 and Qt6), there are
    // incorrect kernings with some fonts, especially the space after
    // commas/periods in the default SF Pro font starting macOS 11, see:
    //
    // https://bugreports.qt.io/browse/QTBUG-88495
    //
    // Using Helvetica Neue works around this issue.
    //
    setStyleSheet("QMessageBox QLabel {"
                  "    font-family: Helvetica Neue;"
                  "    font-size: 12pt;"
                  "    font-weight: 300;"
                  "}");
#endif
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
// XXX Instead of #ifdef VGC_DEBUG_BUILD, one option might be to check, at the
// start of the application (or each invokation of notify(), depending on how
// slow it is in each platform), whether a debugger is currently attached to
// the application. We would keep the try {} catch {} blocks only if no
// debugger is attached.
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
    catch (const std::exception& error) {
        isUnhandledException_ = true;
        app_->onUnhandledException(error.what());
        std::terminate();
    }
    catch (...) {
        isUnhandledException_ = true;
        app_->onUnhandledException("Unknown error.");
        std::terminate();
    }
#endif
}

void QApplicationImpl::onSystemSignalReceived(std::string_view errorMessage, int sig) {
    app_->onSystemSignalReceived(errorMessage, sig);
}

// TODO: get stacktrace, e.g., via https://github.com/bombela/backward-cpp
void systemSignalHandler(int sig) {
    if (isUnhandledException_) {
        exit(1);
    }
    std::string errorMessage =
        core::format("{}: {}", signalName(sig), signalDescription(sig));
    if (qApp) {
        static_cast<detail::QApplicationImpl*>(qApp)->onSystemSignalReceived(
            errorMessage, sig);
    }
    else {
        VGC_CRITICAL(LogVgcApp, errorMessage);
        exit(1);
    }
}

} // namespace detail

QtWidgetsApplication::QtWidgetsApplication(CreateKey key, int argc, char* argv[])
    : ui::Application(key, argc, argv)
    , preInitializer_()
    , argc_(argc)
    , application_(argc_, argv, this) {

    setBasePath();

#ifdef VGC_OS_WINDOWS
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

QtWidgetsApplicationPtr QtWidgetsApplication::create(int argc, char* argv[]) {
    return core::createObject<QtWidgetsApplication>(argc, argv);
}

void QtWidgetsApplication::onUnhandledException(std::string_view errorMessage) {
    VGC_CRITICAL(LogVgcApp, "Unhandled exception: {}", errorMessage);
}

void QtWidgetsApplication::onSystemSignalReceived(std::string_view errorMessage, int) {
    VGC_CRITICAL(LogVgcApp, errorMessage);
    exit(1);
}

} // namespace vgc::app
