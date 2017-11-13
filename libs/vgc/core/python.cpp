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

#include <vgc/core/python.h>
#include <Python.h>

namespace vgc {
namespace core {

std::string pythonPath()
{
    // This macro is defined by the build system.
    // See vgc/core/CMakeFiles.txt
    return VGC_CORE_PYTHON_PATH_;
}

PythonInterpreter::PythonInterpreter()
{
    // Initialize the python interpreter
    Py_Initialize();
    //Py_SetProgramName(argv[0]);  /* XXX todo? */

    // Add the VGC Python libraries to the path
    run("import sys");
    run("sys.path.append('" + vgc::core::pythonPath() + "')");
}

PythonInterpreter::~PythonInterpreter()
{
    Py_Finalize();
}

void PythonInterpreter::run(const std::string& str)
{
    run(str.c_str());
}

void PythonInterpreter::run(const char* str)
{
    PyRun_SimpleString(str);
}

} // namespace core
} // namespace vgc
