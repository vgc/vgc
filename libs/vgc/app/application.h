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

#ifndef VGC_APP_APPLICATION_H
#define VGC_APP_APPLICATION_H

#include <QApplication>

#include <vgc/app/api.h>
#include <vgc/core/object.h>

namespace vgc::app {

class Application;

namespace detail {

struct VGC_APP_API PreInitializer {
    PreInitializer();
};

class VGC_APP_API QApplicationImpl : public QApplication {
public:
    QApplicationImpl(int& argc, char** argv, Application* app);

    // Reimplements notify to handle exceptions.
    bool notify(QObject* receiver, QEvent* event) override;

    // Handles a system signal by calling app_->onSystemSignalReceived
    void onSystemSignalReceived(std::string_view errorMessage, int sig);

private:
    Application* app_;
};

void systemSignalHandler(int sig);

} // namespace detail

VGC_DECLARE_OBJECT(Application);

/// \class vgc::app::Application
/// \brief Represents an instance of a VGC application
///
class VGC_APP_API Application : public core::Object {
    VGC_OBJECT(Application, core::Object)

protected:
    Application(int argc, char* argv[]);

public:
    /// Creates the application. Note that you must never create more than one
    /// application in a given process.
    ///
    static ApplicationPtr create(int argc, char* argv[]);

    /// Starts execution of the application.
    ///
    int exec();

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

protected:
    /// Override this function to perform any last minute operations (e.g.,
    /// saving the current document to a recovery file) if an unhandled
    /// exception is encountered during the execution of the application.
    ///
    /// The default implementation logs the error via `VGC_CRITICAL()`.
    ///
    /// It is recommended to call the default implementation at the end of your
    /// override using `SuperClass::onUnhandledException(errorMessage)`.
    ///
    /// You can use the following idiom if you wish to perform different
    /// actions based on the exception type, for example:
    ///
    /// ```cpp
    /// void MyApplication::onUnhandledException() {
    ///     std::exception_ptr ex = std::current_exception();
    ///     try {
    ///         std::rethrow_exception(ex);
    ///     }
    ///     catch (const core::IndexError& error) {
    ///         std::cout << "Index error encountered: " << error.what() << std::endl;
    ///     }
    ///     catch (...) {
    ///         std::cout << "Unknown error encountered." << std::endl;
    ///     }
    /// }
    /// ```
    ///
    virtual void onUnhandledException(std::string_view errorMessage);

    /// Override this function to perform any last minute operations (e.g.,
    /// saving the current document to a recovery file) if the application
    /// receives a system signal, i.e. one of:
    ///
    /// - SIGTERM: Termination request sent to the program.
    /// - SIGSEGV: Invalid memory access (segmentation fault).
    /// - SIGINT: External interrupt, usually initiated by the user.
    /// - SIGILL: invalid program image, such as invalid instruction.
    /// - SIGABRT: abnormal termination condition (e.g., initiated by `abort()`).
    /// - SIGFPE: erroneous arithmetic operation (e.g., divide by zero).
    ///
    /// The default implementation logs the error via `VGC_CRITICAL()` then
    /// calls `exit(1)`.
    ///
    /// It is recommended to call the base implementation at the end of your
    /// override using `SuperClass::onSystemSignalReceived(errorMessage)`.
    ///
    /// Note that the C++ standard provides very little guarantees on what
    /// functions you are allowed to call here (e.g., no dynamic allocation,
    /// see [1]), but in practice, in the operating systems supported by VGC,
    /// it is generally okay to save a file and/or show a message box, which is
    /// much preferrable than crashing without attempting these things.
    ///
    /// [1] https://en.cppreference.com/w/cpp/utility/program/signal)
    ///
    virtual void onSystemSignalReceived(std::string_view errorMessage, int sig);

private:
    friend class detail::QApplicationImpl;

    // Performs pre-initialization. Must be located before QApplication.
    detail::PreInitializer preInitializer_;

    // Note: we use QApplication (from Qt Widgets) rather than QGuiApplication
    // (from Qt Gui) since for now, we use QFileDialog and QMessageBox, which
    // are QWidgets and require an instance of QApplication.
    //
    int argc_; // we need to copy it because QApplication needs a reference.
    detail::QApplicationImpl application_;
};

} // namespace vgc::app

#endif // VGC_APP_APPLICATION_H
