// Copyright 2021 The VGC Developers
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

#ifndef VGC_CORE_PYTHON_H
#define VGC_CORE_PYTHON_H

#include <string>

#if defined(Q_SLOTS) && defined(slots)
#    undef slots
#    include <pybind11/pybind11.h>
#    define slots Q_SLOTS
#else
#    include <pybind11/pybind11.h>
#endif

#include <vgc/core/api.h>
#include <vgc/core/object.h>

namespace vgc::core {

VGC_DECLARE_OBJECT(PythonInterpreter);

/// \class vgc::core::PythonInterpreter
/// \brief A thin C++ wrapper around the CPython interpreter.
///
class VGC_CORE_API PythonInterpreter : public Object {
private:
    VGC_OBJECT(PythonInterpreter, Object)

protected:
    PythonInterpreter(const std::string& programName, const std::string& pythonHome);

public:
    /// Creates a PythonInterpreter, with the following settings:
    ///
    /// - program name is set as \p programName.
    ///
    ///   See: https://docs.python.org/3/c-api/init.html#c.Py_SetProgramName
    ///
    /// - sys.path is determined from the given \p pythonHome, which must be a
    ///   string of the form prefix[:exec_prefix].
    ///
    ///   See: https://docs.python.org/3/c-api/init.html#c.Py_SetPythonHome
    ///
    /// - sys.argv is set as [""], which is the expected value when running a
    ///   Python interpreter in interactive mode.
    ///
    ///   See: https://docs.python.org/3/c-api/init.html#c.PySys_SetArgvEx
    ///
    /// Due to limitations of CPython, only one PythonInterpreter can be
    /// constructed at any given time. For simplicity, this is not enforced via
    /// a singleton pattern, so just be careful. You typically want to create
    /// the interpreter early in your main() functions, then pass it around to
    /// objects that need it.
    ///
    static PythonInterpreterPtr
    create(const std::string& programName, const std::string& pythonHome);

    /// Interprets the given string.
    ///
    void run(const std::string& str);

    /// Interprets the given string.
    ///
    void run(const char* str);

    /// Set the given \p value to a variable called \p name.
    ///
    template<typename T>
    void setVariableValue(const char* name, const T& value) {
        main_.attr(name) = value;
    }

    /// This signal is emitted when the interpreter is about to run some Python
    /// code.
    ///
    VGC_SIGNAL(runStarted);

    /// This signal is emitted when the interpreter has finished to run.
    ///
    VGC_SIGNAL(runFinished);

private:
    // Note: the scoped interpreter must be constructed first, and destructed
    // last, thus order of declaration of member variables matters.
    struct ScopedInterpreter_ {
        ScopedInterpreter_(const std::string& programName, const std::string& pythonHome);
        ~ScopedInterpreter_();
        wchar_t* programName_;
        wchar_t* pythonHome_;
        int argc_;
        wchar_t** argv_;
    };
    ScopedInterpreter_ scopedInterpreter_;

    // Exception-safe emission of runStarted and runFinished
    class ScopedRunSignalsEmitter_ {
        PythonInterpreter* p_;

    public:
        ScopedRunSignalsEmitter_(PythonInterpreter* p)
            : p_(p) {

            p_->runStarted().emit();
        }

        ~ScopedRunSignalsEmitter_() {
            p_->runFinished().emit();
        }
    };

    pybind11::module main_;
    pybind11::object locals_;
    pybind11::object globals_;
};

} // namespace vgc::core

#endif // VGC_CORE_PYTHON_H
