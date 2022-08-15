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

#include <vgc/core/python.h>

#include <iostream>

#include <pybind11/embed.h>
#include <pybind11/eval.h>

#include <vgc/core/paths.h>

namespace vgc::core {

PythonInterpreter::ScopedInterpreter_::ScopedInterpreter_(
    const std::string& programName,
    const std::string& pythonHome) {

    if (!Py_IsInitialized()) {

        // https://docs.python.org/3.8/c-api/init.html#c.Py_SetProgramName
        //
        // This function should be called before Py_Initialize() is called for
        // the first time, if it is called at all. It tells the interpreter the
        // value of the argv[0] argument to the main() function of the program
        // (converted to wide characters). This is used by Py_GetPath() and some
        // other functions below to find the Python run-time libraries relative
        // to the interpreter executable. The default value is 'python'.
        //
        // The argument should point to a zero-terminated wide character string
        // in static storage whose contents will not change for the duration of
        // the program’s execution. No code in the Python interpreter will
        // change the contents of this storage.
        //
        programName_ = Py_DecodeLocale(programName.data(), nullptr);
        Py_SetProgramName(programName_);

        // https://docs.python.org/3.8/c-api/init.html#c.Py_SetPythonHome
        // https://docs.python.org/3.8/using/cmdline.html#envvar-PYTHONHOME
        //
        // Set the default “home” directory, that is, the location of the
        // standard Python libraries.
        //
        // By default, the libraries are searched in prefix/lib/pythonversion
        // and exec_prefix/lib/pythonversion, where prefix and exec_prefix are
        // installation-dependent directories, both defaulting to /usr/local.
        //
        // When PYTHONHOME is set to a single directory, its value replaces both
        // prefix and exec_prefix. To specify different values for these, set
        // PYTHONHOME to prefix:exec_prefix.
        //
        // The argument should point to a zero-terminated character string in
        // static storage whose contents will not change for the duration of the
        // program’s execution. No code in the Python interpreter will change
        // the contents of this storage.
        //
        pythonHome_ = Py_DecodeLocale(pythonHome.data(), nullptr);
        Py_SetPythonHome(pythonHome_);

        // https://docs.python.org/3.8/c-api/init.html#c.Py_Initialize
        //
        // Initialize the Python interpreter. In an application embedding
        // Python, this should be called before using any other Python/C API
        // functions; see Before Python Initialization for the few exceptions.
        //
        // This initializes the table of loaded modules (sys.modules), and
        // creates the fundamental modules builtins, __main__ and sys. It also
        // initializes the module search path (sys.path). It does not set
        // sys.argv; use PySys_SetArgvEx() for that. This is a no-op when called
        // for a second time (without calling Py_FinalizeEx() first). There is
        // no return value; it is a fatal error if the initialization fails.
        //
        // Note On Windows, changes the console mode from O_TEXT to O_BINARY,
        // which will also affect non-Python uses of the console using the C
        // Runtime.
        //
        // If initsigs is 0, it skips initialization registration of signal
        // handlers, which might be useful when Python is embedded.
        //
        // Note: we use the pybind11 wrapper function
        // initialize_interpreter(initsigs), which simply calls
        // Py_InitializeEx(initsigs), then adds "." to the path. However, we
        // don't use pybind11::scoped_interpreter, since it doesn't allow to
        // set the program name or python home, as of pybind11 v2.4.2.
        //
        bool initsigs = false;
        pybind11::initialize_interpreter(initsigs);

        // https://docs.python.org/3.8/c-api/init.html#c.PySys_SetArgvEx
        //
        // Set sys.argv based on argc and argv. These parameters are similar
        // to those passed to the program’s main() function with the
        // difference that the first entry should refer to the script file to
        // be executed rather than the executable hosting the Python
        // interpreter. If there isn’t a script that will be run, the first
        // entry in argv can be an empty string.
        //
        // [...]
        //
        // It is recommended that applications embedding the Python
        // interpreter for purposes other than executing a single script pass
        // 0 as updatepath, and update sys.path themselves if desired.
        //
        // Note: the documentation is unclear whether the passed arguments
        // are copied internally, therefore like programName_, we free them
        // only after Python is finalized.
        //
        argc_ = 1;
        argv_ = new wchar_t*[argc_];
        argv_[0] = Py_DecodeLocale("", nullptr);
        // If in the future, we wish to make `argv` an additional argument of
        // PythonInterpreter, we would add here something like:
        //   for (int i = 1; i < argc_; ++i) {
        //       argv_[i] = Py_DecodeLocale(argv[i], nullptr);
        //   }
        int updatepath = 0;
        PySys_SetArgvEx(argc_, argv_, updatepath);
    }
}

PythonInterpreter::ScopedInterpreter_::~ScopedInterpreter_() {

    // Finalize the interpreter.
    //
    // Note: there exists another variant, Py_FinalizeEx(), which can tell
    // whether there was errors during finalization. We don't need this for
    // now.
    //
    // Note: we use the pybind11 wrapper function finalize_interpreter(),
    // which simply calls Py_Finalize() but also perform cleanup of
    // pybind11's internal data. See:
    //
    //   https://pybind11.readthedocs.io/en/stable/advanced/embedding.html
    //
    //   Do not use the raw CPython API functions Py_Initialize and Py_Finalize as
    //   these do not properly handle the lifetime of pybind11’s internal data.
    //
    pybind11::finalize_interpreter();

    // Free memory allocated by Py_DecodeLocale().
    //
    PyMem_RawFree(programName_);
    PyMem_RawFree(pythonHome_);
    for (int i = 0; i < argc_; ++i) {
        PyMem_RawFree(argv_[i]);
    }
}

PythonInterpreter::PythonInterpreter(
    const std::string& programName,
    const std::string& pythonHome)

    : scopedInterpreter_(programName, pythonHome) {

    // Store useful Python objects
    main_ = pybind11::module::import("__main__");
    globals_ = main_.attr("__dict__");
    locals_ = globals_;

    // Add the VGC Python extension modules to the path
    pybind11::module::import("sys").attr("path").cast<pybind11::list>().append(
        pythonPath());
}

/* static */
PythonInterpreterPtr
PythonInterpreter::create(const std::string& programName, const std::string& pythonHome) {
    return PythonInterpreterPtr(new PythonInterpreter(programName, pythonHome));
}

void PythonInterpreter::run(const std::string& str) {
    ScopedRunSignalsEmitter_ emitter(this);

    try {
        pybind11::eval<pybind11::eval_statements>(str, globals_, locals_);
    }
    catch (const pybind11::error_already_set& e) {
        std::cout << "Python error:\n" << e.what() << std::endl;

        // XXX using cout is temporary, better error handling
        // should come later.
        // or maybe the interpreter should indeed throw, and it's
        // the role of the widgets::console to display the error
        // as appropriate?
    }
}

void PythonInterpreter::run(const char* str) {
    run(std::string(str));

    // Note: the above implementation is inefficient but safe and maintainable:
    // we convert to a std::string, which is converted back to a char* by
    // pybind11. If performance happens to be a problem, then we should
    // directly use the CPython API without using std::strings.
}

} // namespace vgc::core
