#!/usr/bin/env python3
#
# Copyright 2021 The VGC Developers
# See the COPYRIGHT file at the top-level directory of this distribution
# and at https://github.com/vgc/vgc/blob/master/COPYRIGHT
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

# This script generates all vec2x.h/.cpp files. Usage:
#
# ./vec2.py
#
# The script assumes that it lives in vgc/core/tools, that is, it generates
# the files in its parent directy vgc/core.
#

import re
from pathlib import Path

autogenTemplateInfo='''
// This file is a "template" (not in the C++ sense) used to generate all the
// Vec2 variants. Please refer to vec2.py for more info.
'''

autogenOutputInfo='''
// This file was automatically generated, please do not edit directly.
// Instead, edit ./tools/{}, and re-run ./tools/vec2.py.
'''

def generateFile(templateFile, outputFile, tchar, tname, tdesc, relTol):
    s = templateFile.read_text()

    # Include guards, includes, class name, and other identifiers
    s = s.replace("VEC2X", "VEC2" + tchar.upper())
    s = s.replace("vec2x", "vec2" + tchar)
    s = s.replace("Vec2x", "Vec2" + tchar)

    # Scalar type. Note: we literally use "float" in the template file rather
    # than, say, "%SCALAR%", to make it a well-formed C++ file, which is
    # convenient to have meaningful warnings and auto-completion when editing
    # the template.
    s = s.replace("float", tname)
    s = s.replace("%SCALAR_DESCRIPTION%", tdesc)
    if tname == "double":
        # This should stay as "isClose(float, float)": it's a documentation
        # link to this specific overload of the function
        s = s.replace("isClose(double, double)", "isClose(float, float)")

    # Scalar values. Note: the base template file was chosen to use floats
    # rather than doubles, so that we can easily remove the "f" suffix in
    # floating point literals. This is easier than the other way around (= add
    # the "f" suffix), since some numbers are integers, or even version
    # numbers (e.g., "Apache 2.0").
    #matches = re.findall(floatValueRegex, s)
    #for match in matches:
    #    print(match.group(0))
    s = s.replace("relTol = 1e-5f", "relTol = " + relTol)
    if tname == "double":
        floatValueRegex = r"([0-9]+\.?[0-9]*([eE][+-]?[0-9]+)?)f"
        s = re.sub(floatValueRegex, r"\1", s)

    # Autogen info
    s = s.replace(autogenTemplateInfo, autogenOutputInfo.format(templateFile.name))

    outputFile.write_text(s)

def generateVariant(libDir, toolsDir, tchar, tname, tdesc, relTol):
    generateFile(toolsDir / "vec2.h", libDir / ("vec2"+tchar+".h"), tchar, tname, tdesc, relTol)
    generateFile(toolsDir / "vec2.cpp", libDir / ("vec2"+tchar+".cpp"), tchar, tname, tdesc, relTol)

# Script entry point.
#
if __name__ == "__main__":
    thisFile = Path(__file__).resolve()
    toolsDir = thisFile.parent
    libDir = toolsDir.parent
    generateVariant(libDir, toolsDir, "f", "float", "single-precision floating points", "1e-5f")
    generateVariant(libDir, toolsDir, "d", "double", "double-precision floating points", "1e-9")
