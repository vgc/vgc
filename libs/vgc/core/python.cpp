// Copyright 2018 The VGC Developers
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

#include <iostream>
#include <vgc/core/python.h>
#include <Python.h>
#include <pybind11/eval.h>

namespace vgc {
namespace core {

std::string pythonPath()
{
    // This macro is defined by the build system.
    // See vgc/core/CMakeFiles.txt
    return VGC_CORE_PYTHON_PATH_;
}

PythonInterpreter::ScopedInterpreter_::ScopedInterpreter_()
{
    Py_Initialize();
    //Py_SetProgramName(argv[0]);  /* XXX todo? */
}

PythonInterpreter::ScopedInterpreter_::~ScopedInterpreter_()
{
    Py_Finalize();
}

PythonInterpreter::PythonInterpreter()
{
    // Store useful Python objects
    main_ = pybind11::module::import("__main__");
    globals_ = main_.attr("__dict__");
    locals_ = globals_;

    // Add the VGC Python extension modules to the path
    run("import sys");
    run("sys.path.append('" + pythonPath() + "')");
}

PythonInterpreter::~PythonInterpreter()
{

}

void PythonInterpreter::run(const std::string& str)
{
    ScopedRunSignalsEmitter_ emitter(this);

    try {
        pybind11::eval<pybind11::eval_statements>(str, globals_, locals_);
    }
    catch (const pybind11::error_already_set& e) {
        std::cout << "Python error:\n"
                  << e.what()
                  << std::endl;

        // XXX using cout is temporary, better error handling
        // should come later.
        // or maybe the interpreter should indeed throw, and it's
        // the role of the widgets::console to display the error
        // as appropriate?
    }

}

void PythonInterpreter::run(const char* str)
{
    run(std::string(str));

    // Note: the above implementation is inefficient but safe and maintainable:
    // we convert to a std::string, which is converted back to a char* by
    // pybind11. If performance happens to be a problem, then we should
    // directly use the CPython API without using std::strings.
}

} // namespace core
} // namespace vgc
