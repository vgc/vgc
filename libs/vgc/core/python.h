// Copyright 2017 The VGC Developers
// See the COPYRIGHT file at the top-level directory of this distribution
// and at https://github.com/vgc.io/vgc/blob/master/COPYRIGHT
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
#include <pybind11/pybind11.h>
#include <vgc/core/api.h>

namespace vgc {
namespace core {

/// Returns the absolute path where the python wrappers of
/// the VGC libraries live.
///
VGC_CORE_API std::string pythonPath();

/// \class vgc::core::PythonInterpreter
/// \brief A thin C++ wrapper of the CPython interpreter
///
class VGC_CORE_API PythonInterpreter
{
public:
    /// Constructs a PythonInterpreter. This calls Py_Initialize(),
    /// and should be called no more than once (that is, we only
    /// support a single python interpreter at the moment).
    ///
    /// Useful info here: https://docs.python.org/3.6/c-api/init.html
    ///
    PythonInterpreter();

    /// Destructs the PythonInterpreter. This calls Py_Finalize().
    ///
    ~PythonInterpreter();

    /// Interprets the given string. This calls PyRun_String().
    ///
    void run(const std::string& str);

    /// Interprets the given string. This calls PyRun_String().
    ///
    void run(const char* str);

    /// Set the given \p value to a variable called \p name.
    ///
    template <typename T>
    void setVariableValue(const char* name, const T& value) {
        main_.attr(name) = value;
    }

private:
    // Note: the guard must be constructed first, and destructed last,
    // thus order of declaration of member variables matters.

    // XXX use pybind11::scoped_interpreter instead (need to update to latest
    // version of pybind11)
    struct ScopedInterpreter_ {
        ScopedInterpreter_();
        ~ScopedInterpreter_();
    };
    ScopedInterpreter_ guard_;

    pybind11::module main_;
    pybind11::object locals_;
    pybind11::object globals_;
};

} // namespace core
} // namespace vgc

#endif // VGC_CORE_PYTHON_H
